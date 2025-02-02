#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef void (*ButtonPressIrqCb)(void);

void ButtonIsrHandler(void* args);

class IButtonIrqHandle {
   public:
    virtual ~IButtonIrqHandle() = default;
    virtual ButtonPressIrqCb GetButtonPressIrqCb() = 0;
};

template <gpio_num_t kGpioNum>
class Button : public IButtonIrqHandle {
   protected:
    Button() = default;
    ~Button() = default;

   public:
    static Button& GetInstance() {
        static Button instance{};
        return instance;
    }

    void Init();
    void RegisterButtonPressIrqCb(ButtonPressIrqCb cb);
    ButtonPressIrqCb GetButtonPressIrqCb() final;

   private:
    TaskHandle_t button_pressed_thread_{};
    ButtonPressIrqCb button_press_cb_;
};

template <gpio_num_t kGpioNum>
void Button<kGpioNum>::Init() {
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

constexpr gpio_num_t kButton0GpioNum = GPIO_NUM_34;

using Button0 = Button<kButton0GpioNum>;