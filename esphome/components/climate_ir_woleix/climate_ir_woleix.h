#pragma once

#include <cinttypes>
#include <vector>
#include <string>

#include "esphome/core/optional.h"

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_base/pronto_protocol.h"

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
        : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX)
    {
        this->target_temperature = WOLEIX_TEMP_DEFAULT;
        this->mode = climate::CLIMATE_MODE_OFF;
        this->fan_mode = climate::CLIMATE_FAN_LOW;
    }
    
    /// Set the humidity sensor for current humidity readings
    void set_humidity_sensor(sensor::Sensor *humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
    
    /// Setup method called once - initializes humidity sensor callback
    void setup() override;

protected :
    // Transmit via IR the state of this climate controller.
    void transmit_state() override;
    
    ClimateTraits traits() override;

    /// Transmit power command
    virtual void transmit_power_();
    
    /// Transmit temperature up command
    virtual void transmit_temp_up_();
    
    /// Transmit temperature down command
    virtual void transmit_temp_down_();
    
    /// Transmit mode command
    virtual void transmit_mode_();
    
    /// Transmit fan speed command
    virtual void transmit_speed_();
    
    /// Transmit timer command
    virtual void transmit_timer_();
    
    /// Helper to transmit Pronto data
    virtual void transmit_pronto_(const std::string& pronto_hex);
    
    /// Store last transmitted state to minimize IR commands
    optional<ClimateMode> last_mode_{};
    optional<float> last_target_temp_{};
    optional<ClimateFanMode> last_fan_mode_{};
    
    /// Humidity sensor for current humidity readings
    sensor::Sensor *humidity_sensor_{nullptr};
};

}
}
