#include "button.h"

#include "driver/gpio.h"

#include "ir_transmitter.h"
#include "real_time.h"

static void CButtonPressedThread(void* args);
static void IRAM_ATTR IsrHandler(void* args);

static SemaphoreHandle_t s_button_pressed_sem_{nullptr};

void Button::Init() {
    s_button_pressed_sem_ = xSemaphoreCreateBinary();
    assert(s_button_pressed_sem_ != nullptr);

    constexpr gpio_num_t kButtonGpioNum = GPIO_NUM_34;
    constexpr gpio_config_t kButtonConf{.pin_bit_mask = 1ULL << kButtonGpioNum,
                                        .mode = GPIO_MODE_INPUT,
                                        .pull_up_en = GPIO_PULLUP_DISABLE,
                                        .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                        .intr_type = GPIO_INTR_NEGEDGE};
    assert(gpio_config(&kButtonConf) == ESP_OK);
    assert(gpio_isr_handler_add(kButtonGpioNum, CButtonPressedThread, static_cast<void*>(this)) ==
           ESP_OK);
    assert(gpio_isr_handler_add(kButtonGpioNum, IsrHandler, static_cast<void*>(this)) == ESP_OK);

    constexpr uint32_t kButtonPressedStackSize = 5000;
    constexpr UBaseType_t kButtonPressedPriority = 2;
    assert(xTaskCreate(CButtonPressedThread, "Button Press", kButtonPressedStackSize,
                       static_cast<void*>(this), kButtonPressedPriority,
                       &button_pressed_thread_) == pdPASS);
}

void CButtonPressedThread(void* args) {
    Button* instance = static_cast<Button*>(args);
    instance->ButtonPressedThread();
}

void Button::ButtonPressedThread() {
    while (true) {
        assert(xSemaphoreTake(s_button_pressed_sem_, portMAX_DELAY) == pdTRUE);
        IrTransmitter::GetInstance().SendCode(0x41A2);
        IrTransmitter::GetInstance().SendCode(0x425D);
    }
}

void IsrHandler(void* args) {
    // TODO: Add cooldown
    xSemaphoreGiveFromISR(s_button_pressed_sem_, nullptr);
}