#include "ir_common.h"

namespace ir_common {

const char* IrValueToString(IrValue value) {
    switch (value) {
        case IrValue::kLow:
            return "LOW";
        case IrValue::kHigh:
            return "HIGH";
        default:
            assert(false);
            return "UNDEFINED";
    }
}

std::optional<IrEventType> EventToEventType(IrEvent event) {
    if (event.value == IrValue::kHigh && event.time_us < kStartCodeMaxUs) {
        return IrEventType::kStartCode;
    } else if (event.value == IrValue::kLow && event.time_us >= kLogic0MinUs &&
               event.time_us < kLogic0MaxUs) {
        return IrEventType::kLogic0;
    } else if (event.value == IrValue::kLow && event.time_us >= kLogic1MinUs &&
               event.time_us < kLogic1MaxUs) {
        return IrEventType::kLogic1;
    } else if (event.value == IrValue::kLow && event.time_us >= kMsgStartMinUs) {
        return IrEventType::kMsgStart;
    } else {
        return std::nullopt;
    }
}

etl::vector<bool, kCodeBitLength> GpioEventCodeToBitCode(
    const gsl::span<IrEvent, kCodeEventLength> event_code) {
    etl::vector<bool, kCodeBitLength> bit_code{};

    for (int event_index = 0; event_index < event_code.size(); event_index += 2) {
        assert(event_code[event_index].value == IrValue::kHigh);
        assert(event_code[event_index].time_us < kStartCodeMaxUs);
        assert(event_code[event_index + 1].value == IrValue::kLow);
        assert(event_code[event_index + 1].time_us > kLogic0MinUs &&
               event_code[event_index + 1].time_us < kLogic1MaxUs);

        bool logic_val = event_code[event_index + 1].time_us > kLogic1MinUs ? true : false;
        bit_code.emplace_back(logic_val);
    }

    return bit_code;
}

etl::vector<IrEvent, kCodeEventLength> BitCodeToGpioEventCode(
    const gsl::span<bool, kCodeBitLength> bit_code) {
    etl::vector<IrEvent, kCodeEventLength> event_code{};
    constexpr IrEvent kStartEvent{.value = IrValue::kHigh, .time_us = 320};
    constexpr IrEvent kLogicEventOne{.value = IrValue::kLow, .time_us = 1680};
    constexpr IrEvent kLogicEventZero{.value = IrValue::kLow, .time_us = 680};
    for (bool bit : bit_code) {
        event_code.emplace_back(kStartEvent);
        const IrEvent& logic_event = bit ? kLogicEventOne : kLogicEventZero;
        event_code.emplace_back(logic_event);
    }

    return event_code;
}

uint16_t BitCodeToUint16(const gsl::span<bool, kCodeBitLength> bit_code) {
    uint16_t val{0};
    for (int bit_index = 0; bit_index < bit_code.size(); ++bit_index) {
        val |= (bit_code[bit_index] ? 1 : 0) << ((bit_code.size() - 1) - bit_index);
    }
    return val;
}

etl::vector<bool, kCodeBitLength> Uint16ToBitCode(uint16_t code) {
    etl::vector<bool, kCodeBitLength> bit_code{};
    for (int bit_index = 14; bit_index >= 0; --bit_index) {
        bit_code.emplace_back((code & (1 << bit_index)) != 0);
    }
    return bit_code;
}

}    // namespace ir_common