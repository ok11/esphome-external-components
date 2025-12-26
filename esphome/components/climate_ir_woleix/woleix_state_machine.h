#pragma once

#include <string>
#include <vector>
#include <memory>

#include "woleix_command.h"

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

const WoleixPowerState WOLEIX_POWER_DEFAULT = WoleixPowerState::OFF;         /**< Default power state */
const WoleixMode WOLEIX_MODE_DEFAULT = WoleixMode::COOL;                     /**< Default operating mode */
const float WOLEIX_TEMP_DEFAULT = 25.0f;                                     /**< Default temperature in Celsius */
const WoleixFanSpeed WOLEIX_FAN_DEFAULT = WoleixFanSpeed::LOW;               /**< Default fan speed */

/**
 * @brief Complete internal state of the AC unit.
 * 
 * This structure holds all the state variables that define the current
 * operating configuration of the air conditioner unit.
 */
struct WoleixInternalState {
    WoleixPowerState power;         /**< Power state (ON/OFF) */
    WoleixMode mode;                /**< Operating mode (COOL/DEHUM/FAN) */
    float temperature;              /**< Target temperature in Celsius (15-30°C, only applicable in COOL mode) */
    WoleixFanSpeed fan_speed;       /**< Fan speed (LOW/HIGH) */

    /**
        * Default constructor initializes to device defaults.
        * 
        * Sets: power=ON, mode=COOL, temperature=25.0°C, fan_speed=LOW
        */
    WoleixInternalState()
      : WoleixInternalState
        (
            WOLEIX_POWER_DEFAULT,
            WOLEIX_MODE_DEFAULT,
            WOLEIX_TEMP_DEFAULT,
            WOLEIX_FAN_DEFAULT
        )
    {}

    WoleixInternalState(WoleixPowerState p, WoleixMode m, float t, WoleixFanSpeed f)
      : power(p),
        mode(m),
        temperature(t),
        fan_speed(f)
    {}

    bool operator==(const WoleixInternalState& other)
    {
        return power == other.power 
            && mode == other.mode
            && temperature == other.temperature
            && fan_speed == other.fan_speed;
    }
};

class WoleixInternalStateBuilder
{
public:
    WoleixInternalStateBuilder& power(WoleixPowerState p) { state_.power = p; return *this; }
    WoleixInternalStateBuilder& mode(WoleixMode m) { state_.mode = m; return *this; }
    WoleixInternalStateBuilder& temperature(float t) { state_.temperature = t; return *this; }
    WoleixInternalStateBuilder& fan(WoleixFanSpeed f) { state_.fan_speed = f; return *this; }
    WoleixInternalState build() { return state_; }
private:
    WoleixInternalState state_;
};

class WoleixCommandFactory
{
public:
    WoleixCommandFactory(uint16_t address) : address_(address) {}
    virtual ~WoleixCommandFactory() = default;

    virtual WoleixCommand create(WoleixCommand::Type type, uint32_t repeats = 1) const
    {
        return WoleixCommand(type, address_, repeats);
    }
private:
    uint16_t address_;
};

namespace WoleixCategory::StateManager
{
    inline constexpr auto WX_CATEGORY_INVALID_MODE = 
        Category::make(CategoryId::StateManager, 1, "StateManager.InvalidMode");
}

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
 * state_machine.move_to(WoleixPowerState::ON, WoleixMode::COOL, 24.0f, WoleixFanSpeed::HIGH);
 * auto commands = state_machine.get_commands();
 * // Transmit commands via IR
 * @endcode
 * 
 * @see WoleixInternalState
 */
class WoleixStateMachine: protected WoleixStatusReporter
{
public:
    /**
     * Default constructor.
     * 
     * Initializes the state machine with default device settings:
     * power=OFF, mode=COOL, temperature=25°C, fan_speed=LOW
     */
    WoleixStateMachine();

    virtual ~WoleixStateMachine() = default;

    void setup();
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
    const std::vector<WoleixCommand>& move_to(const WoleixInternalState& target_state);

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


    // void hold()
    // {
    //     on_hold_ = true;
    // }

    // void resume()
    // {
    //     on_hold_ = false;
    // }
    WoleixInternalState current_state_;  /**< Current tracked state of the AC unit */
    std::unique_ptr<WoleixCommandFactory> command_factory_{nullptr};  /**< Factory for creating IR commands */

//    WoleixCommandQueue* command_queue_;

    std::vector<WoleixCommand> commands_;
    
//    bool on_hold_{false};
};

}  // namespace climate_ir_woleix
}  // namespace esphome
