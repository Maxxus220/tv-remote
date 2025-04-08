#include "button_handler.h"

#include "button.h"
#include "sleep.h"

#include "esp_sleep.h"

#include <optional>

static constexpr uint8_t kButton0PressNum = 0;
static constexpr uint8_t kButton1PressNum = 1;
static constexpr uint8_t kButton2PressNum = 2;
static constexpr uint8_t kButton3PressNum = 3;

static QueueHandle_t s_button_press_queue;
static void ButtonPress0IrqCb();
static void ButtonPress1IrqCb();
static void ButtonPress2IrqCb();
static void ButtonPress3IrqCb();

static void CHandleButtonPressThread(void* args);

void ButtonHandler::Init() {
    s_button_press_queue = xQueueCreate(kButtonPressQueueLength, sizeof(uint8_t));

    Button0::GetInstance().RegisterButtonPressIrqCb(ButtonPress0IrqCb);
    Button1::GetInstance().RegisterButtonPressIrqCb(ButtonPress1IrqCb);
    Button2::GetInstance().RegisterButtonPressIrqCb(ButtonPress2IrqCb);
    Button3::GetInstance().RegisterButtonPressIrqCb(ButtonPress3IrqCb);

    led_2_.Set(1);

    constexpr uint32_t kHandleButtonPressThreadStackSize = 5000;
    constexpr UBaseType_t kHandleButtonPressTheadPriority = 3;
    assert(xTaskCreate(CHandleButtonPressThread, "Button Presses",
                       kHandleButtonPressThreadStackSize, static_cast<void*>(this),
                       kHandleButtonPressTheadPriority, &handle_button_press_thread_));
}

void CHandleButtonPressThread(void* args) {
    static_cast<ButtonHandler*>(args)->HandleButtonPressThread();
}

void ButtonHandler::HandleButtonPressThread() {
    Sleep& sleep = Sleep::GetInstance();
    while (true) {
        uint8_t button_press_num{};
        assert(xQueueReceive(s_button_press_queue, &button_press_num, portMAX_DELAY) == pdTRUE);
        sleep.Reset();

        switch (button_press_num) {
            case 0:
                HandleButton0Press();
                break;
            case 1:
                HandleButton1Press();
                break;
            case 2:
                HandleButton2Press();
                break;
            case 3:
                HandleButton3Press();
                break;
            default:
                assert(false);
        }
    }
}

void ButtonHandler::HandleButton0Press() {
    printf("Handling button 0 press!\n");

    key_layer_ = (key_layer_ + 1) % 3;

    led_0_.Set(0);
    led_1_.Set(0);
    led_2_.Set(0);

    switch (key_layer_) {
        case 0:
            led_2_.Set(1);
            break;
        case 1:
            led_1_.Set(1);
            break;
        case 2:
            led_0_.Set(1);
            break;
        default:
            assert(false);
    }
}

void ButtonHandler::HandleButton1Press() {
    printf("Handling button 1 press!\n");

    switch (key_layer_) {
        case 0:
            static constexpr std::array<uint16_t, 2> kVolumeUpCodes{0x40A2, 0x435D};
            ir_transmitter_.SendCode(kVolumeUpCodes[0]);
            ir_transmitter_.SendCode(kVolumeUpCodes[1]);
            break;
        case 1:
            break;
        case 2:
            break;
        default:
            assert(false);
    }
}

void ButtonHandler::HandleButton2Press() {
    printf("Handling button 2 press!\n");

    switch (key_layer_) {
        case 0:
            static constexpr std::array<uint16_t, 2> kVolumeDownCodes{0x42A2, 0x415D};
            ir_transmitter_.SendCode(kVolumeDownCodes[0]);
            ir_transmitter_.SendCode(kVolumeDownCodes[1]);
            break;
        case 1:
            break;
        case 2:
            break;
        default:
            assert(false);
    }
}

void ButtonHandler::HandleButton3Press() {
    printf("Handling button 3 press!\n");

    switch (key_layer_) {
        case 0:
            static constexpr std::array<uint16_t, 2> kPowerCodes{0x41A2, 0x425D};
            ir_transmitter_.SendCode(kPowerCodes[0]);
            ir_transmitter_.SendCode(kPowerCodes[1]);
            break;
        case 1:
            break;
        case 2:
            break;
        default:
            assert(false);
    }
}

void IRAM_ATTR ButtonPress0IrqCb() {
    xQueueSendFromISR(s_button_press_queue, &kButton0PressNum, nullptr);
    // TODO: Add way to do deferred logging so that a failure here can be logged safely.
}

void IRAM_ATTR ButtonPress1IrqCb() {
    xQueueSendFromISR(s_button_press_queue, &kButton1PressNum, nullptr);
}

void IRAM_ATTR ButtonPress2IrqCb() {
    xQueueSendFromISR(s_button_press_queue, &kButton2PressNum, nullptr);
}

void IRAM_ATTR ButtonPress3IrqCb() {
    xQueueSendFromISR(s_button_press_queue, &kButton3PressNum, nullptr);
}
