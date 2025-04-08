#include "sleep.h"

#include "button.h"
#include "led.h"

#include "esp_sleep.h"

#include <cassert>

extern "C" {
static bool IRAM_ATTR EnterSleep(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t* event_data, void* user_ctx) {
    (void)timer;
    (void)event_data;
    (void)user_ctx;

    // TODO: Async logging
    Led0::GetInstance().EnterDeepSleep();
    Led1::GetInstance().EnterDeepSleep();
    Led2::GetInstance().EnterDeepSleep();
    esp_deep_sleep_start();

    return false;
}
};

void Sleep::Setup() {
    // Setup wakeup pins
    static constexpr uint64_t kWakeupGpioMask =
        (1ULL << kButton0InverseGpioNum) | (1ULL << kButton1InverseGpioNum) |
        (1ULL << kButton2InverseGpioNum) | (1ULL << kButton3InverseGpioNum);
    assert(esp_sleep_enable_ext1_wakeup(
               kWakeupGpioMask, esp_sleep_ext1_wakeup_mode_t::ESP_EXT1_WAKEUP_ANY_HIGH) == ESP_OK);

    // Setup timer
    static constexpr gptimer_config_t kSleepTimerConfig{
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,    // 1 us per tick
        .intr_priority = 0,
        .flags = {.intr_shared = 0, .backup_before_sleep = 0}};
    assert(gptimer_new_timer(&kSleepTimerConfig, &sleep_timer_) == ESP_OK);

    constexpr gptimer_event_callbacks_t kSleepTimerCallbacksConfig{.on_alarm = EnterSleep};
    assert(gptimer_register_event_callbacks(sleep_timer_, &kSleepTimerCallbacksConfig, nullptr) ==
           ESP_OK);
    assert(gptimer_enable(sleep_timer_) == ESP_OK);
}

void Sleep::Enable() {
    assert(gptimer_set_raw_count(sleep_timer_, 0) == ESP_OK);
    static constexpr const gptimer_alarm_config_t k8SCountdownSleepTimerConfig{
        .alarm_count = 8000000, .reload_count = 0, .flags{.auto_reload_on_alarm = 0}};
    assert(gptimer_set_alarm_action(sleep_timer_, &k8SCountdownSleepTimerConfig) == ESP_OK);
    assert(gptimer_start(sleep_timer_) == ESP_OK);
}

void Sleep::Reset() {
    assert(gptimer_stop(sleep_timer_) == ESP_OK);
    assert(gptimer_set_raw_count(sleep_timer_, 0) == ESP_OK);
    assert(gptimer_start(sleep_timer_) == ESP_OK);
}