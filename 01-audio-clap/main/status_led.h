#pragma once
#include <cstdint>

namespace status_led {
  void init();
  void set_color(uint8_t red, uint8_t green, uint8_t blue);
  void off();
}
