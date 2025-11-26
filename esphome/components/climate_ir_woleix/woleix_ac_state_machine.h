#pragma once

#include <string>
#include <vector>

namespace esphome {
namespace climate_ir_woleix {

// Power state of the AC unit
enum class WoleixPowerState {
    OFF,
    ON
};

// Operating mode of the AC unit
enum class WoleixMode {
    COOL,      // Cooling mode - temperature adjustable
    DEHUM,     // Dehumidify/Dry mode
    FAN        // Fan only mode
};

// Fan speed settings
enum class WoleixFanSpeed {
    LOW,
    HIGH
};

const WoleixPowerState WOLEIX_POWER_DEFAULT = WoleixPowerState::ON;
const WoleixMode WOLEIX_MODE_DEFAULT = WoleixMode::COOL;
const float WOLEIX_TEMP_DEFAULT = 25.0f;  // Celsius
const WoleixFanSpeed WOLEIX_FAN_DEFAULT = WoleixFanSpeed::LOW;

// Complete internal state of the AC unit
struct WoleixInternalState {
    WoleixPowerState power;
    WoleixMode mode;
    float temperature;        // Temperature in Celsius (15-30°C, only applicable in COOL mode)
    WoleixFanSpeed fan_speed;
    
    // Default constructor initializes to device defaults
    WoleixInternalState()
        : power(WOLEIX_POWER_DEFAULT),
          mode(WOLEIX_MODE_DEFAULT),
          temperature(WOLEIX_TEMP_DEFAULT),
          fan_speed(WOLEIX_FAN_DEFAULT)
    {}
};

/**
 * State machine for Woleix AC unit control via IR commands.
 * 
 * This class manages the internal state of the AC unit and generates
 * the necessary sequence of Pronto IR commands to transition from the
 * current state to a desired target state.
 * 
 * Key behaviors:
 * - Power toggle affects all other states (ON resets to defaults)
 * - Mode cycles through COOL→DEHUM→FAN→COOL
 * - Temperature only adjustable in COOL mode
 * - Fan speed toggles between LOW and HIGH
 */
class WoleixACStateMachine {
public:
    WoleixACStateMachine();
    
    /**
     * Set the target state and generate the command sequence needed.
     * This calculates the optimal sequence of IR commands to reach
     * the target state from the current state.
     * 
     * @param power Target power state
     * @param mode Target operating mode
     * @param temperature Target temperature (only used in COOL mode)
     * @param fan_speed Target fan speed
     */
    void set_target_state(WoleixPowerState power, WoleixMode mode, 
                         float temperature, WoleixFanSpeed fan_speed);
    
    /**
     * Get the generated command sequence and clear the internal queue.
     * 
     * @return Vector of Pronto hex command strings
     */
    std::vector<std::string> get_commands();
    
    /**
     * Reset the internal state to device defaults.
     * Used for recovery when state may be out of sync.
     * 
     * Defaults: ON, COOL mode, 25°C, LOW fan speed
     */
    void reset();
    
    /**
     * Get the current internal state.
     * 
     * @return Reference to current state
     */
    const WoleixInternalState& get_state() const { return current_state_; }

protected:

    WoleixInternalState current_state_;

private:
    std::vector<std::string> command_queue_;
    
    // Command generation methods
    void generate_power_commands_(WoleixPowerState target_power);
    void generate_mode_commands_(WoleixMode target_mode);
    void generate_temperature_commands_(float target_temp);
    void generate_fan_commands_(WoleixFanSpeed target_fan);
    
    /**
     * Calculate the number of MODE button presses needed to go from
     * one mode to another in the circular sequence.
     * 
     * @param from_mode Starting mode
     * @param to_mode Target mode
     * @return Number of steps (0-2)
     */
    int calculate_mode_steps_(WoleixMode from_mode, WoleixMode to_mode);
    
    /**
     * Add a command to the queue.
     * 
     * @param pronto_hex Pronto format IR command string
     */
    void enqueue_command_(const std::string& pronto_hex);
};

}  // namespace climate_ir_woleix
}  // namespace esphome

