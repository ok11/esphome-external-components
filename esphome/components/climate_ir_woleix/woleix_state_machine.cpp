
#include "esphome/core/log.h"

#include "woleix_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

// Mode sequence for circular transitions
static const std::vector<WoleixMode> MODE_SWITCH_SEQUENCE = 
{
    WoleixMode::COOL,
    WoleixMode::DEHUM,
    WoleixMode::FAN
};

WoleixStateMachine::WoleixStateMachine()
    : WoleixStateMachine(new WoleixCommandFactory(ADDRESS_NEC)) 
{
}

WoleixStateMachine::WoleixStateMachine(WoleixCommandFactory* command_factory)
    : command_factory_(command_factory) 
{
    reset();
}

const std::vector<WoleixCommand>& WoleixStateMachine::transit_to_state
(
    WoleixPowerState power, WoleixMode mode, float temperature, WoleixFanSpeed fan_speed
)
{
    // Clear any previous commands
    command_queue_.clear();
    
    // Step 1: Handle power state transitions
    generate_power_commands_(power);
    
    // If turning off, we're done, otherwise continue
    if (current_state_.power == WoleixPowerState::ON)
    {
        generate_mode_commands_(mode);
        
        // Step 3: Handle temperature changes (only in COOL mode)
        if (current_state_.mode == WoleixMode::COOL)
        {
            generate_temperature_commands_(temperature);
        }
        
        // Step 4: Handle fan speed changes
        if (current_state_.mode == WoleixMode::FAN)
        {
            generate_fan_commands_(fan_speed);
        }
        
        ESP_LOGD(TAG, "Calculated and queued %zu commands for state transition: power=%d, mode=%d, temp=%.1f, fan=%d",
                command_queue_.size(),
                static_cast<int>(power),
                static_cast<int>(mode),
                temperature,
                static_cast<int>(fan_speed));

    }
    return command_queue_;
}

void WoleixStateMachine::reset()
{
    current_state_ = WoleixInternalState();  // Reset to defaults
    command_queue_.clear();
    
    ESP_LOGD(TAG, "State machine reset to defaults: ON, COOL, 25°C, LOW fan");
}

void WoleixStateMachine::generate_power_commands_(WoleixPowerState target_power)
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

void WoleixStateMachine::generate_mode_commands_(WoleixMode target_mode)
{
    if (current_state_.mode != target_mode) 
    {
        int steps = calculate_mode_steps_(current_state_.mode, target_mode);
        
        // Send MODE commands to cycle through modes
        enqueue_command_(command_factory_->create(WoleixCommand::Type::MODE, 200, steps));

        current_state_.mode = target_mode;
        
        ESP_LOGD(TAG, "Mode change: %d steps to reach mode %d", 
                steps, static_cast<int>(target_mode));
    }
}

void WoleixStateMachine::generate_temperature_commands_(float target_temp)
{
    // Temperature is only adjustable in COOL mode
    if (current_state_.mode == WoleixMode::COOL)
    {
        // Clamp temperature to valid range
        target_temp = std::clamp(target_temp, WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX);

        float temp_diff = target_temp - current_state_.temperature;
        
        if (temp_diff > 0.1f)
        {
            // Temperature increase needed
            int steps = std::lround(std::abs(temp_diff));  // Round to nearest integer

            if (steps > 0)
                enqueue_command_(command_factory_->create(WoleixCommand::Type::TEMP_UP, 150, steps + 1));

            current_state_.temperature += steps;
            
            ESP_LOGD(TAG, "Temperature UP: %d steps to %.1f°C", steps, current_state_.temperature);
            
        }
        else if (temp_diff < -0.1f)
        {
            // Temperature decrease needed
            int steps = std::lround(std::abs(temp_diff));  // Round to nearest integer
            
            if (steps > 0)
                enqueue_command_(command_factory_->create(WoleixCommand::Type::TEMP_DOWN, 150, steps + 1));

            current_state_.temperature -= steps;
            
            ESP_LOGD(TAG, "Temperature DOWN: %d steps to %.1f°C", steps, current_state_.temperature);
        }
    }
}

void WoleixStateMachine::generate_fan_commands_(WoleixFanSpeed target_fan)
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

int WoleixStateMachine::calculate_mode_steps_(WoleixMode from_mode, WoleixMode to_mode)
{
    auto from_it = std::ranges::find(MODE_SWITCH_SEQUENCE, from_mode);
    auto to_it = std::ranges::find(MODE_SWITCH_SEQUENCE, to_mode);
    
    // Check if both modes were found
    if (from_it == MODE_SWITCH_SEQUENCE.end() || to_it == MODE_SWITCH_SEQUENCE.end())
    {
        ESP_LOGW(TAG, "Invalid mode in sequence: from=%d, to=%d", 
            static_cast<int>(from_mode), static_cast<int>(to_mode));
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

void WoleixStateMachine::enqueue_command_(const WoleixCommand& command)
{
    command_queue_.push_back(command);
}

}  // namespace climate_ir_woleix
}  // namespace esphome
