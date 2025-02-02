#pragma once

#include "driver/gpio.h"
#include "etl/vector.h"
#include "freertos/FreeRTOS.h"
#include "gsl/gsl"

class IrSensor {
   protected:
    IrSensor() = default;
    ~IrSensor() = default;

   public:
    static IrSensor& GetInstance() {
        static IrSensor instance{};
        return instance;
    }

    void Init();

    // This needs to be public so that the CSensorEventThread can call in.
    void SensorEventThread();

    static constexpr gpio_num_t kIrSensorGpioNum = GPIO_NUM_22;

   private:
    enum class SharpProtocolState { kWaitForMsgStart, kWaitForStartPulse, kWaitForLogicPulse };
    TaskHandle_t sensor_event_thread_{};
};