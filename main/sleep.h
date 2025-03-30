#pragma once

namespace mcu_sleep {
void SetupSleep();
void EnableSleep();
void DisableSleep();
void EnableSleepIfUnused();
};    // namespace mcu_sleep