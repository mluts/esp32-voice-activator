#pragma once
#include "esp_adc/adc_continuous.h"
#include "soc/soc_caps.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>

class MicArray {
public:
  static constexpr uint32_t SAMPLE_RATE_HZ = 16000;
  static constexpr size_t FRAME_SAMPLES = 256;
  static constexpr size_t STORE_FRAMES = 4;

  static constexpr size_t MAX_MICS =
      SOC_ADC_SAMPLE_FREQ_THRES_HIGH / SAMPLE_RATE_HZ;

  using Frame = std::array<int16_t, FRAME_SAMPLES>;

  explicit MicArray(std::initializer_list<int> gpios = {1});

  void start();

  // NOTE: Blocks until one frame is captured and writes one bias-removed
  // frame per microphone, in constructor order; `frames.size()` must equal
  // `size()`.
  void read(std::span<Frame> frames);

  size_t size() const { return mic_count; }

  int raw_bias(size_t mic) const;

  uint32_t overflow_count() const { return overflows.load(); }

  uint32_t unexpected_channel_count() const { return unexpected_channels; }

private:
  struct Mic {
    int gpio;
    adc_channel_t channel;
    float bias;
  };

  static bool on_pool_overflow(adc_continuous_handle_t handle,
                               const adc_continuous_evt_data_t *edata,
                               void *user_data);

  std::array<Mic, MAX_MICS> mics = {};

  size_t mic_count = 0;

  std::array<adc_digi_output_data_t, FRAME_SAMPLES> raw_chunk = {};
  adc_continuous_handle_t handle = nullptr;

  bool bias_seeded = false;

  // Diagnostics
  std::atomic<uint32_t> overflows = 0;
  uint32_t unexpected_channels = 0;
};
