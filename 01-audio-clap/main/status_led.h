#pragma once
#include <cstdint>

namespace status_led {
struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

void init();
void set_color(const Color &color);
void off();
} // namespace status_led
