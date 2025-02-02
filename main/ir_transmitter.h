#pragma once

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "gsl/gsl"

class IrTransmitter {
   protected:
    IrTransmitter() = default;
    ~IrTransmitter() = default;

   public:
    static IrTransmitter& GetInstance() {
        static IrTransmitter s_instance{};
        return s_instance;
    }

    void Init();
    void SendCode(uint16_t code);
    void TxThread();

   private:
    static constexpr gpio_num_t kIrTransmitterPwmGpioNum = GPIO_NUM_23;

    gptimer_handle_t transmit_timer_{};

    TaskHandle_t ir_transmit_thread_{};
    QueueHandle_t ir_transmit_queue_ = nullptr;
};