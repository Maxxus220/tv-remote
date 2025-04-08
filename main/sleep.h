#pragma once

#include "driver/gptimer.h"

class Sleep {
   protected:
    Sleep() = default;

   public:
    ~Sleep() = default;

    static Sleep& GetInstance() {
        static Sleep instance;
        return instance;
    }

    void Setup();
    void Enable();
    void Reset();

   private:
    gptimer_handle_t sleep_timer_;
};