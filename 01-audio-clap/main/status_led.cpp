#include "status_led.h"
#include "esp_err.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "led_strip_types.h"
#include <algorithm>

namespace {
constexpr int ESP32S3ZERO_LED_GPIO = 21;
constexpr uint32_t LED_RMT_RESOLUTION_HZ = 10 * 1000 * 1000; // 10MHz, default
constexpr uint8_t BRIGHTNESS_CAP = 16;

led_strip_handle_t led_strip = nullptr;
} // namespace

void status_led::init() {
  led_strip_config_t strip_config = {};
  strip_config.strip_gpio_num = ESP32S3ZERO_LED_GPIO;
  strip_config.max_leds = 1;
  strip_config.led_model = LED_MODEL_WS2812;

  led_strip_rmt_config_t rmt_config = {};
  rmt_config.resolution_hz = LED_RMT_RESOLUTION_HZ;

  ESP_ERROR_CHECK(
      led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

  ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

void status_led::set_color(uint8_t red, uint8_t green, uint8_t blue) {
  led_strip_set_pixel(led_strip, 0, std::min(BRIGHTNESS_CAP, red),
                      std::min(BRIGHTNESS_CAP, green),
                      std::min(BRIGHTNESS_CAP, blue));
  led_strip_refresh(led_strip);
}

void status_led::off() { led_strip_clear(led_strip); }
