#pragma once

namespace esphome {
namespace climate {

enum ClimateMode : uint8_t {
  CLIMATE_MODE_OFF = 0,
  CLIMATE_MODE_HEAT_COOL = 1,
  CLIMATE_MODE_COOL = 2,
  CLIMATE_MODE_HEAT = 3,
  CLIMATE_MODE_FAN_ONLY = 4,
  CLIMATE_MODE_DRY = 5,
  CLIMATE_MODE_AUTO = 6,
};

enum ClimateFanMode : uint8_t {
  CLIMATE_FAN_ON = 0,
  CLIMATE_FAN_OFF = 1,
  CLIMATE_FAN_AUTO = 2,
  CLIMATE_FAN_LOW = 3,
  CLIMATE_FAN_MEDIUM = 4,
  CLIMATE_FAN_HIGH = 5,
  CLIMATE_FAN_MIDDLE = 6,
  CLIMATE_FAN_FOCUS = 7,
  CLIMATE_FAN_DIFFUSE = 8,
  CLIMATE_FAN_QUIET = 9,
};

enum ClimateSwingMode : uint8_t {
  CLIMATE_SWING_OFF = 0,
  CLIMATE_SWING_BOTH = 1,
  CLIMATE_SWING_VERTICAL = 2,
  CLIMATE_SWING_HORIZONTAL = 3,
};

// Feature flags
const uint8_t CLIMATE_SUPPORTS_CURRENT_TEMPERATURE = 1 << 0;
const uint8_t CLIMATE_SUPPORTS_CURRENT_HUMIDITY = 1 << 1;
const uint8_t CLIMATE_SUPPORTS_TWO_POINT_TARGET_TEMPERATURE = 1 << 2;

// Mock ClimateTraits
class ClimateTraits {
public:
  void add_feature_flags(uint8_t flags) { feature_flags_ |= flags; }
  void set_visual_min_temperature(float temp) { visual_min_temp_ = temp; }
  void set_visual_max_temperature(float temp) { visual_max_temp_ = temp; }
  void set_visual_temperature_step(float step) { visual_temp_step_ = step; }
  
  uint8_t get_feature_flags() const { return feature_flags_; }
  float get_visual_min_temperature() const { return visual_min_temp_; }
  float get_visual_max_temperature() const { return visual_max_temp_; }
  float get_visual_temperature_step() const { return visual_temp_step_; }

private:
  uint8_t feature_flags_ = 0;
  float visual_min_temp_ = 0;
  float visual_max_temp_ = 0;
  float visual_temp_step_ = 0;
};

} // namespace climate
} // namespace esphome
