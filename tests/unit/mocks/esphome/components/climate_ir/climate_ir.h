#pragma once

#include <gmock/gmock.h>
#include <vector>
#include <memory>

#include "esphome/core/optional.h"
#include "esphome/core/log.h"

#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/remote_base/remote_base.h"

namespace esphome {
namespace climate_ir {

using climate::Climate;
using climate::ClimateMode;
using climate::ClimateFanMode;
using climate::ClimateSwingMode;
using climate::ClimateTraits;

// Mock ClimateIR base class
class ClimateIR: public Climate
{
public:
    ClimateIR(float min_temp, float max_temp)
      : temperature_step_(1.0f),
        min_temperature_(min_temp),
        max_temperature_(max_temp) {}
    
    virtual ~ClimateIR() = default;
    
    // Public state that child classes use
    ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
    float target_temperature = 20.0f;
    optional<ClimateFanMode> fan_mode;
    optional<ClimateSwingMode> swing_mode;
    
    // Setter for transmitter (used by tests)
    void set_transmitter(remote_base::RemoteTransmitterBase* transmitter) {
      transmitter_ = transmitter;
    }
    
    // Call transmit_state from tests
    void call_transmit_state() {
      transmit_state();
    }
    
    // Call traits from tests
    ClimateTraits call_traits()
    {
      return traits();
    }
    
    // Setup method (can be overridden)
    virtual void setup() {}
    
    void control(const climate::ClimateCall &call)
    {
      if (call.get_mode().has_value())
        this->mode = *call.get_mode();
      if (call.get_target_temperature().has_value())
        this->target_temperature = *call.get_target_temperature();
      if (call.get_fan_mode().has_value())
        this->fan_mode = *call.get_fan_mode();
      this->transmit_state();
      this->publish_state();
    }
    // Override these in derived classes
    virtual void transmit_state() = 0;
    virtual ClimateTraits traits() {
      return climate::ClimateTraits();
    }
    virtual void publish_state() {}
    // Protected members accessible to derived classes
    remote_base::RemoteTransmitterBase* transmitter_ = nullptr;
    float temperature_step_;
    float min_temperature_;
    float max_temperature_;

    float current_humidity{NAN};
};

} // namespace climate_ir
} // namespace esphome
