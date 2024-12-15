#include "ir_sensor.h"

#include "real_time.h"

struct SensorEvent {
    bool value;          // What state the sensor was in. High or low.
    uint64_t time_us;    // Time in us that sensor was in that state.
};

static void CSensorEventThread(void* args);
static void IRAM_ATTR IrqHandler(void* args);

static QueueHandle_t s_sensor_queue = nullptr;

void IrSensor::Init() {
    constexpr int kSensorQueueSize = 100;
    s_sensor_queue = xQueueCreate(kSensorQueueSize, sizeof(SensorEvent));

    constexpr gpio_config_t kSensorConf{
        .pin_bit_mask = 1ULL << kIrSensorGpioNum,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    assert(gpio_config(&kSensorConf) == ESP_OK);
    assert(gpio_isr_handler_add(kIrSensorGpioNum, IrqHandler, nullptr) ==
           ESP_OK);

    constexpr uint32_t kSensorEventThreadStackSize = 5000;
    constexpr UBaseType_t kSensorEventThreadPriority = 2;
    assert(xTaskCreate(CSensorEventThread, "Sensor Event",
                       kSensorEventThreadStackSize, static_cast<void*>(this),
                       kSensorEventThreadPriority,
                       &sensor_event_thread_) == pdPASS);
}

static void CSensorEventThread(void* args) {
    IrSensor* instance = static_cast<IrSensor*>(args);
    instance->SensorEventThread();
}

void IrSensor::SensorEventThread() {

    SensorEvent event{};
    while (true) {
        assert(xQueueReceive(s_sensor_queue, &event, portMAX_DELAY) == pdTRUE);
        printf("| Sensor Event | %s | %10llu us |\n",
               event.value ? "HIGH" : " LOW", event.time_us);
    }
}

static void IrqHandler(void* args) {
    static uint64_t s_last_time_us = 0;

    bool gpio_val = gpio_get_level(IrSensor::kIrSensorGpioNum) == 1;
    uint64_t cur_time_us = RealTime::GetInstance().GetTimeUs();

    SensorEvent event{
        .value = !gpio_val,
        .time_us = RealTime::GetTimeDiffUs(s_last_time_us, cur_time_us)};
    assert(xQueueSendFromISR(s_sensor_queue, &event, nullptr) == pdTRUE);

    s_last_time_us = cur_time_us;
}