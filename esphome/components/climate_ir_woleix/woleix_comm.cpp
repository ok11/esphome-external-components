#include <ranges>
#include <algorithm>
#include <cmath>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "woleix_comm.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::NECData;
using remote_base::NECProtocol;

/**
 * Transmit a single Woleix command via NEC protocol.
 * 
 * Converts the WoleixCommand into NEC protocol format and sends it through
 * the IR transmitter. The command is repeated according to the repeat_count
 * parameter, with appropriate delays between transmissions.
 * 
 * @param command The command to transmit
 */
void WoleixTransmitter::transmit_(const WoleixCommand& command)
{
    NECData nec_data;
    nec_data.address = command.get_address();
    nec_data.command = command.get_command();
    nec_data.command_repeats = 1; 
    uint16_t delay_ms = command.get_delay_ms();

    ESP_LOGD
    (
        TAG,
        "Transmitting NEC command: " 
            "address=%#04x, "
            "code=%#04x, "
            "repeats=%u, "
            "send_times=%u, "
            "send_wait=%u",
        nec_data.address,
        nec_data.command,
        nec_data.command_repeats,
        command.get_repeat_count(),
        command.get_delay_ms()
    );

    transmitter_->transmit<NECProtocol>(nec_data, command.get_repeat_count(), delay_ms * 1000);
}

}  // namespace climate_ir_woleix
}  // namespace esphome
