#pragma once

#include "etl/vector.h"
#include "gsl/gsl"

namespace IrCommon {

enum class IrValue { kLow, kHigh };

struct IrEvent {
    IrValue value;       // IR high or low.
    uint64_t time_us;    // Time in us that IR was in that state.
};

constexpr size_t kCodeBitLength = 15;
constexpr size_t kCodeEventLength = kCodeBitLength * 2;

const char* IrValueToString(IrValue value);

etl::vector<bool, kCodeBitLength> GpioEventCodeToBitCode(
    const gsl::span<IrEvent, kCodeEventLength> event_code);
etl::vector<IrEvent, kCodeEventLength> BitCodeToGpioEventCode(
    const gsl::span<bool, kCodeBitLength> bit_code);

uint16_t BitCodeToUint16(const gsl::span<bool, kCodeBitLength> bit_code);
etl::vector<bool, kCodeBitLength> Uint16ToBitCode(uint16_t code);

};    // namespace IrCommon