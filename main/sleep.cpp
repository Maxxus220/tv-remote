#include "sleep.h"

#include "button.h"

#include "esp_sleep.h"

#include <atomic>

std::atomic_bool allow_sleep{false};
std::atomic_bool allow_sleep_set{false};

extern "C" {
void vApplicationIdleHook() {
    // TODO: Async logging
    if (allow_sleep) {
        esp_deep_sleep_start();
    }
}
};

namespace mcu_sleep {
void SetupSleep() {
    uint64_t gpio_mask = (1ULL << kButton0GpioNum) | (1ULL << kButton1GpioNum) |
                         (1ULL << kButton2GpioNum) | (1ULL << kButton3GpioNum);
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