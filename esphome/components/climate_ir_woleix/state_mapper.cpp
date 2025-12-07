#include "esphome/components/climate/climate_mode.h"

#include "state_mapper.h"
#include "woleix_ac_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

using climate::ClimateMode;
using climate::ClimateFanMode;

climate::ClimateMode StateMapper::woleix_to_esphome_mode(WoleixMode mode) {
  switch (mode) {
    case WoleixMode::COOL:
      return ClimateMode::CLIMATE_MODE_COOL;
    case WoleixMode::DEHUM:
      return ClimateMode::CLIMATE_MODE_DRY;
    case WoleixMode::FAN:
      return ClimateMode::CLIMATE_MODE_FAN_ONLY;
    default:
      return ClimateMode::CLIMATE_MODE_COOL;
  }
}

WoleixMode StateMapper::esphome_to_woleix_mode(climate::ClimateMode mode) {
  switch (mode) {
    case ClimateMode::CLIMATE_MODE_COOL:
      return WoleixMode::COOL;
    case ClimateMode::CLIMATE_MODE_DRY:
      return WoleixMode::DEHUM;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:
      return WoleixMode::FAN;
    default:
      return WoleixMode::COOL;
  }
}

climate::ClimateFanMode StateMapper::woleix_to_esphome_fan_mode(WoleixFanSpeed speed) {
  switch (speed) {
    case WoleixFanSpeed::LOW:
      return ClimateFanMode::CLIMATE_FAN_LOW;
    case WoleixFanSpeed::HIGH:
      return ClimateFanMode::CLIMATE_FAN_HIGH;
    default:
      return ClimateFanMode::CLIMATE_FAN_LOW;
  }
}

WoleixFanSpeed StateMapper::esphome_to_woleix_fan_mode(climate::ClimateFanMode mode) {
  switch (mode) {
    case ClimateFanMode::CLIMATE_FAN_LOW:
      return WoleixFanSpeed::LOW;
    case ClimateFanMode::CLIMATE_FAN_HIGH:
      return WoleixFanSpeed::HIGH;
    default:
      return WoleixFanSpeed::LOW;
  }
}

bool StateMapper::woleix_to_esphome_power(WoleixPowerState power) {
  return power == WoleixPowerState::ON;
}

WoleixPowerState StateMapper::esphome_to_woleix_power(bool power) {
  return power ? WoleixPowerState::ON : WoleixPowerState::OFF;
}

}  // namespace climate_ir_woleix
}  // namespace esphome
