#include <stdio.h>

#include "driver/gpio.h"

#include "button.h"
#include "button_handler.h"
#include "ir_sensor.h"
#include "ir_transmitter.h"
#include "led.h"
#include "real_time.h"
#include "sleep.h"

extern "C" {
void app_main(void) {
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    gpio_install_isr_service(0);

    RealTime& real_time = RealTime::GetInstance();
    IrTransmitter& ir_transmitter = IrTransmitter::GetInstance();
    // IrSensor& ir_sensor = IrSensor::GetInstance();
    Button0& button_0 = Button0::GetInstance();
    Button1& button_1 = Button1::GetInstance();
    Button2& button_2 = Button2::GetInstance();
    Button3& button_3 = Button3::GetInstance();
    Led0& led_0 = Led0::GetInstance();
    Led1& led_1 = Led1::GetInstance();
    Led2& led_2 = Led2::GetInstance();
    ButtonHandler& button_handler = ButtonHandler::GetInstance();

    real_time.Init();
    ir_transmitter.Init();
    // ir_sensor.Init();
    button_0.Init();
    button_1.Init();
    button_2.Init();
    button_3.Init();
    led_0.Init();
    led_1.Init();
    led_2.Init();
    button_handler.Init();

    mcu_sleep::SetupSleep();
    // mcu_sleep::EnableSleepIfUnused();

    while (true) {}
}
}