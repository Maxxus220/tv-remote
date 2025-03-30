#include "button.h"

#include <gsl/gsl>

#include "real_time.h"

void IRAM_ATTR ButtonIsrHandler(void* args) {
    gsl::not_null<IButtonIrqHandle*> irq_handle = static_cast<IButtonIrqHandle*>(args);

    // TODO: Update debounce to account for false positive on release.
    uint64_t current_time = RealTime::GetInstance().GetTimeUs();
    constexpr uint64_t kDebounceTimeUs = 150000;    // 150ms
    if (RealTime::GetTimeDiffUs(irq_handle->GetLastPressTimeUs(), current_time) > kDebounceTimeUs) {
        irq_handle->GetButtonPressIrqCb()();
        irq_handle->SetLastPressTimeUs(current_time);
    }
}