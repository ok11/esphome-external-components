#pragma once

#include <string>
#include <vector>
#include <memory>

#include "woleix_comm.h"

namespace esphome {
namespace climate_ir_woleix {

/**
 * @brief Represents the power state of the AC unit.
 */
enum class WoleixPowerState {
    OFF,  /**< AC unit is powered off */
    ON    /**< AC unit is powered on */
};

/**
 * @brief Represents the operating mode of the AC unit.
 */
enum class WoleixMode {
    COOL,   /**< Cooling mode - temperature adjustable (15-30°C) */
    DEHUM,  /**< Dehumidify/Dry mode - removes moisture from air */
    FAN     /**< Fan only mode - circulates air without cooling */
};

/**
 * @brief Represents the fan speed settings of the AC unit.
 */
enum class WoleixFanSpeed {
    LOW,   /**< Low fan speed */
    HIGH   /**< High fan speed */
};

const WoleixPowerState WOLEIX_POWER_DEFAULT = WoleixPowerState::ON;  /**< Default power state */
const WoleixMode WOLEIX_MODE_DEFAULT = WoleixMode::COOL;              /**< Default operating mode */
const float WOLEIX_TEMP_DEFAULT = 25.0f;                              /**< Default temperature in Celsius */
const WoleixFanSpeed WOLEIX_FAN_DEFAULT = WoleixFanSpeed::LOW;        /**< Default fan speed */

/**
 * @brief Complete internal state of the AC unit.
 * 
 * This structure holds all the state variables that define the current
 * operating configuration of the air conditioner unit.
 */
struct WoleixInternalState {
    WoleixPowerState power;    /**< Power state (ON/OFF) */
    WoleixMode mode;           /**< Operating mode (COOL/DEHUM/FAN) */
    float temperature;         /**< Target temperature in Celsius (15-30°C, only applicable in COOL mode) */
    WoleixFanSpeed fan_speed;  /**< Fan speed setting (LOW/HIGH) */

    /**
        * Default constructor initializes to device defaults.
        * 
        * Sets: power=ON, mode=COOL, temperature=25.0°C, fan_speed=LOW
        */
    WoleixInternalState()
                    : power(WOLEIX_POWER_DEFAULT),
                            mode(WOLEIX_MODE_DEFAULT),
                            temperature(WOLEIX_TEMP_DEFAULT),
                            fan_speed(WOLEIX_FAN_DEFAULT)
    {}
};

class WoleixCommandFactory
{
public:
    WoleixCommandFactory(uint16_t address) : address_(address) {}
    virtual ~WoleixCommandFactory() = default;

    virtual WoleixCommand create(WoleixCommand::Type type, uint32_t delay = 0, uint32_t repeats = 1) const
    {
                    return WoleixCommand(type, address_, delay, repeats);
    }
private:
    uint16_t address_;
};

/**
 * @brief State machine for Woleix AC unit control via IR commands.
 * 
 * This class manages the internal state of the AC unit and generates
 * the necessary sequence of IR commands to transition
 * from the current state to a desired target state.
 * 
 * Key behaviors:
 * - Power toggle affects all other states (turning ON resets to defaults)
 * - Mode cycles through COOL→DEHUM→FAN→COOL in sequence
 * - Temperature only adjustable in COOL mode (15-30°C range)
 * - Fan speed toggles between LOW and HIGH
 * 
 * Usage example:
 * @code
 * WoleixStateMachine state_machine;
 * state_machine.transit_to_state(WoleixPowerState::ON, WoleixMode::COOL, 24.0f, WoleixFanSpeed::HIGH);
 * auto commands = state_machine.get_commands();
 * // Transmit commands via IR
 * @endcode
 * 
 * @see WoleixInternalState
 */
class WoleixStateMachine {
public:
    /**
     * Default constructor.
     * 
     * Initializes the state machine with default device settings:
     * power=ON, mode=COOL, temperature=25°C, fan_speed=LOW
     */
    WoleixStateMachine();

    /**
     * Factory constructor.
     * 
     * Initializes the state machine with default device settings:
     * power=ON, mode=COOL, temperature=25°C, fan_speed=LOW
     * and sets the command factory
     */
    WoleixStateMachine(WoleixCommandFactory* command_factory);

    /**
     * Set the target state and generate the command sequence needed.
     * 
     * This method calculates the optimal sequence of IR commands to transition
     * from the current state to the target state. The generated commands are
     * queued internally and can be retrieved with get_commands().
     * 
     * @param power Target power state (ON/OFF)
     * @param mode Target operating mode (COOL/DEHUM/FAN)
     * @param temperature Target temperature in Celsius (15-30°C, only used in COOL mode)
     * @param fan_speed Target fan speed (LOW/HIGH)
     * 
     * @note Temperature parameter is ignored when mode is not COOL
     * @note Turning power OFF will queue power command only, ignoring other parameters
     * @note Turning power ON from OFF will reset all settings to defaults
     */
    const std::vector<WoleixCommand>& transit_to_state(WoleixPowerState power, WoleixMode mode, 
                    float temperature, WoleixFanSpeed fan_speed);

    /**
     * Reset the internal state to device defaults.
     * 
     * Used for recovery when the actual AC state may have drifted out of
     * sync with the tracked state. This does NOT generate any IR commands;
     * it only resets the internal state tracking.
     * 
     * Default state: power=ON, mode=COOL, temperature=25°C, fan_speed=LOW
     * 
     * @note This does not transmit any IR commands
     * @note Call set_target_state() after reset to synchronize the physical unit
     */
    void reset();

    /**
     * Get the current internal state.
     * 
     * @return Const reference to the current state structure
     * 
     * @note This returns the tracked state, not the actual AC unit state
     */
    const WoleixInternalState& get_state() const { return current_state_; }

protected:

    WoleixInternalState current_state_;  /**< Current tracked state of the AC unit */
    WoleixCommandFactory* command_factory_{nullptr};  /**< Factory for creating IR commands */

private:
    std::vector<WoleixCommand> command_queue_;  /**< Queue of IR commands to be transmitted */

    /**
     * Generate IR commands to change power state.
     * 
     * @param target_power Target power state (ON/OFF)
     */
    void generate_power_commands_(WoleixPowerState target_power);

    /**
     * Generate IR commands to change operating mode.
     * 
     * Uses the circular mode sequence: COOL→DEHUM→FAN→COOL
     * 
     * @param target_mode Target operating mode
     */
    void generate_mode_commands_(WoleixMode target_mode);

    /**
     * Generate IR commands to adjust temperature.
     * 
     * Only applicable in COOL mode. Generates TEMP_UP or TEMP_DOWN
     * commands as needed to reach the target temperature.
     * 
     * @param target_temp Target temperature in Celsius (15-30°C)
     */
    void generate_temperature_commands_(float target_temp);

    /**
     * Generate IR commands to change fan speed.
     * 
     * @param target_fan Target fan speed (LOW/HIGH)
     */
    void generate_fan_commands_(WoleixFanSpeed target_fan);

    /**
     * Calculate the number of MODE button presses needed.
     * 
     * Calculates the shortest path through the circular mode sequence
     * (COOL→DEHUM→FAN→COOL) to reach the target mode.
     * 
     * @param from_mode Starting mode
     * @param to_mode Target mode
     * @return Number of MODE button presses needed (0-2)
     */
    int calculate_mode_steps_(WoleixMode from_mode, WoleixMode to_mode);

    /**
     * Add a command to the transmission queue.
     * 
     * @param command IR command object
     */
    void enqueue_command_(const WoleixCommand& command);
};

}  // namespace climate_ir_woleix
}  // namespace esphome
