#include "button.h"

#include "real_time.h"

static void CButtonPressedThread(void* args);
static void CISRHandler(void* args);

void Button::Init() {
    button_pressed_sem_ = xSemaphoreCreateBinary();
    assert(button_pressed_sem_ != nullptr);

    gpio_config_t button_conf{.pin_bit_mask = 1ULL << kButtonGPIONum,
                              .mode = GPIO_MODE_INPUT,
                              .pull_up_en = GPIO_PULLUP_DISABLE,
                              .pull_down_en = GPIO_PULLDOWN_DISABLE,
                              .intr_type = GPIO_INTR_NEGEDGE};
    assert(gpio_config(&button_conf) == ESP_OK);
    assert(gpio_isr_handler_add(kButtonGPIONum, CButtonPressedThread,
                                static_cast<void*>(this)) == ESP_OK);
    assert(gpio_isr_handler_add(kButtonGPIONum, CISRHandler,
                                static_cast<void*>(this)) == ESP_OK);

    assert(xTaskCreate(CButtonPressedThread, "Button Press", 5000,
                       static_cast<void*>(this), 2,
                       &button_pressed_thread_) == pdPASS);
}

static void CButtonPressedThread(void* args) {
    Button* instance = static_cast<Button*>(args);
    instance->ButtonPressedThread();
}

void Button::ButtonPressedThread() {
    uint64_t last_time_us = 0;
    while (true) {
        assert(xSemaphoreTake(button_pressed_sem_, portMAX_DELAY) == pdTRUE);
        printf("Button pressed!\n");

        uint64_t now_us = RealTime::GetInstance().GetTimeUs();
        uint64_t diff_us = RealTime::GetTimeDiffUs(last_time_us, now_us);
        printf("Time since last button press: %llu us\n", diff_us);

        last_time_us = now_us;
    }
}

static void IRAM_ATTR CISRHandler(void* args) {
    Button* instance = static_cast<Button*>(args);
    instance->ISRHandler();
}

void IRAM_ATTR Button::ISRHandler() {
    // TODO: Add cooldown
    xSemaphoreGiveFromISR(button_pressed_sem_, nullptr);
}