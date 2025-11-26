#include "woleix_ac_state_machine.h"
#include "climate_ir_woleix.h"
#include "esphome/core/log.h"

namespace esphome {
namespace climate_ir_woleix {

static const char *const TAG = "woleix_ac_state_machine";

// Mode sequence for circular transitions
static const std::vector<WoleixMode> MODE_SEQUENCE = 
{
    WoleixMode::COOL,
    WoleixMode::DEHUM,
    WoleixMode::FAN
};

WoleixACStateMachine::WoleixACStateMachine()
{
    reset();
}

void WoleixACStateMachine::set_target_state
(
    WoleixPowerState power, WoleixMode mode, float temperature, WoleixFanSpeed fan_speed
)
{
    // Clear any previous commands
    command_queue_.clear();
    
    // Step 1: Handle power state transitions
    generate_power_commands_(power);
    
    // If turning off, we're done
    if (current_state_.power == WoleixPowerState::OFF) return;
    
    // Step 2: Handle mode changes (only if powered on)
    if (current_state_.power == WoleixPowerState::ON)
    {
        generate_mode_commands_(mode);
    }
    
    // Step 3: Handle temperature changes (only in COOL mode)
    if (current_state_.mode == WoleixMode::COOL)
    {
        generate_temperature_commands_(temperature);
    }
    
    // Step 4: Handle fan speed changes
    generate_fan_commands_(fan_speed);
    
    ESP_LOGD(TAG, "Generated %zu commands to reach the target state: power=%d, mode=%d, temp=%.1f, fan=%d",
             command_queue_.size(),
             static_cast<int>(power),
             static_cast<int>(mode),
             temperature,
             static_cast<int>(fan_speed));
}

std::vector<std::string> WoleixACStateMachine::get_commands()
{
    std::vector<std::string> commands = std::move(command_queue_);
    command_queue_.clear();
    return commands;
}

void WoleixACStateMachine::reset()
{
    current_state_ = WoleixInternalState();  // Reset to defaults
    command_queue_.clear();
    
    ESP_LOGD(TAG, "State machine reset to defaults: ON, COOL, 25°C, LOW fan");
}

void WoleixACStateMachine::generate_power_commands_(WoleixPowerState target_power)
{
    if (current_state_.power != target_power)
    {
        // Power state change required
        enqueue_command_(POWER_PRONTO);
        
        if (target_power == WoleixPowerState::ON)
        {            
            // Turning on
            ESP_LOGD(TAG, "Power ON");
            current_state_.power = WoleixPowerState::ON;
        }
        else
        {
            // Turning off
            ESP_LOGD(TAG, "Power OFF");
            current_state_.power = WoleixPowerState::OFF;            
        }
    }
}

void WoleixACStateMachine::generate_mode_commands_(WoleixMode target_mode)
{
    if (current_state_.mode != target_mode) 
    {
        int steps = calculate_mode_steps_(current_state_.mode, target_mode);
        
        // Send MODE commands to cycle through modes
        for (int i = 0; i < steps; i++)
        {
            enqueue_command_(MODE_PRONTO);
        }
        
        current_state_.mode = target_mode;
        
        ESP_LOGD(TAG, "Mode change: %d steps to reach mode %d", 
                 steps, static_cast<int>(target_mode));
    }
}

void WoleixACStateMachine::generate_temperature_commands_(float target_temp)
{
    // Temperature is only adjustable in COOL mode
    if (current_state_.mode == WoleixMode::COOL)
    {
    
        // Clamp temperature to valid range
        if (target_temp < 15.0f) target_temp = 15.0f;
        if (target_temp > 30.0f) target_temp = 30.0f;
        
        float temp_diff = target_temp - current_state_.temperature;
        
        if (temp_diff > 0.1f)
        {
            // Temperature increase needed
            int steps = static_cast<int>(temp_diff + 0.5f);  // Round to nearest integer
            
            for (int i = 0; i < steps; i++)
            {
                enqueue_command_(TEMP_UP_PRONTO);
            }
            
            current_state_.temperature += steps;
            
            ESP_LOGD(TAG, "Temperature UP: %d steps to %.1f°C", steps, current_state_.temperature);
            
        }
        else if (temp_diff < -0.1f)
        {
            // Temperature decrease needed
            int steps = static_cast<int>(-temp_diff + 0.5f);  // Round to nearest integer
            
            for (int i = 0; i < steps; i++)
            {
                enqueue_command_(TEMP_DOWN_PRONTO);
            }
            
            current_state_.temperature -= steps;
            
            ESP_LOGD(TAG, "Temperature DOWN: %d steps to %.1f°C", steps, current_state_.temperature);
        }
    }
}

void WoleixACStateMachine::generate_fan_commands_(WoleixFanSpeed target_fan)
{
    if (current_state_.fan_speed != target_fan)
    {
        // Fan speed toggles between LOW and HIGH with single SPEED command
        enqueue_command_(SPEED_PRONTO);
        
        current_state_.fan_speed = target_fan;
        
        ESP_LOGD(TAG, "Fan speed changed to %s", 
                 target_fan == WoleixFanSpeed::LOW ? "LOW" : "HIGH");
    }
}

int WoleixACStateMachine::calculate_mode_steps_(WoleixMode from_mode, WoleixMode to_mode)
{
    // Find indices in the mode sequence
    int from_index = -1;
    int to_index = -1;
    
    for (size_t i = 0; i < MODE_SEQUENCE.size(); i++) {
        if (MODE_SEQUENCE[i] == from_mode)
        {
            from_index = static_cast<int>(i);
        }
        if (MODE_SEQUENCE[i] == to_mode)
        {
            to_index = static_cast<int>(i);
        }
    }
    
    if (from_index < 0 || to_index < 0)
    {
        ESP_LOGW(TAG, "Invalid mode in sequence: from=%d, to=%d", 
                 static_cast<int>(from_mode), static_cast<int>(to_mode));
        return 0;
    }
    
    // Calculate forward distance with wrap-around
    int size = static_cast<int>(MODE_SEQUENCE.size());
    int steps = (to_index - from_index + size) % size;
    
    return steps;
}

void WoleixACStateMachine::enqueue_command_(const std::string& pronto_hex)
{
    command_queue_.push_back(pronto_hex);
}

}  // namespace climate_ir_woleix
}  // namespace esphome

