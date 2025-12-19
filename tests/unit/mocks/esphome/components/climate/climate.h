#pragma once

#include <optional>
#include "esphome/components/climate/climate_mode.h"

namespace esphome
{
namespace climate
{

// Mock ClimateCall class
class ClimateCall {
public:
    ClimateCall& set_mode(ClimateMode mode) { mode_ = mode; return *this; }
    ClimateCall& set_target_temperature(float temp) { target_temperature_ = temp; return *this; }
    ClimateCall& set_fan_mode(ClimateFanMode fan_mode) { fan_mode_ = fan_mode; return *this; }

    std::optional<ClimateMode> get_mode() const { return mode_; }
    std::optional<float> get_target_temperature() const { return target_temperature_; }
    std::optional<ClimateFanMode> get_fan_mode() const { return fan_mode_; }

private:
    std::optional<ClimateMode> mode_;
    std::optional<float> target_temperature_;
    std::optional<ClimateFanMode> fan_mode_;
};

class Climate {
public:
    virtual ~Climate() = default;
    virtual ClimateCall make_call() {
      return ClimateCall();
    }
    virtual void control(const ClimateCall &call) = 0;
};
}
}