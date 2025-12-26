#include "esphome/components/climate/climate_mode.h"

#include "woleix_state_mapper.h"
#include "woleix_state_manager.h"

namespace esphome {
namespace climate_ir_woleix {

using climate::ClimateMode;
using climate::ClimateFanMode;

/**
 * Convert Woleix operating mode to ESPHome climate mode.
 * 
 * Maps Woleix-specific modes to their ESPHome equivalents:
 * - COOL → CLIMATE_MODE_COOL
 * - DEHUM → CLIMATE_MODE_DRY
 * - FAN → CLIMATE_MODE_FAN_ONLY
 * 
 * @param mode Woleix operating mode
 * @return Corresponding ESPHome climate mode
 */
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

/**
 * Convert ESPHome climate mode to Woleix operating mode.
 * 
 * Maps ESPHome climate modes to their Woleix equivalents:
 * - CLIMATE_MODE_COOL → COOL
 * - CLIMATE_MODE_DRY → DEHUM
 * - CLIMATE_MODE_FAN_ONLY → FAN
 * 
 * @param mode ESPHome climate mode
 * @return Corresponding Woleix operating mode
 */
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

/**
 * Convert Woleix fan speed to ESPHome fan mode.
 * 
 * Maps Woleix fan speeds to ESPHome fan modes:
 * - LOW → CLIMATE_FAN_LOW
 * - HIGH → CLIMATE_FAN_HIGH
 * 
 * @param speed Woleix fan speed
 * @return Corresponding ESPHome fan mode
 */
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

/**
 * Convert ESPHome fan mode to Woleix fan speed.
 * 
 * Maps ESPHome fan modes to Woleix fan speeds:
 * - CLIMATE_FAN_LOW → LOW
 * - CLIMATE_FAN_HIGH → HIGH
 * 
 * @param mode ESPHome fan mode
 * @return Corresponding Woleix fan speed
 */
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

/**
 * Convert Woleix power state to ESPHome boolean.
 * 
 * @param power Woleix power state
 * @return true for ON, false for OFF
 */
bool StateMapper::woleix_to_esphome_power(WoleixPowerState power) {
    return power == WoleixPowerState::ON;
}

/**
 * Convert ESPHome boolean to Woleix power state.
 * 
 * @param power ESPHome power state (true for on, false for off)
 * @return Corresponding Woleix power state
 */
WoleixPowerState StateMapper::esphome_to_woleix_power(bool power) {
    return power ? WoleixPowerState::ON : WoleixPowerState::OFF;
}

}  // namespace climate_ir_woleix
}  // namespace esphome
