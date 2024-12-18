#include <stdio.h>

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

    while (true) {
        sleep(9999);
    }
}
}