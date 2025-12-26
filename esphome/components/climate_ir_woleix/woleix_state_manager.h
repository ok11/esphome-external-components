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

/**
 * @brief Builder class for creating WoleixInternalState objects.
 * 
 * This class provides a fluent interface for constructing WoleixInternalState
 * objects, allowing for easy and readable state creation.
 */
class WoleixInternalStateBuilder
{
public:
    /**
     * @brief Set the power state.
     * @param p The power state to set.
     * @return Reference to this builder for method chaining.
     */
    WoleixInternalStateBuilder& power(WoleixPowerState p) { state_.power = p; return *this; }

    /**
     * @brief Set the operating mode.
     * @param m The mode to set.
     * @return Reference to this builder for method chaining.
     */
    WoleixInternalStateBuilder& mode(WoleixMode m) { state_.mode = m; return *this; }

    /**
     * @brief Set the temperature.
     * @param t The temperature to set in Celsius.
     * @return Reference to this builder for method chaining.
     */
    WoleixInternalStateBuilder& temperature(float t) { state_.temperature = t; return *this; }

    /**
     * @brief Set the fan speed.
     * @param f The fan speed to set.
     * @return Reference to this builder for method chaining.
     */
    WoleixInternalStateBuilder& fan(WoleixFanSpeed f) { state_.fan_speed = f; return *this; }

    /**
     * @brief Build and return the constructed WoleixInternalState.
     * @return The fully constructed WoleixInternalState object.
     */
    WoleixInternalState build() { return state_; }

private:
    WoleixInternalState state_;
};

/**
 * @brief Factory class for creating WoleixCommand objects.
 * 
 * This class encapsulates the creation logic for WoleixCommand objects,
 * ensuring that all commands are created with the correct address.
 */
class WoleixCommandFactory
{
public:
    /**
     * @brief Construct a new WoleixCommandFactory.
     * @param address The address to use for all created commands.
     */
    WoleixCommandFactory(uint16_t address) : address_(address) {}

    virtual ~WoleixCommandFactory() = default;

    /**
     * @brief Create a new WoleixCommand.
     * @param type The type of command to create.
     * @param repeats The number of times to repeat the command (default is 1).
     * @return A new WoleixCommand object.
     */
    virtual WoleixCommand create(WoleixCommand::Type type, uint32_t repeats = 1) const
    {
        return WoleixCommand(type, address_, repeats);
    }

private:
    uint16_t address_;  /**< The address used for all created commands */
};

namespace WoleixCategory::StateManager
{
    inline constexpr auto WX_CATEGORY_INVALID_MODE = 
        Category::make(CategoryId::StateManager, 1, "StateManager.InvalidMode");
}

/**
 * @brief State manager for Woleix AC unit control via IR commands.
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
 * WoleixStateManager state_manager;
 * state_manager.move_to(WoleixPowerState::ON, WoleixMode::COOL, 24.0f, WoleixFanSpeed::HIGH);
 * auto commands = state_manager.get_commands();
 * // Transmit commands via IR
 * @endcode
 * 
 * @see WoleixInternalState
 */
class WoleixStateManager: protected WoleixStatusReporter
{
public:
    /**
     * Default constructor.
     * 
     * Initializes the state manager with default device settings:
     * power=OFF, mode=COOL, temperature=25°C, fan_speed=LOW
     */
    WoleixStateManager();

    virtual ~WoleixStateManager() = default;

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
    virtual const WoleixInternalState& get_state() const { return current_state_; }

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

    WoleixInternalState current_state_;  /**< Current tracked state of the AC unit */
    std::unique_ptr<WoleixCommandFactory> command_factory_{nullptr};  /**< Factory for creating IR commands */

    std::vector<WoleixCommand> commands_;
};

}  // namespace climate_ir_woleix
}  // namespace esphome
