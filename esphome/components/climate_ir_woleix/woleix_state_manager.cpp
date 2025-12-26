#include <cmath>
#include <algorithm>
#include <array>
#include <format>

#include "esphome/core/log.h"

#include "woleix_state_manager.h"

namespace esphome {
namespace climate_ir_woleix {

/**
 * Mode sequence for circular mode transitions.
 * 
 * The Woleix AC cycles through modes in this order when the MODE button
 * is pressed: COOL → DEHUM → FAN → COOL (wraps around).
 */
static constexpr std::array<WoleixMode, 3> MODE_SWITCH_SEQUENCE =
{
    WoleixMode::COOL,
    WoleixMode::DEHUM,
    WoleixMode::FAN
};

static constexpr float TEMP_EPSILON = 0.5f; 

WoleixStateManager::WoleixStateManager()
  : command_factory_(std::make_unique<WoleixCommandFactory>(ADDRESS_NEC)) 
{
    reset();
}

void WoleixStateManager::setup()
{
    // command_queue_ = command_queue;
}
/**
 * Calculate and generate the command sequence needed to reach target state.
 * 
 * This is the main entry point for state transitions. It orchestrates the
 * generation of IR commands in the correct order:
 * 1. Power state changes
 * 2. Mode changes (if powered on)
 * 3. Temperature adjustments (if in COOL mode)
 * 4. Fan speed changes (if in FAN mode)
 * 
 * The function updates internal state as commands are generated and returns
 * a reference to the complete command queue.
 * 
 * @param power Target internal state
 * 
 * Side effect: fills in the command queue with commnds in the right order
 */
const std::vector<WoleixCommand>& WoleixStateManager::move_to(const WoleixInternalState& target_state)
{
    // if (!command_queue_)
    // {
    //     ESP_LOGE(TAG, "WoleixStateManager not initialized with command queue");
    //     return;
    // }

    commands_.clear();

    WoleixPowerState power = target_state.power;
    WoleixMode mode = target_state.mode;
    float temperature = target_state.temperature;
    WoleixFanSpeed fan_speed = target_state.fan_speed;

    // Step 1: Handle power state transitions
    generate_power_commands_(power);
    
    // If turning off, we're done, otherwise continue
    if (current_state_.power == WoleixPowerState::ON)
    {
        // Step 2: Handle mode changes
        generate_mode_commands_(mode);
        
        // Step 3: Handle temperature changes (only in COOL mode)
        if (current_state_.mode == WoleixMode::COOL)
        {
            generate_temperature_commands_(temperature);
        }
        
        // Step 4: Handle fan speed changes (only in FAN mode)
        if (current_state_.mode == WoleixMode::FAN)
        {
            generate_fan_commands_(fan_speed);
        }
        
        ESP_LOGD(TAG, "Calculated and queued %zu commands for state transition: power=%d, mode=%d, temp=%.1f, fan=%d",
            commands_.size(),
            static_cast<int>(power),
            static_cast<int>(mode),
            temperature,
            static_cast<int>(fan_speed));

    }
    return commands_;
}

/**
 * Reset the state machine to default values.
 * 
 * Sets the internal state to: power=ON, mode=COOL, temperature=25°C, fan_speed=LOW.
 * This is used for state reconciliation after a physical power cycle of the AC unit.
 * 
 * Note: This does not generate any IR commands; it only resets internal tracking.
 */
void WoleixStateManager::reset()
{
    current_state_ = WoleixInternalState();  // Reset to defaults
    commands_.clear();

    ESP_LOGD(TAG, "State manager reset to defaults: ON, COOL, 25°C, LOW fan");
}

/**
 * Generate power state change commands.
 * 
 * Adds a POWER command to the queue if the target power state differs from current.
 * The POWER command toggles the AC unit on/off.
 * 
 * @param target_power Desired power state
 */
void WoleixStateManager::generate_power_commands_(WoleixPowerState target_power)
{
    if (current_state_.power != target_power)
    {
        // Power state change required
        enqueue_command_(command_factory_->create(WoleixCommand::Type::POWER));

        current_state_.power = target_power;

        ESP_LOGD(TAG, "Power switched to %s", 
            target_power == WoleixPowerState::ON ? "ON" : "OFF");
    }
}

/**
 * Generate mode change commands.
 * 
 * Calculates the number of MODE button presses needed to reach the target mode
 * via the circular mode sequence (COOL→DEHUM→FAN→COOL), then generates the
 * appropriate command with repeat count.
 * 
 * @param target_mode Desired operating mode
 */
void WoleixStateManager::generate_mode_commands_(WoleixMode target_mode)
{
    if (current_state_.mode != target_mode) 
    {
        int steps = calculate_mode_steps_(current_state_.mode, target_mode);
        
        // Send MODE commands to cycle through modes
        for (int i = 0; i < steps; i++)
        {
            enqueue_command_(command_factory_->create(WoleixCommand::Type::MODE));
        }

        current_state_.mode = target_mode;
        
        ESP_LOGD(TAG, "Mode change: %d steps to reach mode %d", 
                steps, static_cast<int>(target_mode));
    }
}

/**
 * Generate temperature adjustment commands.
 * 
 * Only applicable in COOL mode. Calculates the temperature difference and generates
 * the appropriate number of TEMP_UP or TEMP_DOWN commands to reach the target.
 * Each command changes temperature by 1°C. Temperature is clamped to 15-30°C range.
 * 
 * @param target_temp Desired temperature in Celsius
 */
void WoleixStateManager::generate_temperature_commands_(float target_temp)
{
    // Temperature is only adjustable in COOL mode
    if (current_state_.mode == WoleixMode::COOL)
    {
        // Clamp temperature to valid range
        target_temp = std::clamp(target_temp, WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX);

        float temp_diff = target_temp - current_state_.temperature;

        int steps = static_cast<int>(std::round(temp_diff));

        if (steps == 0) return;

        WoleixCommand::Type type = (steps > 0)
            ? WoleixCommand::Type::TEMP_UP
            : WoleixCommand::Type::TEMP_DOWN;

        for (int i = 0; i < std::abs(steps); i++)
        {
            enqueue_command_(command_factory_->create(type));
        }
        current_state_.temperature += steps;
        
        ESP_LOGD(TAG, "Temperature change: %d steps to %.1f°C", 
            steps, current_state_.temperature);
    }
}

/**
 * Generate fan speed change commands.
 * 
 * Adds a FAN_SPEED command to toggle between LOW and HIGH fan speeds.
 * The fan speed setting is only relevant in FAN mode but can be adjusted in any mode.
 * 
 * @param target_fan Desired fan speed
 */
void WoleixStateManager::generate_fan_commands_(WoleixFanSpeed target_fan)
{
    if (current_state_.fan_speed != target_fan)
    {
        // Fan speed toggles between LOW and HIGH with single SPEED command
        enqueue_command_(command_factory_->create(WoleixCommand::Type::FAN_SPEED));
        
        current_state_.fan_speed = target_fan;
        
        ESP_LOGD(TAG, "Fan speed changed to %s", 
            target_fan == WoleixFanSpeed::LOW ? "LOW" : "HIGH");
    }
}

/**
 * Calculate the number of MODE button presses needed.
 * 
 * Determines the shortest path through the circular mode sequence
 * (COOL→DEHUM→FAN→COOL) from the current mode to the target mode.
 * 
 * @param from_mode Current operating mode
 * @param to_mode Target operating mode
 * @return Number of MODE button presses needed (0-2)
 */
int WoleixStateManager::calculate_mode_steps_(WoleixMode from_mode, WoleixMode to_mode)
{
    auto from_it = std::ranges::find(MODE_SWITCH_SEQUENCE, from_mode);
    auto to_it = std::ranges::find(MODE_SWITCH_SEQUENCE, to_mode);
    
    // Check if both modes were found
    if (from_it == MODE_SWITCH_SEQUENCE.end() || to_it == MODE_SWITCH_SEQUENCE.end())
    {
        report
        (
            WoleixStatus
            (
                WoleixStatus::Severity::WX_SEVERITY_WARNING,
                WoleixCategory::StateManager::WX_CATEGORY_INVALID_MODE,
                std::format
                (
                    "Invalid mode in sequence: from={}, to={}", 
                    "static_cast<uint8_t>(from_mode)", 
                    "static_cast<uint8_t>(to_mode)"
                )
            )
        );
        // ESP_LOGW(TAG, "Invalid mode in sequence: from=%d, to=%d", 
        //     static_cast<int>(from_mode), static_cast<int>(to_mode));
        return 0;
    }
    
    // Calculate indices using std::ranges::distance
    int from_index = std::ranges::distance(MODE_SWITCH_SEQUENCE.begin(), from_it);
    int to_index = std::ranges::distance(MODE_SWITCH_SEQUENCE.begin(), to_it);
    
    // Calculate forward distance with wrap-around
    int size = static_cast<int>(MODE_SWITCH_SEQUENCE.size());
    int steps = (to_index - from_index + size) % size;
    
    return steps;
}

/**
 * Add a command to the transmission queue.
 * 
 * Commands are queued in the order they should be transmitted.
 * 
 * @param command Command to add to the queue
 */
void WoleixStateManager::enqueue_command_(const WoleixCommand& command)
{
    commands_.push_back(command);
}

}  // namespace climate_ir_woleix
}  // namespace esphome
