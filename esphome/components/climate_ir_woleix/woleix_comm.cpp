#include <ranges>
#include <algorithm>
#include <cmath>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "woleix_comm.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::ProntoData;
using remote_base::NECData;
using remote_base::ProntoProtocol;
using remote_base::NECProtocol;

void WoleixCommandTransmitter::operator()(const WoleixNecCommand& command)
{
    NECData nec_data;
    nec_data.address = command.get_address();
    nec_data.command = command.get_command_code();
    nec_data.command_repeats = 1; 
    uint16_t delay_ms = command.get_delay_ms();

    ESP_LOGD
    (
        TAG,
        "Transmitting NEC command: " 
            "address=%#04x, "
            "code=%#04x, "
            "repeats=%d", 
        nec_data.address,
        nec_data.command,
        nec_data.command_repeats
    );

    transmitter_->transmit<NECProtocol>(nec_data, command.get_repeat_count(), delay_ms);
}

}  // namespace climate_ir_woleix
}  // namespace esphome

