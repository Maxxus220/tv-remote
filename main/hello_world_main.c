#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define IR_SENSOR_GPIO_NUM 23
static QueueHandle_t s_ir_sensor_queue = NULL;
struct ir_sensor_event {
    int64_t time_us;
    bool val;
};

#define MAX_MS 2000
#define PULSE_RESOLUTION 1
struct ir_pulse {
    uint16_t high_ms;
    uint16_t low_ms;
};

#define IR_TRANSMITTER_GPIO_NUM 22

static void IRAM_ATTR ir_sensor_isr_handler(void* arg)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    int64_t time_us = (int64_t)time.tv_sec * 1000000L + (int64_t)time.tv_usec;

    bool val = gpio_get_level(IR_SENSOR_GPIO_NUM);

    struct ir_sensor_event event = {
        .time_us = time_us,
        .val = val
    };
    xQueueSendFromISR(s_ir_sensor_queue, &event, NULL);
}
    
void send_ir_blast()
{
    uint16_t pulses[35] = {
        150,
        60,
        60,
        60,
        60,
        60,
        60,
        0,
        30,
        60,
        0,
        1950,
        150,
        60,
        60,
        60,
        60,
        150,
        60,
        90,
        120,
        60,
        0,
        30,
        1700,
        150,
        60,
        60,
        60,
        60,
        60,
        60,
        0,
        30,
        60,
        0
    };

    gpio_set_level(IR_TRANSMITTER_GPIO_NUM, 1);
    vTaskDelay(20 / portTICK_PERIOD_MS);

    for(int i = 0; i < sizeof(pulses) / sizeof(uint16_t); i++)
    {
        gpio_set_level(IR_TRANSMITTER_GPIO_NUM, 0);
        vTaskDelay(pulses[i] / portTICK_PERIOD_MS);

        gpio_set_level(IR_TRANSMITTER_GPIO_NUM, 1);
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    gpio_set_level(IR_TRANSMITTER_GPIO_NUM, 0);
}

void app_main(void)
{
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    s_ir_sensor_queue = xQueueCreate(10, sizeof(struct ir_sensor_event));

    gpio_config_t ir_sensor_conf = {};
    ir_sensor_conf.pin_bit_mask = 1 << IR_SENSOR_GPIO_NUM;
    ir_sensor_conf.mode = GPIO_MODE_INPUT;
    ir_sensor_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&ir_sensor_conf);

    gpio_config_t ir_transmitter_conf = {};
    ir_transmitter_conf.pin_bit_mask = 1 << IR_TRANSMITTER_GPIO_NUM;
    ir_transmitter_conf.mode = GPIO_MODE_OUTPUT;
    ir_transmitter_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&ir_transmitter_conf);

    // gpio_install_isr_service(0);
    // gpio_isr_handler_add(IR_SENSOR_GPIO_NUM, ir_sensor_isr_handler, NULL);

    bool val = gpio_get_level(IR_SENSOR_GPIO_NUM);
    printf("IR Sensor starts %s\n", val ? "HIGH" : "LOW");

    while(true)
    {
        // send_ir_blast();
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
        struct ir_pulse pulse = {};
        while(gpio_get_level(IR_SENSOR_GPIO_NUM) && pulse.high_ms <= MAX_MS)
        {
            pulse.high_ms += PULSE_RESOLUTION;
            vTaskDelay(PULSE_RESOLUTION / portTICK_PERIOD_MS);
        }

        while(!gpio_get_level(IR_SENSOR_GPIO_NUM) && pulse.low_ms <= MAX_MS)
        {
            pulse.low_ms += PULSE_RESOLUTION;
            vTaskDelay(PULSE_RESOLUTION / portTICK_PERIOD_MS);
        }

        if(pulse.high_ms <= MAX_MS && pulse.low_ms <= MAX_MS)
        {
            printf("HIGH: %8d | LOW: %8d\n", pulse.high_ms, pulse.low_ms);
            fflush(stdout);
        }
        //struct ir_sensor_event event = {};
        //if(xQueueReceive(s_ir_sensor_queue, &event, portMAX_DELAY) == pdTRUE)
        //{
        //    printf("IR IN: %4s, %lldus\n", event.val ? "HIGH" : "LOW", event.time_us);
        //}
    }
}
