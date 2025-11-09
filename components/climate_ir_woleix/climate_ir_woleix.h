#pragma once

#include <cinttypes>
#include <vector>

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace climate_ir_woleix {

const float_t WOLEIX_TEMP_MIN = 15.0f;          // Celsius
const float_t WOLEIX_TEMP_MAX = 30.0f;          // Celsius

class WoleixClimate : public climate_ir::ClimateIR {

public:
    WoleixClimate::WoleixClimate()
        : climate_ir::ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX)
    {}

protected :
    // Transmit via IR the state of this climate controller.
    void transmit_state() override;

    climate::ClimateTraits traits() override;

    /// Encode power command
    void encode_power_();
    
    /// Encode temperature up command
    void encode_temp_up_();
    
    /// Encode temperature down command
    void encode_temp_down_();
    
    /// Encode mode command
    void encode_mode_();
    
    /// Encode fan speed command
    void encode_speed_();
    
    /// Encode timer command
    void encode_timer_();
    
    /// Helper to transmit raw timing data
    void transmit_raw_(const ::std::vector<int32_t>& timings);
    
    /// Store last transmitted state to minimize IR commands
    optional<climate::ClimateMode> last_mode_{};
    optional<float> last_target_temp_{};
    optional<climate::ClimateFanMode> last_fan_mode_{};
};

}
}
