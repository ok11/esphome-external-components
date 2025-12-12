#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "woleix_comm.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::ProntoData;
using remote_base::NECData;
using remote_base::ProntoProtocol;
using remote_base::NECProtocol;

void WoleixCommandTransmitter::operator()(const WoleixProntoCommand& command)
{
    ProntoData pronto_data;
    pronto_data.data = command.get_pronto_hex();
    pronto_data.delta = 0;
    uint16_t repeats = command.get_repeat_count();
    uint16_t delay_ms = command.get_delay_ms();

    ESP_LOGD
    (
        TAG,
        "Transmitting Pronto command: " 
            "data=%s, "
            "delta=%d, "
            "repeats=%d", 
        pronto_data.data.c_str(),
        pronto_data.delta,
        repeats
    );

    transmitter_->transmit<ProntoProtocol>(pronto_data, repeats, delay_ms);
}

void WoleixCommandTransmitter::operator()(const WoleixNecCommand& command)
{
    NECData nec_data;
    nec_data.address = command.get_address();
    nec_data.command = command.get_command_code();
    nec_data.command_repeats = command.get_repeat_count(); 
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

    transmitter_->transmit<NECProtocol>(nec_data, nec_data.command_repeats, delay_ms);
}

}  // namespace climate_ir_woleix
}  // namespace esphome

