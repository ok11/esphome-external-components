#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"

#include "climate_ir_woleix.h"

namespace esphome
{
namespace climate_ir_woleix
{

using ::esphome::climate::ClimateMode;

static const char *const TAG = "climate_ir_woleix.climate";

// Decoded timings from Pronto codes (in microseconds)
// Carrier frequency: 38.03 kHz

// Power button timing
static const ::std::vector<int32_t> POWER_TIMINGS = {
    5568, 2832, 528, 600, 528, 600, 528, 1680, 528, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
    528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 600, 528, 1680, 528, 600,
    528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600,
    528, 1704, 516, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
};

// Temperature Up timing
static const ::std::vector<int32_t> TEMP_UP_TIMINGS = {
    5568, 2832, 516, 600, 528, 576, 528, 1704, 516, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
    528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 600,
    528, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 600,
    528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
};

// Temperature Down timing
static const ::std::vector<int32_t> TEMP_DOWN_TIMINGS = {
    5568, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
    528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680,
    528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
};

// Mode button timing
static const ::std::vector<int32_t> MODE_TIMINGS = {
    5592, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
    516, 1704, 516, 1704, 516, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
    516, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1704, 516, 600,
    528, 600, 528, 1704, 516, 1704, 516, 1704, 516, 1680, 528, 18780
};

// Speed/Fan button timing
static const ::std::vector<int32_t> SPEED_TIMINGS = {
    5568, 2832, 516, 600, 528, 600, 528, 1656, 528, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1632, 552, 1704,
    516, 1704, 516, 1560, 624, 1632, 552, 600, 528, 600, 528, 1584, 600, 1704,
    516, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1704, 516, 600,
    528, 600, 528, 1704, 516, 1704, 516, 1656, 528, 1704, 516, 1704, 516, 18780
};

// Timer button timing
static const ::std::vector<int32_t> TIMER_TIMINGS = {
    5592, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
    516, 1704, 516, 1704, 516, 1680, 528, 600, 528, 600, 528, 600, 528, 600,
    528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680, 528, 1680,
    528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
};

void WoleixClimate::transmit_state()
{
  // Handle power on/off
  if (this->mode == ClimateMode::CLIMATE_MODE_OFF)
  {
    if (last_mode_.has_value() && last_mode_.value() != ClimateMode::CLIMATE_MODE_OFF)
    {
      // Turning off - send power command
      encode_power_();
      last_mode_ = ClimateMode::CLIMATE_MODE_OFF;
    }
    return;
  }

  // Handle power on (if coming from off state)
  if (!last_mode_.has_value() || last_mode_.value() == ClimateMode::CLIMATE_MODE_OFF)
  {
    encode_power_();
    delay(100); // Small delay between commands
  }

  // Handle mode changes
  if (!last_mode_.has_value() || last_mode_.value() != this->mode)
  {
    // Mode changed - send mode command
    // Note: You may need to send mode command multiple times to cycle through modes
    encode_mode_();
    delay(100);
    last_mode_ = this->mode;
  }

  // Handle temperature changes
  if (last_target_temp_.has_value())
  {
    float temp_diff = this->target_temperature - last_target_temp_.value();

    if (temp_diff > 0)
    {
      // Temperature increased
      int steps = (int)(temp_diff / this->temperature_step_);
      for (int i = 0; i < steps; i++)
      {
        encode_temp_up_();
        delay(100);
      }
    }
    else if (temp_diff < 0)
    {
      // Temperature decreased
      int steps = (int)(-temp_diff / this->temperature_step_);
      for (int i = 0; i < steps; i++)
      {
        encode_temp_down_();
        delay(100);
      }
    }
  }
  last_target_temp_ = this->target_temperature;

  // Handle fan mode changes
  if (!last_fan_mode_.has_value() || last_fan_mode_.value() != this->fan_mode.value())
  {
    encode_speed_();
    delay(100);
    last_fan_mode_ = this->fan_mode.value();
  }

  ESP_LOGD(TAG, "Transmitted climate state - Mode: %d, Temp: %.1f, Fan: %d",
            this->mode, this->target_temperature, this->fan_mode.value());
}

void WoleixClimate::encode_power_()
{
  ESP_LOGD(TAG, "Sending Power command");
  transmit_raw_(POWER_TIMINGS);
}

void WoleixClimate::encode_temp_up_()
{
  ESP_LOGD(TAG, "Sending Temp+ command");
  transmit_raw_(TEMP_UP_TIMINGS);
}

void WoleixClimate::encode_temp_down_()
{
  ESP_LOGD(TAG, "Sending Temp- command");
  transmit_raw_(TEMP_DOWN_TIMINGS);
}

void WoleixClimate::encode_mode_()
{
  ESP_LOGD(TAG, "Sending Mode command");
  transmit_raw_(MODE_TIMINGS);
}

void WoleixClimate::encode_speed_()
{
  ESP_LOGD(TAG, "Sending Speed/Fan command");
  transmit_raw_(SPEED_TIMINGS);
}

void WoleixClimate::encode_timer_()
{
  ESP_LOGD(TAG, "Sending Timer command");
  transmit_raw_(TIMER_TIMINGS);
}

void WoleixClimate::transmit_raw_(const ::std::vector<int32_t> &timings)
{
  auto transmit = this->transmitter_->transmit();
  auto data = transmit.get_data();

  data->set_carrier_frequency(38030); // 38.03 kHz carrier

  // Add all timing values
  for (size_t i = 0; i < timings.size(); i++)
  {
    if (i % 2 == 0)
    {
      data->mark(timings[i]); // Mark (IR on)
    }
    else
    {
      data->space(timings[i]); // Space (IR off)
    }
  }

  transmit.perform();
}

::esphome::climate::ClimateTraits WoleixClimate::traits()
{
  auto traits = ::esphome::climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_current_humidity(true);
  traits.set_visual_min_temperature(WOLEIX_TEMP_MIN);
  traits.set_visual_max_temperature(WOLEIX_TEMP_MAX);
  traits.set_visual_temperature_step(1.0f);
  return traits;
}

}
}
