#pragma once

#include "driver/gptimer.h"

class RealTime {
   protected:
    RealTime() = default;
    ~RealTime() = default;

   public:
    static RealTime& GetInstance() {
        static RealTime s_instance{};
        return s_instance;
    }

    /**
     * @brief Initializes the real time timer.
     */
    void Init();

    /**
     * @brief Gets the current time in us.
     * 
     * @return Current time in us.
     */
    uint64_t GetTimeUs();

    /**
     * @brief Gets the time difference in us between start and end.
     * 
     * This function doesn't work if end has looped past start.
     * 
     * @param start Start time in us.
     * @param end   End time in us.
     * @return      Difference in us.
     */
    static uint64_t GetTimeDiffUs(uint64_t start, uint64_t end);

   private:
    gptimer_handle_t gptimer_{};
};