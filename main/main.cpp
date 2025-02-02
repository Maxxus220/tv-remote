#include <stdio.h>

#include "driver/gpio.h"

#include "button.h"
#include "ir_sensor.h"
#include "ir_transmitter.h"
#include "led.h"
#include "real_time.h"

extern "C" {
void app_main(void) {
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    gpio_install_isr_service(0);

    RealTime& real_time = RealTime::GetInstance();
    IrTransmitter& ir_transmitter = IrTransmitter::GetInstance();
    IrSensor& ir_sensor = IrSensor::GetInstance();
    Button0& button_0 = Button0::GetInstance();
    Led0& led_0 = Led0::GetInstance();
    Led1& led_1 = Led1::GetInstance();
    Led2& led_2 = Led2::GetInstance();

    real_time.Init();
    ir_transmitter.Init();
    // ir_sensor.Init();
    button_0.Init();
    led_0.Init();
    led_1.Init();
    led_2.Init();

    while (true) {
        // led_0.Set(1);
        // led_1.Set(1);
        // led_2.Set(1);
        // sleep(1);
        // led_0.Set(0);
        // led_1.Set(0);
        // led_2.Set(0);
        // sleep(1);
    }
}
}