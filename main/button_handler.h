#pragma once

#include "freertos/FreeRTOS.h"

#include "ir_transmitter.h"
#include "led.h"


class ButtonHandler {
   protected:
    ButtonHandler() = default;
    ~ButtonHandler() = default;

   public:
    static ButtonHandler& GetInstance() {
        static ButtonHandler s_instance{};
        return s_instance;
    }

    void Init();
    void HandleButtonPressThread();
    void HandleButton0Press();
    void HandleButton1Press();
    void HandleButton2Press();
    void HandleButton3Press();

   private:
    static constexpr size_t kButtonPressQueueLength = 5;

    TaskHandle_t handle_button_press_thread_{};

    IrTransmitter& ir_transmitter_{IrTransmitter::GetInstance()};
    Led0& led_0_{Led0::GetInstance()};
    Led1& led_1_{Led1::GetInstance()};
    Led2& led_2_{Led2::GetInstance()};
    uint8_t key_layer_{0};
};