#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

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

    static constexpr gpio_num_t kIrSensorGpioNum = GPIO_NUM_23;

   private:
    TaskHandle_t sensor_event_thread_{};
};