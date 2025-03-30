#pragma once

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "freertos/FreeRTOS.h"

typedef void (*ButtonPressIrqCb)(void);

void ButtonIsrHandler(void* args);

class IButtonIrqHandle {
   public:
    virtual ~IButtonIrqHandle() = default;
    virtual ButtonPressIrqCb GetButtonPressIrqCb() = 0;
    virtual void SetLastPressTimeUs(uint64_t time_us) = 0;
    virtual uint64_t GetLastPressTimeUs() = 0;
};

template <gpio_num_t kGpioNum>
class Button : public IButtonIrqHandle {
   protected:
    Button() = default;
    ~Button() = default;

   public:
    static Button& GetInstance() {
        static Button s_instance{};
        return s_instance;
    }

    void Init();
    void RegisterButtonPressIrqCb(ButtonPressIrqCb cb);
    ButtonPressIrqCb GetButtonPressIrqCb() final;
    void SetLastPressTimeUs(uint64_t time_us) final;
    uint64_t GetLastPressTimeUs() final;

   private:
    TaskHandle_t button_pressed_thread_{};
    ButtonPressIrqCb button_press_cb_{};

    uint64_t last_press_time_us_{0};
};

template <gpio_num_t kGpioNum>
void Button<kGpioNum>::Init() {
    // This is required to reinitialize the GPIO as digital after a deep sleep causing them to become RTC IO
    assert(rtc_gpio_deinit(kGpioNum) == ESP_OK);

    constexpr gpio_config_t kButtonConf{.pin_bit_mask = 1ULL << kGpioNum,
                                        .mode = GPIO_MODE_INPUT,
                                        .pull_up_en = GPIO_PULLUP_DISABLE,
                                        .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                        .intr_type = GPIO_INTR_NEGEDGE};
    assert(gpio_config(&kButtonConf) == ESP_OK);
    assert(gpio_isr_handler_add(kGpioNum, ButtonIsrHandler, static_cast<void*>(this)) == ESP_OK);
}

template <gpio_num_t kGpioNum>
void Button<kGpioNum>::RegisterButtonPressIrqCb(ButtonPressIrqCb cb) {
    button_press_cb_ = cb;
}

template <gpio_num_t kGpioNum>
ButtonPressIrqCb Button<kGpioNum>::GetButtonPressIrqCb() {
    return button_press_cb_;
}

template <gpio_num_t kGpioNum>
void Button<kGpioNum>::SetLastPressTimeUs(uint64_t time_us) {
    last_press_time_us_ = time_us;
}

template <gpio_num_t kGpioNum>
uint64_t Button<kGpioNum>::GetLastPressTimeUs() {
    return last_press_time_us_;
}

constexpr gpio_num_t kButton0GpioNum = GPIO_NUM_34;
constexpr gpio_num_t kButton1GpioNum = GPIO_NUM_35;
constexpr gpio_num_t kButton2GpioNum = GPIO_NUM_32;
constexpr gpio_num_t kButton3GpioNum = GPIO_NUM_33;

using Button0 = Button<kButton0GpioNum>;
using Button1 = Button<kButton1GpioNum>;
using Button2 = Button<kButton2GpioNum>;
using Button3 = Button<kButton3GpioNum>;