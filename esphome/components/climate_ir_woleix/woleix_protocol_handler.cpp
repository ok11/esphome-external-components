#include <ranges>
#include <algorithm>
#include <cmath>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/remote_base/nec_protocol.h"

#include "woleix_protocol_handler.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::NECData;
using remote_base::NECProtocol;

void WoleixProtocolHandler::process()
{
    process_(nullptr);
}

void WoleixProtocolHandler::process_(std::function<void()> on_complete)
{
    if (command_queue_->is_empty())
    {
        if (on_complete) on_complete();
        return;
    }

    ESP_LOGD(TAG, "Executing %u commands", command_queue_->length());
    
    on_complete_ = on_complete;
    
    process_next_command_();
}

void WoleixProtocolHandler::process_next_command_()
{
    if (command_queue_->is_empty())
    {
        // All commands processed
        ESP_LOGD(TAG, "All commands executed");
        if (on_complete_)
        {
            on_complete_();
            on_complete_ = nullptr;
        }
        set_timeout_(TIMEOUT_NEXT_COMMAND, POLL_COMMAND_INTERVAL_MS,
            [this]() { process_next_command_(); });
    }

    const auto& cmd = command_queue_->dequeue();
    
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
                        
            // Schedule next command with delay
            set_timeout_(TIMEOUT_NEXT_COMMAND, INTER_COMMAND_DELAY_MS, 
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
    
    // Wait for AC to enter setting mode, then continue with remaining commands
    set_timeout_(TIMEOUT_NEXT_COMMAND, TEMP_ENTER_DELAY_MS, 
        [this]() { process_next_command_(); });
}

void WoleixProtocolHandler::handle_regular_command_(const WoleixCommand& cmd)
{
    ESP_LOGD(TAG, "Sending regular command");
    transmit_(cmd);
    
    set_timeout_(TIMEOUT_NEXT_COMMAND, INTER_COMMAND_DELAY_MS, 
        [this]() { process_next_command_(); });
}

void WoleixProtocolHandler::extend_setting_mode_timeout_()
{
    cancel_timeout_(TIMEOUT_SETTING_MODE);
    set_timeout_(TIMEOUT_SETTING_MODE, TEMP_SETTING_MODE_TIMEOUT_MS,
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
    
    cancel_timeout_(TIMEOUT_SETTING_MODE);
    cancel_timeout_(TIMEOUT_NEXT_COMMAND);    
    
    temp_state_ = TempProtocolState::IDLE;
    on_complete_ = nullptr;

    set_timeout_(TIMEOUT_NEXT_COMMAND, POLL_COMMAND_INTERVAL_MS,
        [this]() { process_next_command_(); });

}

bool WoleixProtocolHandler::is_temp_command_(const WoleixCommand& cmd)
{
    return cmd.get_type() == WoleixCommand::Type::TEMP_UP ||
           cmd.get_type() == WoleixCommand::Type::TEMP_DOWN;
}

/**
 * Transmit a single Woleix command via NEC protocol.
 * 
 * Converts the WoleixCommand into NEC protocol format and sends it through
 * the IR transmitter. The command is repeated according to the repeat_count
 * parameter, with appropriate delays between transmissions.
 * 
 * @param command The command to transmit
 */
void WoleixProtocolHandler::transmit_(const WoleixCommand& command)
{
    NECData nec_data;
    nec_data.address = command.get_address();
    nec_data.command = command.get_command();
    nec_data.command_repeats = 1; 

    ESP_LOGD
    (
        TAG,
        "Transmitting NEC command: " 
            "address=%#04x, "
            "code=%#04x, "
            "repeats=%u, "
            "send_times=%u",
        nec_data.address,
        nec_data.command,
        nec_data.command_repeats,
        command.get_repeat_count()
    );

    transmitter_->transmit<NECProtocol>(nec_data, command.get_repeat_count(), 0);
}

}  // namespace climate_ir_woleix
}  // namespace esphome
