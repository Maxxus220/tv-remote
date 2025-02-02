#include "real_time.h"

#include <limits>

void RealTime::Init() {
    constexpr gptimer_config_t timer_config{
        .clk_src = GPTIMER_CLK_SRC_APB,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,    // 1 us per tick
        .intr_priority = 0,
        .flags = {.intr_shared = 0, .allow_pd = 0, .backup_before_sleep = 0}};
    assert(gptimer_new_timer(&timer_config, &gptimer_) == ESP_OK);
    assert(gptimer_set_raw_count(gptimer_, 0) == ESP_OK);
    assert(gptimer_enable(gptimer_) == ESP_OK);
    assert(gptimer_start(gptimer_) == ESP_OK);
}

uint64_t RealTime::GetTimeUs() {
    uint64_t ticks = 0;
    assert(gptimer_get_raw_count(gptimer_, &ticks) == ESP_OK);
    return ticks;
}

uint64_t RealTime::GetTimeDiffUs(uint64_t start, uint64_t end) {
    return end >= start ? end - start : std::numeric_limits<uint64_t>::max() - start + end;
}