#pragma once

#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "gsl/gsl"

class IrTransmitter {
   protected:
    IrTransmitter() = default;
    ~IrTransmitter() = default;

   public:
    static IrTransmitter& GetInstance() {
        static IrTransmitter instance{};
        return instance;
    }

    void Init();
    void SendCode(uint16_t code);
    void IrTransmitThread();

   private:
    static constexpr gpio_num_t kIrTransmitterGpioNum = GPIO_NUM_22;

    gptimer_handle_t transmit_timer_{};
    TaskHandle_t ir_transmit_thread_{};
    QueueHandle_t ir_transmit_queue_ = nullptr;
};