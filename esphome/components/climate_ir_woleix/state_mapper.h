#pragma once

#include "esphome/components/climate/climate_mode.h"
#include "woleix_ac_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

class StateMapper {
 public:
  static climate::ClimateMode woleix_to_esphome_mode(WoleixMode mode);
  static WoleixMode esphome_to_woleix_mode(climate::ClimateMode mode);

  static climate::ClimateFanMode woleix_to_esphome_fan_mode(WoleixFanSpeed speed);
  static WoleixFanSpeed esphome_to_woleix_fan_mode(climate::ClimateFanMode mode);

  static bool woleix_to_esphome_power(WoleixPowerState power);
  static WoleixPowerState esphome_to_woleix_power(bool power);
};

}  // namespace climate_ir_woleix
}  // namespace esphome
