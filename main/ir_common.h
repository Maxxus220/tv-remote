#pragma once

#include <optional>
#include "etl/vector.h"
#include "gsl/gsl"

namespace ir_common {

enum class IrValue { kLow, kHigh };

struct IrEvent {
    IrValue value;       // IR high or low.
    uint64_t time_us;    // Time in us that IR was in that state.
};

enum class IrEventType { kStartCode, kLogic0, kLogic1, kMsgStart };

constexpr size_t kCodeBitLength = 15;
constexpr size_t kCodeEventLength = kCodeBitLength * 2;

/**
 * Start Code:    High <500us
 * Logic 0 Value: Low >=500 && <1000us
 * Logic 1 Value: Low >=1000 && <2000us
 * Msg Start:     Low >=2000us
 */
constexpr uint64_t kStartCodeMaxUs = 500;
constexpr uint64_t kLogic0MinUs = 500;
constexpr uint64_t kLogic0MaxUs = 1000;
constexpr uint64_t kLogic1MinUs = 1000;
constexpr uint64_t kLogic1MaxUs = 2000;
constexpr uint64_t kMsgStartMinUs = 2000;

const char* IrValueToString(IrValue value);

std::optional<IrEventType> EventToEventType(IrEvent event);

etl::vector<bool, kCodeBitLength> GpioEventCodeToBitCode(
    const gsl::span<IrEvent, kCodeEventLength> event_code);
etl::vector<IrEvent, kCodeEventLength> BitCodeToGpioEventCode(
    const gsl::span<bool, kCodeBitLength> bit_code);

uint16_t BitCodeToUint16(const gsl::span<bool, kCodeBitLength> bit_code);
etl::vector<bool, kCodeBitLength> Uint16ToBitCode(uint16_t code);

};    // namespace ir_common