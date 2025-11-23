#pragma once

#include <cinttypes>
#include <vector>

#include "esphome/core/optional.h"

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace climate_ir_woleix {

const float_t WOLEIX_TEMP_MIN = 15.0f;          // Celsius
const float_t WOLEIX_TEMP_MAX = 30.0f;          // Celsius
const float_t WOLEIX_TEMP_DEFAULT = 25.0f;      // Celsius


using climate::ClimateMode;
using climate::ClimateFanMode;
using climate::ClimateTraits;
using climate_ir::ClimateIR;

class WoleixClimate : public ClimateIR {

public:
    WoleixClimate()
        : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX, 1.0f, 0.5f, 0.5f)
    {
        this->target_temperature = WOLEIX_TEMP_DEFAULT;
        this->mode = climate::CLIMATE_MODE_COOL;
        this->fan_mode = climate::CLIMATE_FAN_AUTO;
    }
    
    /// Set the humidity sensor for current humidity readings
    void set_humidity_sensor(sensor::Sensor *humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
    
    /// Setup method called once - initializes humidity sensor callback
    void setup() override;

protected :
    // Transmit via IR the state of this climate controller.
    void transmit_state() override;
    
    ClimateTraits traits() override;

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
    optional<ClimateMode> last_mode_{};
    optional<float> last_target_temp_{};
    optional<ClimateFanMode> last_fan_mode_{};
    
    /// Humidity sensor for current humidity readings
    sensor::Sensor *humidity_sensor_{nullptr};
    float current_humidity_;
};

}
}
