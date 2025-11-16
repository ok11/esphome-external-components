#pragma once

#include <optional>

namespace esphome {

// Use std::optional as the basis for ESPHome's optional
template<typename T>
using optional = std::optional<T>;

} // namespace esphome
