#pragma once

#include "esphome/components/climate/climate_mode.h"

#include "woleix_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

/**
 * @brief Utility class for mapping between ESPHome and Woleix state representations.
 *
 * This class provides static methods to convert between ESPHome climate states
 * and Woleix-specific states for modes, fan speeds, and power states.
 */
class StateMapper {
public:
    /**
     * @brief Convert Woleix mode to ESPHome climate mode.
     * @param mode Woleix operating mode
     * @return Corresponding ESPHome climate mode
     */
    static climate::ClimateMode woleix_to_esphome_mode(WoleixMode mode);

    /**
     * @brief Convert ESPHome climate mode to Woleix mode.
     * @param mode ESPHome climate mode
     * @return Corresponding Woleix operating mode
     */
    static WoleixMode esphome_to_woleix_mode(climate::ClimateMode mode);

    /**
     * @brief Convert Woleix fan speed to ESPHome fan mode.
     * @param speed Woleix fan speed
     * @return Corresponding ESPHome fan mode
     */
    static climate::ClimateFanMode woleix_to_esphome_fan_mode(WoleixFanSpeed speed);

    /**
     * @brief Convert ESPHome fan mode to Woleix fan speed.
     * @param mode ESPHome fan mode
     * @return Corresponding Woleix fan speed
     */
    static WoleixFanSpeed esphome_to_woleix_fan_mode(climate::ClimateFanMode mode);

    /**
     * @brief Convert Woleix power state to ESPHome power state.
     * @param power Woleix power state
     * @return Corresponding ESPHome power state (true for ON, false for OFF)
     */
    static bool woleix_to_esphome_power(WoleixPowerState power);

    /**
     * @brief Convert ESPHome power state to Woleix power state.
     * @param power ESPHome power state (true for ON, false for OFF)
     * @return Corresponding Woleix power state
     */
    static WoleixPowerState esphome_to_woleix_power(bool power);
};

}  // namespace climate_ir_woleix
}  // namespace esphome
