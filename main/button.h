#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

class Button {
   protected:
    Button() = default;
    ~Button() = default;

   public:
    static Button& GetInstance() {
        static Button instance{};
        return instance;
    }

    void Init();

    // Need to be public for C to C++ reasons
    void ButtonPressedThread();
    void ISRHandler();

   private:
    static constexpr gpio_num_t kButtonGPIONum = GPIO_NUM_34;

    SemaphoreHandle_t button_pressed_sem_{nullptr};
    TaskHandle_t button_pressed_thread_{};
};