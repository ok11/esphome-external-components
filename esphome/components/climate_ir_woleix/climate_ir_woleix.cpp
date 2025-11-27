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

WoleixClimate::WoleixClimate()
    : WoleixClimate(new WoleixACStateMachine())
{
    this->target_temperature = WOLEIX_TEMP_DEFAULT;
    this->mode = climate::CLIMATE_MODE_OFF;
    this->fan_mode = climate::CLIMATE_FAN_LOW;
}

WoleixClimate::WoleixClimate(WoleixACStateMachine *state_machine)
    : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX)
{
    target_temperature = WOLEIX_TEMP_DEFAULT;
    mode = climate::CLIMATE_MODE_OFF;
    fan_mode = climate::CLIMATE_FAN_LOW;
    state_machine_ = state_machine;
}

void WoleixClimate::reset_state()
{
    state_machine_->reset();
}

void WoleixClimate::setup()
{
  // Call parent setup first
  ClimateIR::setup();
  
  // Set up callback to update humidity from sensor
  if (humidity_sensor_ != nullptr)
  {
    humidity_sensor_->add_on_state_callback([this](float state)
    {
      if (!std::isnan(state))
      {
        current_humidity = state;
        publish_state();
        ESP_LOGD(TAG, "Updated humidity: %.1f%%", state);
      }
      else
      {
        ESP_LOGW(TAG, "Received NaN humidity reading");
      }
    });
  }
}


void WoleixClimate::transmit_state()
{
  // Map ESPHome Climate states to Woleix AC states
  WoleixPowerState woleix_power = 
    (mode == ClimateMode::CLIMATE_MODE_OFF)
      ? WoleixPowerState::OFF
      : WoleixPowerState::ON;
  
  WoleixMode woleix_mode = map_climate_mode_(mode);
  WoleixFanSpeed woleix_fan_speed = map_fan_mode_(fan_mode.value());
  
  // Generate command sequence via state machine
  state_machine_->set_target_state(woleix_power, woleix_mode, target_temperature, woleix_fan_speed);
  
  // Get commands from state machine and transmit them
  auto commands = state_machine_->get_commands();
  if (!commands.empty())
  {
    for (const auto& cmd : commands)
    {
      enqueue_command_(cmd);
    }
  
    // Send them over the IR transmitter
    transmit_commands_();
  
    ESP_LOGD(TAG, "Transmitted climate state - Mode: %d, Temp: %.1f, Fan: %d",
           static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));
  
    // Publish the new state
    publish_state();
  }
  else
  {
    ESP_LOGW(TAG, "No commands generated to reach the target state");
  }
}

void WoleixClimate::enqueue_command_(const WoleixCommand& command)
{
  this->commands_.push_back(command);
}

void WoleixClimate::transmit_commands_()
{
  for(const WoleixCommand& command: this->commands_)
  {
    // Create ProntoData from the hex string
    remote_base::ProntoData pronto_data;
    pronto_data.data = command.pronto_hex;
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
    delay(command.delay_ms); // Small delay between commands
  }
  this->commands_.clear();
}

ClimateTraits WoleixClimate::traits()
{
  auto traits = ClimateTraits();

  traits.set_supported_modes({
    ClimateMode::CLIMATE_MODE_OFF,
    ClimateMode::CLIMATE_MODE_COOL,
    ClimateMode::CLIMATE_MODE_DRY,
    ClimateMode::CLIMATE_MODE_FAN_ONLY
  });

  traits.set_supported_fan_modes({
    ClimateFanMode::CLIMATE_FAN_LOW,
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

WoleixMode WoleixClimate::map_climate_mode_(ClimateMode climate_mode)
{
  switch (climate_mode)
  {
    case ClimateMode::CLIMATE_MODE_COOL:
      return WoleixMode::COOL;
    case ClimateMode::CLIMATE_MODE_DRY:
      return WoleixMode::DEHUM;
    case ClimateMode::CLIMATE_MODE_FAN_ONLY:
      return WoleixMode::FAN;
    default:
      // Default to COOL for any other mode
      return WoleixMode::COOL;
  }
}

WoleixFanSpeed WoleixClimate::map_fan_mode_(ClimateFanMode fan_mode)
{
  switch (fan_mode)
  {
    case ClimateFanMode::CLIMATE_FAN_LOW:
      return WoleixFanSpeed::LOW;
    case ClimateFanMode::CLIMATE_FAN_MEDIUM:
    case ClimateFanMode::CLIMATE_FAN_HIGH:
      return WoleixFanSpeed::HIGH;
    default:
      return WoleixFanSpeed::LOW;
  }
}

}
}
