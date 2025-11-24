#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/remote_base/pronto_protocol.h"

#include "climate_ir_woleix.h"

namespace esphome
{
namespace climate_ir_woleix
{

using esphome::climate::ClimateMode;
using esphome::climate::ClimateFanMode;
using esphome::climate::ClimateFeature;
using esphome::climate::ClimateTraits;

static const char *const TAG = "climate_ir_woleix.climate";

// Pronto hex codes for IR commands (from yaml configuration)
// Carrier frequency: 38.03 kHz (0x006D)

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

void WoleixClimate::setup()
{
  // Call parent setup first
  ClimateIR::setup();
  
  // Initialize with default values
  this->mode = ClimateMode::CLIMATE_MODE_OFF;
  this->target_temperature = WOLEIX_TEMP_DEFAULT;
  this->fan_mode = climate::CLIMATE_FAN_LOW;

  // Set up callback to update humidity from sensor
  if (this->humidity_sensor_ != nullptr)
  {
    this->humidity_sensor_->add_on_state_callback([this](float state) {
      if (!std::isnan(state)) {
        this->current_humidity = state;
        this->publish_state();
        ESP_LOGD(TAG, "Updated humidity: %.1f%%", state);
      } else {
        ESP_LOGW(TAG, "Received NaN humidity reading");
      }
    });
  }
  
  // Transmit initial state
  this->transmit_state();
}

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
  
  // Publish the new state
  this->publish_state();
}

void WoleixClimate::encode_power_()
{
  ESP_LOGD(TAG, "Sending Power command");
  transmit_pronto_(POWER_PRONTO);
}

void WoleixClimate::encode_temp_up_()
{
  ESP_LOGD(TAG, "Sending Temp+ command");
  transmit_pronto_(TEMP_UP_PRONTO);
}

void WoleixClimate::encode_temp_down_()
{
  ESP_LOGD(TAG, "Sending Temp- command");
  transmit_pronto_(TEMP_DOWN_PRONTO);
}

void WoleixClimate::encode_mode_()
{
  ESP_LOGD(TAG, "Sending Mode command");
  transmit_pronto_(MODE_PRONTO);
}

void WoleixClimate::encode_speed_()
{
  ESP_LOGD(TAG, "Sending Speed/Fan command");
  transmit_pronto_(SPEED_PRONTO);
}

void WoleixClimate::encode_timer_()
{
  ESP_LOGD(TAG, "Sending Timer command");
  transmit_pronto_(TIMER_PRONTO);
}

void WoleixClimate::transmit_pronto_(const std::string& pronto_hex)
{
  // Create ProntoData from the hex string
  remote_base::ProntoData pronto_data;
  pronto_data.data = pronto_hex;
  pronto_data.delta = 0;
  
  // Transmit using ProntoProtocol
  auto call = this->transmitter_->transmit();
  auto data = call.get_data();
  
  // Set carrier frequency (38.03 kHz)
  data->set_carrier_frequency(38030);
  
  // Encode the Pronto data
  remote_base::ProntoProtocol().encode(data, pronto_data);
  
  // Perform the transmission
  call.perform();
}

ClimateTraits WoleixClimate::traits()
{
  auto traits = ClimateTraits();

  traits.set_supported_modes({
    ClimateMode::CLIMATE_MODE_OFF,
    ClimateMode::CLIMATE_MODE_COOL
  });

  traits.set_supported_fan_modes({
    ClimateFanMode::CLIMATE_FAN_LOW,
    ClimateFanMode::CLIMATE_FAN_MEDIUM,
    ClimateFanMode::CLIMATE_FAN_HIGH
  });

  traits.set_supported_swing_modes({});

  traits.add_feature_flags(ClimateFeature::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(ClimateFeature::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);

  traits.set_visual_min_temperature(WOLEIX_TEMP_MIN);
  traits.set_visual_max_temperature(WOLEIX_TEMP_MAX);
  traits.set_visual_temperature_step(1.0f);

  return traits;
}

}
}
