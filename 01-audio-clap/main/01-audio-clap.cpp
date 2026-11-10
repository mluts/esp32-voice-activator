#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h" // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "status_led.h"
#include <cinttypes>
#include <iterator>

namespace {
constexpr const char *TAG = "app";
constexpr uint32_t BYTES_PER_MB = 1024 * 1024;

constexpr status_led::Color HEARTBEAT_COLORS[] = {{255, 0, 0},

                                                  {0, 255, 0},

                                                  {0, 0, 255}};

const char *chip_model_name(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32S3:
    return "ESPS3-S3";
  default:
    return "unknown";
  }
}

void log_chip_info() {

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  ESP_LOGI(TAG, "Chip: %s (model %d), revision v%d.%d, %d core(s)",
           chip_model_name(chip_info.model), chip_info.model,
           chip_info.revision / 100, chip_info.revision % 100, chip_info.cores);
}

void log_flash_size() {

  uint32_t configured_size = 0;
  esp_err_t err = esp_flash_get_size(nullptr, &configured_size);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to read configured flash size: %s",
             esp_err_to_name(err));
    return;
  }

  uint32_t physical_size = 0;
  err = esp_flash_get_physical_size(nullptr, &physical_size);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Flash: configured %" PRIu32 " MB",
             configured_size / BYTES_PER_MB);
    ESP_LOGW(TAG, "Failed to read physical flash size: %s",
             esp_err_to_name(err));
    return;
  }

  ESP_LOGI(TAG, "Flash: configured %" PRIu32 " MB, physical %" PRIu32 " MB",
           configured_size / BYTES_PER_MB, physical_size / BYTES_PER_MB);

  if (configured_size != physical_size) {
    ESP_LOGW(TAG,
             "Flash size mismatch: configured %" PRIu32 " MB, physical %" PRIu32
             " MB",
             configured_size / BYTES_PER_MB, physical_size / BYTES_PER_MB);
  }
}

} // namespace

extern "C" void app_main(void) {
  log_chip_info();
  log_flash_size();

  status_led::init();

  for (uint32_t count = 0;; count++) {
    const status_led::Color &color =
        HEARTBEAT_COLORS[count % std::size(HEARTBEAT_COLORS)];
    status_led::set_color(color);
    ESP_LOGI(TAG, "Heartbeat %" PRIu32, count);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // for (auto i = 10; i >= 0; i--) {
  //   printf("Restarting in %d seconds...\n", i);
  //   vTaskDelay(1000 / portTICK_PERIOD_MS);
  // }
  // esp_restart();
}
