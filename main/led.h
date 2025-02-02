#pragma once

#include "driver/gpio.h"

template <gpio_num_t kGpioNum>
class Led {
   protected:
    Led() = default;
    ~Led() = default;

   public:
    static Led& GetInstance() {
        static Led s_instance{};
        return s_instance;
    }

    void Init();
    void Set(bool on);

   private:
    gpio_num_t gpio_num_;
};

template <gpio_num_t kGpioNum>
void Led<kGpioNum>::Init() {
    constexpr gpio_config_t kLedGpioConfig{
        .pin_bit_mask = 1 << kGpioNum,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&kLedGpioConfig);
    gpio_set_level(gpio_num_, 0);
}

template <gpio_num_t kGpioNum>
void Led<kGpioNum>::Set(bool on) {
    gpio_set_level(kGpioNum, on ? 1 : 0);
}

constexpr gpio_num_t kLed0GpioNum = GPIO_NUM_15;
constexpr gpio_num_t kLed1GpioNum = GPIO_NUM_2;
constexpr gpio_num_t kLed2GpioNum = GPIO_NUM_4;

using Led0 = Led<kLed0GpioNum>;
using Led1 = Led<kLed1GpioNum>;
using Led2 = Led<kLed2GpioNum>;
