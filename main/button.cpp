#include "button.h"

#include <gsl/gsl>

namespace buttons {
void IRAM_ATTR ButtonIsrHandler(void* args) {
    gsl::not_null<IButtonIrqHandle*> irq_handle = static_cast<IButtonIrqHandle*>(args);
    // TODO: Add cooldown
    irq_handle->GetButtonPressIrqCb()();
}
};    // namespace buttons