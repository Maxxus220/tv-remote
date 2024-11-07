#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "etl/vector.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gsl/gsl"
#include <algorithm>
#include <inttypes.h>
#include <optional>
#include <ranges>
#include <stdio.h>
#include <sys/time.h>

constexpr gpio_num_t kButtonGPIONum = GPIO_NUM_34;
static gptimer_handle_t s_button_timer = nullptr;
static SemaphoreHandle_t s_button_sem = nullptr;

static void IRAM_ATTR button_isr_handler(void *arg) {
  // TODO: Add cooldown
  xSemaphoreGiveFromISR(s_button_sem, nullptr);
}

bool button_timer_isr_handler(gptimer_handle_t timer,
                              const gptimer_alarm_event_data_t *edata,
                              void *user_ctx) {
  return true;
}

static gptimer_handle_t s_bit_bang_timer = nullptr;

constexpr gpio_num_t kIRSensorGPIONum = GPIO_NUM_23;
static QueueHandle_t s_ir_sensor_queue = nullptr;
struct ir_sensor_event {
  int64_t time_us;
  bool val;
};

constexpr gpio_num_t kIRTransmitterGPIONum = GPIO_NUM_22;

static std::optional<bool> convert_to_logic_value(int64_t high_pulse) {
  assert(high_pulse > 0);

  constexpr int64_t kLogicZeroThreshold = 1000;
  constexpr int64_t kLogicOneThreshold = 2000;

  if (high_pulse < kLogicZeroThreshold) {
    return false;
  } else if (high_pulse < kLogicOneThreshold) {
    return true;
  } else {
    return {};
  }
}

static uint16_t convert_to_uint(gsl::span<bool, 15> message) {
  uint16_t val = 0;
  std::ranges::for_each(std::views::enumerate(message), [&val](auto pair) {
    auto [index, bit] = pair;
    val |= static_cast<size_t>(bit) << index;
  });
  return val;
}

static void IRAM_ATTR ir_sensor_isr_handler(void *arg) {
  static int64_t prev_time_us = 0;

  timeval time{};
  gettimeofday(&time, nullptr);

  int64_t cur_time_us = (int64_t)time.tv_sec * 1000000L + (int64_t)time.tv_usec;
  int64_t time_spent_us = cur_time_us - prev_time_us;

  prev_time_us = cur_time_us;

  bool cur_val = gpio_get_level(kIRSensorGPIONum);
  bool val_recorded = !cur_val;

  if (val_recorded) {
    struct ir_sensor_event event = {.time_us = time_spent_us,
                                    .val = val_recorded};
    xQueueSendFromISR(s_ir_sensor_queue, &event, nullptr);
  }
}

extern "C" {
void app_main(void) {
  printf("TV-Remote Booted Beep Beep Boop Boop ~~(^_^)~~\n");
  fflush(stdout);

  s_ir_sensor_queue = xQueueCreate(10, sizeof(ir_sensor_event));
  s_button_sem = xSemaphoreCreateBinary();

  gpio_config_t button_conf{.pin_bit_mask = 1ULL << kButtonGPIONum,
                            .mode = GPIO_MODE_INPUT,
                            .pull_up_en = GPIO_PULLUP_DISABLE,
                            .pull_down_en = GPIO_PULLDOWN_DISABLE,
                            .intr_type = GPIO_INTR_NEGEDGE};
  gpio_config(&button_conf);

  // 1 cycles per us
  gptimer_config_t bit_bang_timer_conf{.clk_src = GPTIMER_CLK_SRC_APB,
                                       .direction = GPTIMER_COUNT_UP,
                                       .resolution_hz = 1000000,
                                       .intr_priority = 0,
                                       .flags = {.intr_shared = 0}};
  gptimer_new_timer(&bit_bang_timer_conf, &s_bit_bang_timer);
  gptimer_set_raw_count(s_bit_bang_timer, 0);

  const gptimer_alarm_config_t bit_bang_timer_alarm_conf{
      .alarm_count = 5,
      .reload_count = 5,
      .flags{.auto_reload_on_alarm = 1},
  };
  gptimer_set_alarm_action(s_bit_bang_timer, &bit_bang_timer_alarm_conf);

  // 1 cycles per us
  gptimer_config_t button_timer_conf{.clk_src = GPTIMER_CLK_SRC_APB,
                                     .direction = GPTIMER_COUNT_UP,
                                     .resolution_hz = 1000000,
                                     .intr_priority = 0,
                                     .flags = {.intr_shared = 0}};
  gptimer_new_timer(&button_timer_conf, &s_button_timer);
  gptimer_set_raw_count(s_button_timer, 0);
  gptimer_event_callbacks_t button_timer_callbacks{
      .on_alarm = button_timer_isr_handler};
  gptimer_register_event_callbacks(s_button_timer, &button_timer_callbacks,
                                   nullptr);

  gpio_config_t ir_sensor_conf{.pin_bit_mask = 1ULL << kIRSensorGPIONum,
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_DISABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type = GPIO_INTR_ANYEDGE};
  gpio_config(&ir_sensor_conf);

  gpio_config_t ir_transmitter_conf{.pin_bit_mask = 1ULL
                                                    << kIRTransmitterGPIONum,
                                    .mode = GPIO_MODE_OUTPUT,
                                    .pull_up_en = GPIO_PULLUP_DISABLE,
                                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                    .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&ir_transmitter_conf);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(kIRSensorGPIONum, ir_sensor_isr_handler, nullptr);
  gpio_isr_handler_add(kButtonGPIONum, button_isr_handler, nullptr);

  bool val = gpio_get_level(kIRSensorGPIONum);
  printf("IR Sensor starts %s\n", val ? "HIGH" : "LOW");

  etl::vector<bool, 15> message{};
  while (true) {
    struct ir_sensor_event event = {};
    if (xQueueReceive(s_ir_sensor_queue, &event, 10 / portTICK_PERIOD_MS) ==
        pdTRUE) {
      if (event.val) {
        if (message.full()) {
          uint16_t val = convert_to_uint(
              gsl::span<bool, 15>(message.data(), message.size()));
          printf("Message: %x\n", val);
          message.clear();
        }
        auto logic_value = convert_to_logic_value(event.time_us);
        if (logic_value.has_value()) {
          message.emplace_back(convert_to_logic_value(event.time_us).value());
        }
      }
    }

    if (xSemaphoreTake(s_button_sem, 10 / portTICK_PERIOD_MS) == pdTRUE) {
      // Send 0x3ee1 0x3ee1 0x4101
      printf("Hello button!\n");
    }
  }
}
}