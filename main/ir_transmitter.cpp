#include "ir_transmitter.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "ir_common.h"

using namespace IrCommon;

static void CIrTransmitThread(void* args);
static bool IRAM_ATTR TransmitTimerIrqHandler(gptimer_handle_t timer,
                                              const gptimer_alarm_event_data_t* event_data,
                                              void* user_ctx);

static SemaphoreHandle_t s_timer_alarm_fired_sem_{nullptr};

void IrTransmitter::Init() {

    constexpr gpio_config_t kIrTransmitterConfig{.pin_bit_mask = 1ULL << kIrTransmitterGpioNum,
                                                 .mode = GPIO_MODE_OUTPUT,
                                                 .pull_up_en = GPIO_PULLUP_DISABLE,
                                                 .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                                 .intr_type = GPIO_INTR_DISABLE};
    assert(gpio_config(&kIrTransmitterConfig) == ESP_OK);

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
        etl::vector<bool, kCodeBitLength> bit_code = Uint16ToBitCode(code);
        etl::vector<IrEvent, kCodeEventLength> event_code =
            BitCodeToGpioEventCode(gsl::span<bool, kCodeBitLength>(bit_code));
        for (auto event : event_code) {
            gpio_set_level(kIrTransmitterGpioNum, event.value == IrValue::kHigh ? 1 : 0);
            // TODO: Use hardware timer to wait event.time_us
        }
        // TODO: Send end sequence. 320us HIGH set low (forever) return.
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