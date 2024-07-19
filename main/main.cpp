#include <optional>
#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

constexpr gpio_num_t kIRSensorGPIONum = GPIO_NUM_23;
static QueueHandle_t s_ir_sensor_queue = NULL;
struct ir_sensor_event
{
    int64_t time_us;
    bool val;
};

constexpr gpio_num_t kIRTransmitterGPIONum = GPIO_NUM_22;

static std::optional<bool> convert_to_logic_value(int64_t high_pulse)
{
    assert(high_pulse > 0);

    constexpr int64_t kLogicZeroThreshold = 1000;
    constexpr int64_t kLogicOneThreshold = 2000;

    if (high_pulse < kLogicZeroThreshold)
    {
        return false;
    }
    else if (high_pulse < kLogicOneThreshold)
    {
        return true;
    }
    else
    {
        return {};
    }
}

static void IRAM_ATTR ir_sensor_isr_handler(void *arg)
{
    static int64_t prev_time_us = 0;

    timeval time{};
    gettimeofday(&time, NULL);

    int64_t cur_time_us = (int64_t)time.tv_sec * 1000000L + (int64_t)time.tv_usec;
    int64_t time_spent_us = cur_time_us - prev_time_us;

    prev_time_us = cur_time_us;

    bool cur_val = gpio_get_level(kIRSensorGPIONum);
    bool val_recorded = !cur_val;

    if (val_recorded)
    {
        struct ir_sensor_event event = {
            .time_us = time_spent_us,
            .val = val_recorded};
        xQueueSendFromISR(s_ir_sensor_queue, &event, NULL);
    }
}

extern "C"
{
    void app_main(void)
    {
        printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
        fflush(stdout);

        s_ir_sensor_queue = xQueueCreate(10, sizeof(ir_sensor_event));

        gpio_config_t ir_sensor_conf = {};
        ir_sensor_conf.pin_bit_mask = 1 << kIRSensorGPIONum;
        ir_sensor_conf.mode = GPIO_MODE_INPUT;
        ir_sensor_conf.intr_type = GPIO_INTR_ANYEDGE;
        gpio_config(&ir_sensor_conf);

        gpio_config_t ir_transmitter_conf = {};
        ir_transmitter_conf.pin_bit_mask = 1 << kIRTransmitterGPIONum;
        ir_transmitter_conf.mode = GPIO_MODE_OUTPUT;
        ir_transmitter_conf.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&ir_transmitter_conf);

        gpio_install_isr_service(0);
        gpio_isr_handler_add(kIRSensorGPIONum, ir_sensor_isr_handler, NULL);

        bool val = gpio_get_level(kIRSensorGPIONum);
        printf("IR Sensor starts %s\n", val ? "HIGH" : "LOW");

        while (true)
        {
            struct ir_sensor_event event = {};
            if (xQueueReceive(s_ir_sensor_queue, &event, portMAX_DELAY) == pdTRUE)
            {
                printf("IR IN: %4s, %lldus\n", event.val ? "HIGH" : "LOW", event.time_us);
            }
        }
    }
}