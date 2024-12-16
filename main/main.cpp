#include <stdio.h>

#include "button.h"
#include "ir_sensor.h"
#include "ir_transmitter.h"
#include "real_time.h"

extern "C" {
void app_main(void) {
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    // const gptimer_alarm_config_t bit_bang_timer_alarm_conf{
    //     .alarm_count = 5,
    //     .reload_count = 5,
    //     .flags{.auto_reload_on_alarm = 1},
    // };
    // gptimer_set_alarm_action(s_bit_bang_timer, &bit_bang_timer_alarm_conf);

    gpio_install_isr_service(0);
    RealTime::GetInstance().Init();
    IrSensor::GetInstance().Init();
    IrTransmitter::GetInstance().Init();
    Button::GetInstance().Init();

    while (true) {}
}
}