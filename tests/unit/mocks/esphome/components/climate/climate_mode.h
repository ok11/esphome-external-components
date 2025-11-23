#pragma once

#include <set>

// Helper function to convert a set of enum values to a bitmask
template<typename EnumType>
uint32_t to_bitmask(const std::set<EnumType>& enum_set) {
  uint32_t bitmask = 0;
  for (const auto& value : enum_set) {
    bitmask |= (1 << static_cast<uint8_t>(value));
  }
  return bitmask;
}

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

enum ClimateFeature : uint32_t {
  // Reporting current temperature is supported
  CLIMATE_SUPPORTS_CURRENT_TEMPERATURE = 1 << 0,
  // Setting two target temperatures is supported (used in conjunction with CLIMATE_MODE_HEAT_COOL)
  CLIMATE_SUPPORTS_TWO_POINT_TARGET_TEMPERATURE = 1 << 1,
  // Single-point mode is NOT supported (UI always displays two handles, setting 'target_temperature' is not supported)
  CLIMATE_REQUIRES_TWO_POINT_TARGET_TEMPERATURE = 1 << 2,
  // Reporting current humidity is supported
  CLIMATE_SUPPORTS_CURRENT_HUMIDITY = 1 << 3,
  // Setting a target humidity is supported
  CLIMATE_SUPPORTS_TARGET_HUMIDITY = 1 << 4,
  // Reporting current climate action is supported
  CLIMATE_SUPPORTS_ACTION = 1 << 5,
};

// Mock ClimateTraits
class ClimateTraits {
public:
  void add_feature_flags(uint8_t flags) { feature_flags_ |= flags; }
  void set_visual_min_temperature(float temp) { visual_min_temp_ = temp; }
  void set_visual_max_temperature(float temp) { visual_max_temp_ = temp; }
  void set_visual_temperature_step(float step) { visual_temp_step_ = step; }
  void set_supported_modes(std::set<uint8_t> modes) { modes_ = to_bitmask<uint8_t>(modes); } 
  void add_supported_mode(uint8_t mode) { modes_ |= 1 << mode; }
  void add_supported_fan_mode(uint8_t mode) { fan_modes_ |= 1 << mode; }
  void set_supported_fan_modes(std::set<uint8_t> modes) { fan_modes_ = to_bitmask<uint8_t>(modes); } 
  void set_supported_swing_modes(std::set<uint8_t> modes) { swing_modes_ = to_bitmask<uint8_t>(modes); } 
  
  uint8_t get_feature_flags() const { return feature_flags_; }
  float get_visual_min_temperature() const { return visual_min_temp_; }
  float get_visual_max_temperature() const { return visual_max_temp_; }
  float get_visual_temperature_step() const { return visual_temp_step_; }

private:
  uint8_t feature_flags_ = 0;
  uint8_t modes_ = 0;
  uint8_t fan_modes_ = 0;
  uint8_t swing_modes_ = 0;
  float visual_min_temp_ = 0;
  float visual_max_temp_ = 0;
  float visual_temp_step_ = 0;
};

} // namespace climate
} // namespace esphome
