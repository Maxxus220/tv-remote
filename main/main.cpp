#include <stdio.h>

#include "driver/gpio.h"

#include "button.h"
#include "ir_sensor.h"
#include "ir_transmitter.h"
#include "real_time.h"

extern "C" {
void app_main(void) {
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    gpio_install_isr_service(0);
    RealTime::GetInstance().Init();
    IrTransmitter::GetInstance().Init();
    IrSensor::GetInstance().Init();
    Button::GetInstance().Init();

    constexpr gpio_config_t kTestLedGpioConfig{
        .pin_bit_mask = 1 << GPIO_NUM_5,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    assert(gpio_config(&kTestLedGpioConfig) == ESP_OK);

    while (true) {
        gpio_set_level(GPIO_NUM_5, 1);
        sleep(1);
        gpio_set_level(GPIO_NUM_5, 0);
        sleep(1);
    }
}
}