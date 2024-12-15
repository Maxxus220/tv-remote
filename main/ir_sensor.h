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
    struct SensorEvent {
        bool value;          // What state the sensor was in. High or low.
        uint64_t time_us;    // Time in us that sensor was in that state.
    };

    static IrSensor& GetInstance() {
        static IrSensor instance{};
        return instance;
    }

    void Init();

    // This needs to be public so that the CSensorEventThread can call in.
    void SensorEventThread();

    static constexpr gpio_num_t kIrSensorGpioNum = GPIO_NUM_23;

   private:
    enum class SharpProtocolState { kWaitForMsgStart, kWaitForStartPulse, kWaitForLogicPulse };

    static constexpr size_t kCodeBitLength = 15;
    static constexpr size_t kCodeEventLength = kCodeBitLength * 2;

    static etl::vector<bool, kCodeBitLength> SensorEventCodeToBitCode(
        const gsl::span<SensorEvent, kCodeEventLength> event_code);
    static uint16_t BitCodeToUint16(const gsl::span<bool, kCodeBitLength> bit_code);

    TaskHandle_t sensor_event_thread_{};
};