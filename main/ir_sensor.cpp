#include "ir_sensor.h"

#include "ir_common.h"
#include "real_time.h"

using namespace IrCommon;

static void CSensorEventThread(void* args);
static void IRAM_ATTR IrqHandler(void* args);

static QueueHandle_t s_sensor_queue = nullptr;

void IrSensor::Init() {
    constexpr int kSensorQueueSize = 100;
    s_sensor_queue = xQueueCreate(kSensorQueueSize, sizeof(IrEvent));

    constexpr gpio_config_t kSensorConf{.pin_bit_mask = 1ULL << kIrSensorGpioNum,
                                        .mode = GPIO_MODE_INPUT,
                                        .pull_up_en = GPIO_PULLUP_DISABLE,
                                        .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                        .intr_type = GPIO_INTR_ANYEDGE};
    assert(gpio_config(&kSensorConf) == ESP_OK);
    assert(gpio_isr_handler_add(kIrSensorGpioNum, IrqHandler, nullptr) == ESP_OK);

    constexpr uint32_t kSensorEventThreadStackSize = 5000;
    constexpr UBaseType_t kSensorEventThreadPriority = 2;
    assert(xTaskCreate(CSensorEventThread, "Sensor Event", kSensorEventThreadStackSize,
                       static_cast<void*>(this), kSensorEventThreadPriority,
                       &sensor_event_thread_) == pdPASS);
}

static void CSensorEventThread(void* args) {
    IrSensor* instance = static_cast<IrSensor*>(args);
    instance->SensorEventThread();
}

void IrSensor::SensorEventThread() {
    SharpProtocolState cur_state = SharpProtocolState::kWaitForMsgStart;
    etl::vector<IrEvent, kCodeEventLength> event_code{};
    IrEvent event{};

    auto reset_state_machine = [&]() {
        cur_state = SharpProtocolState::kWaitForMsgStart;
        event_code.clear();
    };

    while (true) {
        assert(xQueueReceive(s_sensor_queue, &event, portMAX_DELAY) == pdTRUE);
        printf("| IR Event | %4s | %10llu us |\n", IrValueToString(event.value), event.time_us);

        if (cur_state != SharpProtocolState::kWaitForMsgStart && event.value == IrValue::kLow &&
            event.time_us > 2000) {
            printf("Received message start when not expecting one! Discarding active message.\n");
            reset_state_machine();
            cur_state = SharpProtocolState::kWaitForStartPulse;
            continue;
        }

        switch (cur_state) {
            case SharpProtocolState::kWaitForMsgStart:
                if (event.value == IrValue::kLow && event.time_us > 2000) {
                    cur_state = SharpProtocolState::kWaitForStartPulse;
                    continue;
                } else {
                    printf("Was expecting message start event and received: %s %llu us\n",
                           IrValueToString(event.value), event.time_us);
                    // cur_state stays the same (kWaitForMsgStart)
                    continue;
                }
                break;
            case SharpProtocolState::kWaitForStartPulse:
                if (event.value == IrValue::kHigh && event.time_us < 500) {
                    if (event_code.size() < kCodeEventLength) {
                        event_code.emplace_back(event);
                        cur_state = SharpProtocolState::kWaitForLogicPulse;
                        continue;
                    } else {
                        // Message complete
                        etl::vector<bool, kCodeBitLength> bit_code = GpioEventCodeToBitCode(
                            gsl::span<IrEvent, kCodeEventLength>(event_code));
                        uint16_t code_val =
                            BitCodeToUint16(gsl::span<bool, kCodeBitLength>(bit_code));
                        printf("Received code: 0x%4X\n", code_val);

                        reset_state_machine();
                        continue;
                    }
                } else {
                    printf("Was expecting start pulse and received: %s %llu us\n",
                           IrValueToString(event.value), event.time_us);
                    printf("Resetting message state machine and discarding current message.\n");
                    reset_state_machine();
                    continue;
                }
                break;
            case SharpProtocolState::kWaitForLogicPulse:
                if (event.value == IrValue::kLow && event.time_us > 500 && event.time_us < 2000) {
                    event_code.emplace_back(event);
                    cur_state = SharpProtocolState::kWaitForStartPulse;
                    continue;
                } else {
                    printf("Was expecting logic pulse and received: %s %llu us\n",
                           IrValueToString(event.value), event.time_us);
                    printf("Resetting message state machine and discarding current message.\n");
                    reset_state_machine();
                    continue;
                }
                break;
        }
    }
}

static void IrqHandler(void* args) {
    static uint64_t s_last_time_us = 0;

    bool gpio_val = gpio_get_level(IrSensor::kIrSensorGpioNum) == 1;
    uint64_t cur_time_us = RealTime::GetInstance().GetTimeUs();

    // GPIO high means IR active aka HIGH
    IrEvent event{.value = gpio_val ? IrValue::kHigh : IrValue::kLow,
                  .time_us = RealTime::GetTimeDiffUs(s_last_time_us, cur_time_us)};
    assert(xQueueSendFromISR(s_sensor_queue, &event, nullptr) == pdTRUE);

    s_last_time_us = cur_time_us;
}