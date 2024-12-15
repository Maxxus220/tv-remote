#pragma once

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

   private:
    TaskHandle_t button_pressed_thread_{};
};