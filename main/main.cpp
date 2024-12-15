#include <inttypes.h>
#include <stdio.h>
#include <sys/time.h>
#include <algorithm>
#include <optional>
#include <ranges>
#include "button.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "etl/vector.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gsl/gsl"
#include "ir_sensor.h"
#include "real_time.h"

static gptimer_handle_t s_button_timer = nullptr;

bool button_timer_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata,
                              void* user_ctx) {
    return true;
}

static gptimer_handle_t s_bit_bang_timer = nullptr;

struct ir_sensor_event {
    int64_t time_us;
    bool val;
};

constexpr gpio_num_t kIRTransmitterGPIONum = GPIO_NUM_22;

extern "C" {
void app_main(void) {
    printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
    fflush(stdout);

    // 1 cycles per us
    gptimer_config_t bit_bang_timer_conf{.clk_src = GPTIMER_CLK_SRC_APB,
                                         .direction = GPTIMER_COUNT_UP,
                                         .resolution_hz = 1000000,
                                         .intr_priority = 0,
                                         .flags = {.intr_shared = 0, .backup_before_sleep = 0}};
    gptimer_new_timer(&bit_bang_timer_conf, &s_bit_bang_timer);
    gptimer_set_raw_count(s_bit_bang_timer, 0);

    const gptimer_alarm_config_t bit_bang_timer_alarm_conf{
        .alarm_count = 5,
        .reload_count = 5,
        .flags{.auto_reload_on_alarm = 1},
    };
    gptimer_set_alarm_action(s_bit_bang_timer, &bit_bang_timer_alarm_conf);

    // 1 cycles per us
    gptimer_config_t button_timer_conf{.clk_src = GPTIMER_CLK_SRC_APB,
                                       .direction = GPTIMER_COUNT_UP,
                                       .resolution_hz = 1000000,
                                       .intr_priority = 0,
                                       .flags = {.intr_shared = 0, .backup_before_sleep = 0}};
    gptimer_new_timer(&button_timer_conf, &s_button_timer);
    gptimer_set_raw_count(s_button_timer, 0);
    gptimer_event_callbacks_t button_timer_callbacks{.on_alarm = button_timer_isr_handler};
    gptimer_register_event_callbacks(s_button_timer, &button_timer_callbacks, nullptr);

    gpio_config_t ir_transmitter_conf{.pin_bit_mask = 1ULL << kIRTransmitterGPIONum,
                                      .mode = GPIO_MODE_OUTPUT,
                                      .pull_up_en = GPIO_PULLUP_DISABLE,
                                      .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                      .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&ir_transmitter_conf);

    gpio_install_isr_service(0);
    IrSensor::GetInstance().Init();
    RealTime::GetInstance().Init();
    Button::GetInstance().Init();

    while (true) {}
}
}