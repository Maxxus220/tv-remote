#include "ir_transmitter.h"

#include "esp_attr.h"
#include "ir_common.h"

using namespace IrCommon;

static void CIrTransmitThread(void* args);
static bool IRAM_ATTR TransmitTimerIrqHandler(gptimer_handle_t timer,
                                              const gptimer_alarm_event_data_t* event_data,
                                              void* user_ctx);

static SemaphoreHandle_t s_timer_alarm_fired_sem_{nullptr};

void IrTransmitter::Init() {
    ledc_timer_config_t kLedcPwmTimerConfig{
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_7_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 38000,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };
    assert(ledc_timer_config(&kLedcPwmTimerConfig) == ESP_OK);

    ledc_channel_config_t kLedcPwmChannelConfig{
        .gpio_num = kIrTransmitterPwmGpioNum,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags{
            .output_invert = 0,
        },
    };
    assert(ledc_channel_config(&kLedcPwmChannelConfig) == ESP_OK);

    s_timer_alarm_fired_sem_ = xSemaphoreCreateBinary();
    assert(s_timer_alarm_fired_sem_ != nullptr);

    constexpr gptimer_config_t kTransmitTimerConfig{
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,    // 1 us per tick
        .intr_priority = 0,
        .flags = {.intr_shared = 0, .backup_before_sleep = 0}};
    assert(gptimer_new_timer(&kTransmitTimerConfig, &transmit_timer_) == ESP_OK);

    constexpr gptimer_event_callbacks_t kTransmitTimerCallbacksConfig{.on_alarm =
                                                                          TransmitTimerIrqHandler};
    assert(gptimer_register_event_callbacks(transmit_timer_, &kTransmitTimerCallbacksConfig,
                                            nullptr) == ESP_OK);
    assert(gptimer_enable(transmit_timer_) == ESP_OK);

    constexpr int kIrTransmitQueueSize = 5;
    ir_transmit_queue_ = xQueueCreate(kIrTransmitQueueSize, sizeof(uint16_t));

    constexpr uint32_t kIrTransmitStackSize = 5000;
    constexpr UBaseType_t kIrTransmitPriority = 2;
    assert(xTaskCreate(CIrTransmitThread, "IR Transmit", kIrTransmitStackSize,
                       static_cast<void*>(this), kIrTransmitPriority,
                       &ir_transmit_thread_) == pdPASS);
}

void IrTransmitter::SendCode(const uint16_t code) {
    assert(xQueueSend(ir_transmit_queue_, &code, portMAX_DELAY) == pdTRUE);
}

void CIrTransmitThread(void* args) {
    IrTransmitter* instance = static_cast<IrTransmitter*>(args);
    instance->IrTransmitThread();
}

void IrTransmitter::IrTransmitThread() {
    uint16_t code{};
    while (true) {
        assert(xQueueReceive(ir_transmit_queue_, &code, portMAX_DELAY) == pdTRUE);
        printf("Sending code: 0x%04X\n", code);

        etl::vector<bool, kCodeBitLength> bit_code = Uint16ToBitCode(code);
        etl::vector<IrEvent, kCodeEventLength> event_code =
            BitCodeToGpioEventCode(gsl::span<bool, kCodeBitLength>(bit_code));

        for (auto event : event_code) {
            uint32_t pwm_duty = event.value == IrValue::kHigh ? 32 : 0;
            assert(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_duty) == ESP_OK);
            assert(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) == ESP_OK);
            assert(gptimer_set_raw_count(transmit_timer_, 0) == ESP_OK);
            const gptimer_alarm_config_t timer_alarm_config{
                .alarm_count = event.time_us, .reload_count = 0, .flags{.auto_reload_on_alarm = 0}};
            assert(gptimer_set_alarm_action(transmit_timer_, &timer_alarm_config) == ESP_OK);
            assert(gptimer_start(transmit_timer_) == ESP_OK);
            assert(xSemaphoreTake(s_timer_alarm_fired_sem_, portMAX_DELAY) == pdTRUE);
        }

        assert(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 32) == ESP_OK);
        assert(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) == ESP_OK);

        assert(gptimer_set_raw_count(transmit_timer_, 0) == ESP_OK);
        constexpr gptimer_alarm_config_t k320UsTimerAlarmConfig{
            .alarm_count = 320, .reload_count = 0, .flags{.auto_reload_on_alarm = 0}};
        assert(gptimer_set_alarm_action(transmit_timer_, &k320UsTimerAlarmConfig) == ESP_OK);
        assert(gptimer_start(transmit_timer_) == ESP_OK);
        assert(xSemaphoreTake(s_timer_alarm_fired_sem_, portMAX_DELAY) == pdTRUE);

        assert(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0) == ESP_OK);
        assert(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0) == ESP_OK);

        assert(gptimer_set_raw_count(transmit_timer_, 0) == ESP_OK);
        constexpr gptimer_alarm_config_t k40MsTimerAlarmConfig{
            .alarm_count = 40000, .reload_count = 0, .flags{.auto_reload_on_alarm = 0}};
        assert(gptimer_set_alarm_action(transmit_timer_, &k40MsTimerAlarmConfig) == ESP_OK);
        assert(gptimer_start(transmit_timer_) == ESP_OK);
        assert(xSemaphoreTake(s_timer_alarm_fired_sem_, portMAX_DELAY) == pdTRUE);
    }
}

bool TransmitTimerIrqHandler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* event_data,
                             void* user_ctx) {
    (void)event_data;
    (void)user_ctx;
    gptimer_stop(timer);

    BaseType_t higher_priority_task_woken{};
    xSemaphoreGiveFromISR(s_timer_alarm_fired_sem_, &higher_priority_task_woken);
    return higher_priority_task_woken == pdTRUE;
}