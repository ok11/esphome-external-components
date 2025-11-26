#pragma once

#include <cinttypes>
#include <vector>
#include <string>
#include <map>
#include <memory>

#include "esphome/core/optional.h"

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/remote_base/pronto_protocol.h"
#include "woleix_ac_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

using climate::ClimateMode;
using climate::ClimateFanMode;
using climate::ClimateTraits;
using climate_ir::ClimateIR;

const float_t WOLEIX_TEMP_MIN = 15.0f;          // Celsius
const float_t WOLEIX_TEMP_MAX = 30.0f;          // Celsius

// Carrier frequency: 38.03 kHz (0x006D)
// Pronto hex codes for IR commands

// Power button
static const std::string POWER_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0018 0014 0043 0013 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Temperature Up
static const std::string TEMP_UP_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0017 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Temperature Down
static const std::string TEMP_DOWN_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Mode button
static const std::string MODE_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0043 0013 0018 0014 0018 0014 0043 0013 0043 0013 0043 0013 0042 "
    "0014 0483";

// Speed/Fan button
static const std::string SPEED_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0018 0014 0041 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0040 0016 0043 0013 0043 0013 003D 0019 0040 0015 0018 0014 003E "
    "0018 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 "
    "0013 0018 0014 0018 0014 0043 0013 0043 0013 0041 0014 0043 0013 0043 "
    "0013 0483";

// Timer button
static const std::string TIMER_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0042 0014 0018 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

class WoleixClimate : public ClimateIR {

public:
    WoleixClimate();
    WoleixClimate(WoleixACStateMachine* state_machine);
    
    /// Set the humidity sensor for current humidity readings
    void set_humidity_sensor(sensor::Sensor *humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
    
    /// Setup method called once - initializes humidity sensor callback
    void setup() override;

protected:    
    // Transmit via IR the state of this climate controller.
    void transmit_state() override;
    
    ClimateTraits traits() override;
    
    /// Transmit all queued Pronto IR commands with delays between transmissions
    virtual void transmit_commands_();
    
    /// Queue a Pronto hex command for later transmission
    /// @param pronto_hex Pronto format IR command string
    virtual void enqueue_command_(const std::string& pronto_hex);
    
    /// Reset state machine to defaults
    virtual void reset_state_();
    
    /// Map ESPHome ClimateMode to WoleixMode
    WoleixMode map_climate_mode_(ClimateMode climate_mode);
    
    /// Map ESPHome ClimateFanMode to WoleixFanSpeed
    WoleixFanSpeed map_fan_mode_(ClimateFanMode fan_mode);

    WoleixACStateMachine *state_machine_{nullptr};
    sensor::Sensor *humidity_sensor_{nullptr};
    std::vector<std::string> commands_;
};

}  // namespace climate_ir_woleix
}  // namespace esphome
