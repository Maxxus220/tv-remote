#include "sleep.h"

#include "button.h"
#include "led.h"

#include "esp_sleep.h"

#include <atomic>

std::atomic_bool allow_sleep{false};
std::atomic_bool allow_sleep_set{false};

extern "C" {
void vApplicationIdleHook() {
    // TODO: Async logging
    if (allow_sleep) {
        TickType_t kMaxIdleTimeMs = 15000 / portTICK_PERIOD_MS;    // 15000ms
        vTaskDelay(kMaxIdleTimeMs);

        rtc_gpio_init(kLed0GpioNum);
        rtc_gpio_init(kLed1GpioNum);
        rtc_gpio_init(kLed2GpioNum);
        rtc_gpio_set_direction(kLed0GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_direction(kLed1GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_direction(kLed2GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_direction_in_sleep(kLed0GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_direction_in_sleep(kLed1GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_direction_in_sleep(kLed2GpioNum, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_set_level(kLed0GpioNum, 0);
        rtc_gpio_set_level(kLed1GpioNum, 0);
        rtc_gpio_set_level(kLed2GpioNum, 0);
        rtc_gpio_hold_en(kLed0GpioNum);
        rtc_gpio_hold_en(kLed1GpioNum);
        rtc_gpio_hold_en(kLed2GpioNum);
        // Led0::GetInstance().Set(false);
        // Led1::GetInstance().Set(false);
        // Led2::GetInstance().Set(false);
        esp_deep_sleep_start();
    }
}
};

namespace mcu_sleep {
void SetupSleep() {
    uint64_t gpio_mask = (1ULL << kButton0InverseGpioNum) | (1ULL << kButton1InverseGpioNum) |
                         (1ULL << kButton2InverseGpioNum) | (1ULL << kButton3InverseGpioNum);
    assert(esp_sleep_enable_ext1_wakeup(
               gpio_mask, esp_sleep_ext1_wakeup_mode_t::ESP_EXT1_WAKEUP_ANY_HIGH) == ESP_OK);
}

/**
 * These functions are used to allow threads to control times where no thread may be active, but work is still happening in hardware. 
 * For instance, we may need to write on SPI, but want to use interrupts to prevent unnecessary CPU usage. However, you could have the
 * case where no other thread needs to run while we wait for the interrupt from SPI. This could cause the idle thread to run and kick off
 * a sleep. Using these functions, we can temporarily disable sleep.
 */

void EnableSleep() {
    allow_sleep = true;
    allow_sleep_set = true;
}

void DisableSleep() {
    allow_sleep = false;
    allow_sleep_set = true;
}

/**
 * This function exists to allow the main thread to enable sleep as long as nothing else has manually taken control of it. Without this,
 * you could imagine the following scenario:
 * 
 * - Other thread is initialized.
 * - Other thread disables sleep and makes interrupt based call.
 * - Main runs and enables sleep.
 * - Idle thread is called while interrupt based call has not finished and puts MCU to sleep.
 */
static portMUX_TYPE sleep_if_unused_spinlock = portMUX_INITIALIZER_UNLOCKED;
void EnableSleepIfUnused() {
    // There is a race condition without entering a critical section. allow_sleep_set + allow_sleep needs to be atomic.
    taskENTER_CRITICAL(&sleep_if_unused_spinlock);
    if (!allow_sleep_set) {
        allow_sleep = true;
    }
    taskEXIT_CRITICAL(&sleep_if_unused_spinlock);
}
};    // namespace mcu_sleep