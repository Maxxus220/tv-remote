#include "ir_sensor.h"

#include "real_time.h"

static void CSensorEventThread(void* args);
static void IRAM_ATTR IrqHandler(void* args);

static QueueHandle_t s_sensor_queue = nullptr;

void IrSensor::Init() {
    constexpr int kSensorQueueSize = 100;
    s_sensor_queue = xQueueCreate(kSensorQueueSize, sizeof(SensorEvent));

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
    etl::vector<SensorEvent, kCodeEventLength> event_code{};
    SensorEvent event{};

    auto reset_state_machine = [&]() {
        cur_state = SharpProtocolState::kWaitForMsgStart;
        event_code.clear();
    };

    while (true) {
        assert(xQueueReceive(s_sensor_queue, &event, portMAX_DELAY) == pdTRUE);
        printf("| Sensor Event | %s | %10llu us |\n", event.value ? "HIGH" : " LOW", event.time_us);

        if (cur_state != SharpProtocolState::kWaitForMsgStart && event.value == true &&
            event.time_us > 2000) {
            printf("Received message start when not expecting one! Discarding active message.\n");
            reset_state_machine();
            cur_state = SharpProtocolState::kWaitForStartPulse;
            continue;
        }

        switch (cur_state) {
            case SharpProtocolState::kWaitForMsgStart:
                if (event.value == true && event.time_us > 2000) {
                    cur_state = SharpProtocolState::kWaitForStartPulse;
                    continue;
                } else {
                    printf("Was expecting message start event and received: %s %llu us\n",
                           event.value ? "HIGH" : "LOW", event.time_us);
                    // cur_state stays the same (kWaitForMsgStart)
                    continue;
                }
                break;
            case SharpProtocolState::kWaitForStartPulse:
                if (event.value == false && event.time_us < 500) {
                    if (event_code.size() < kCodeEventLength) {
                        event_code.emplace_back(event);
                        cur_state = SharpProtocolState::kWaitForLogicPulse;
                        continue;
                    } else {
                        // Message complete
                        etl::vector<bool, kCodeBitLength> bit_code = SensorEventCodeToBitCode(
                            gsl::span<SensorEvent, kCodeEventLength>(event_code));
                        uint16_t code_val =
                            BitCodeToUint16(gsl::span<bool, kCodeBitLength>(bit_code));
                        printf("Received code: 0x%4X\n", code_val);

                        reset_state_machine();
                        continue;
                    }
                } else {
                    printf("Was expecting start pulse and received: %s %llu us\n",
                           event.value ? "HIGH" : "LOW", event.time_us);
                    printf("Resetting message state machine and discarding current message.\n");
                    reset_state_machine();
                    continue;
                }
                break;
            case SharpProtocolState::kWaitForLogicPulse:
                if (event.value == true && event.time_us > 500 && event.time_us < 2000) {
                    event_code.emplace_back(event);
                    cur_state = SharpProtocolState::kWaitForStartPulse;
                    continue;
                } else {
                    printf("Was expecting logic pulse and received: %s %llu us\n",
                           event.value ? "HIGH" : "LOW", event.time_us);
                    printf("Resetting message state machine and discarding current message.\n");
                    reset_state_machine();
                    continue;
                }
                break;
        }
    }
}

etl::vector<bool, IrSensor::kCodeBitLength> IrSensor::SensorEventCodeToBitCode(
    const gsl::span<SensorEvent, kCodeEventLength> event_code) {
    etl::vector<bool, IrSensor::kCodeBitLength> bit_code{};

    for (int event_index = 0; event_index < event_code.size(); event_index += 2) {
        assert(event_code[event_index].value == false);
        assert(event_code[event_index].time_us < 500);
        assert(event_code[event_index + 1].value == true);
        assert(event_code[event_index + 1].time_us > 500 &&
               event_code[event_index + 1].time_us < 2000);

        bool logic_val = event_code[event_index + 1].time_us > 1000 ? true : false;
        bit_code.emplace_back(logic_val);
    }

    return bit_code;
}

uint16_t IrSensor::BitCodeToUint16(const gsl::span<bool, kCodeBitLength> bit_code) {
    uint16_t val{0};
    for (int bit_index = 0; bit_index < bit_code.size(); ++bit_index) {
        val |= (bit_code[bit_index] ? 1 : 0) << ((bit_code.size() - 1) - bit_index);
    }
    return val;
}

static void IrqHandler(void* args) {
    static uint64_t s_last_time_us = 0;

    bool gpio_val = gpio_get_level(IrSensor::kIrSensorGpioNum) == 1;
    uint64_t cur_time_us = RealTime::GetInstance().GetTimeUs();

    IrSensor::SensorEvent event{.value = !gpio_val,
                                .time_us = RealTime::GetTimeDiffUs(s_last_time_us, cur_time_us)};
    assert(xQueueSendFromISR(s_sensor_queue, &event, nullptr) == pdTRUE);

    s_last_time_us = cur_time_us;
}