/* #include "esphome/core/log.h"

#include "woleix_protocol_handler.h"

namespace esphome {
namespace climate_ir_woleix {

static const char* TAG = "woleix_protocol";

WoleixProtocolHandler::WoleixProtocolHandler(WoleixTransmitter* transmitter, 
        ProtocolScheduler* scheduler)
    : transmitter_(transmitter), scheduler_(scheduler) {}

void WoleixProtocolHandler::execute(const std::vector<WoleixCommand>& commands,
                                    std::function<void()> on_complete) {
    if (commands.empty())
    {
        if (on_complete) on_complete();
        return;
    }

    ESP_LOGD(TAG, "Executing %zu commands", commands.size());
    
    command_queue_ = commands;
    current_command_index_ = 0;
    on_complete_callback_ = on_complete;
    
    process_next_command_();
}

void WoleixProtocolHandler::process_next_command_()
{
    if (current_command_index_ >= command_queue_.size())
    {
        // All commands processed
        ESP_LOGD(TAG, "All commands executed");
        command_queue_.clear();
        if (on_complete_callback_)
        {
            on_complete_callback_();
            on_complete_callback_ = nullptr;
        }
        return;
    }

    const auto& cmd = command_queue_[current_command_index_];
    
    if (is_temp_command_(cmd))
    {
        handle_temp_command_(cmd);
    }
    else
    {
        handle_regular_command_(cmd);
    }
}

void WoleixProtocolHandler::handle_temp_command_(const WoleixCommand& cmd)
{
    switch (temp_state_)
    {
        case TempProtocolState::IDLE:
            // Need to enter setting mode first
            enter_setting_mode_(cmd);
            break;
            
        case TempProtocolState::SETTING_ACTIVE:
            // Already in setting mode, send directly
            ESP_LOGD(TAG, "In setting mode, sending temp command directly");
            transmit_(cmd);
            extend_setting_mode_timeout_();
            
            current_command_index_++;
            
            // Schedule next command with delay
            scheduler_->schedule_timeout(TIMEOUT_NEXT_COMMAND, INTER_COMMAND_DELAY_MS,
                [this]() { process_next_command_(); });
            break;
    }
}

void WoleixProtocolHandler::enter_setting_mode_(const WoleixCommand& cmd)
{
    ESP_LOGD(TAG, "Entering temperature setting mode");
    
    // Send the command (first press enters setting mode)
    transmit_(cmd);
    temp_state_ = TempProtocolState::SETTING_ACTIVE;
    
    // Start the setting mode timeout
    extend_setting_mode_timeout_();
    
    current_command_index_++;
    
    // Wait for AC to enter setting mode, then continue with remaining commands
    scheduler_->schedule_timeout(TIMEOUT_NEXT_COMMAND, TEMP_ENTER_DELAY_MS,
        [this]() { process_next_command_(); });
}

void WoleixProtocolHandler::handle_regular_command_(const WoleixCommand& cmd)
{
    ESP_LOGD(TAG, "Sending regular command");
    transmit_(cmd);
    
    current_command_index_++;
    
    // Use the command's built-in delay, or a default
    uint32_t delay = cmd.get_delay_ms() > 0 ? cmd.get_delay_ms() : INTER_COMMAND_DELAY_MS;
    
    scheduler_->schedule_timeout(TIMEOUT_NEXT_COMMAND, delay,
        [this]() { process_next_command_(); });
}

void WoleixProtocolHandler::extend_setting_mode_timeout_()
{
    scheduler_->cancel_timeout(TIMEOUT_SETTING_MODE);
    scheduler_->schedule_timeout(TIMEOUT_SETTING_MODE, TEMP_SETTING_MODE_TIMEOUT_MS,
        [this]() { on_setting_mode_timeout_(); });
}

void WoleixProtocolHandler::on_setting_mode_timeout_()
{
    ESP_LOGD(TAG, "Temperature setting mode timed out");
    temp_state_ = TempProtocolState::IDLE;
}

void WoleixProtocolHandler::reset()
{
    ESP_LOGD(TAG, "Resetting protocol handler");
    
    scheduler_->cancel_timeout(TIMEOUT_SETTING_MODE);
    scheduler_->cancel_timeout(TIMEOUT_NEXT_COMMAND);
    
    temp_state_ = TempProtocolState::IDLE;
    command_queue_.clear();
    current_command_index_ = 0;
    on_complete_callback_ = nullptr;
}

bool WoleixProtocolHandler::is_temp_command_(const WoleixCommand& cmd)
{
    return cmd.get_type() == WoleixCommand::Type::TEMP_UP ||
           cmd.get_type() == WoleixCommand::Type::TEMP_DOWN;
}

void WoleixProtocolHandler::transmit_(const WoleixCommand& cmd)
{
    ESP_LOGD(TAG, "TX: command type %d", static_cast<int>(cmd.get_type()));
    transmitter_->transmit_(cmd);
}

}  // namespace climate_ir_woleix
}  // namespace esphome */