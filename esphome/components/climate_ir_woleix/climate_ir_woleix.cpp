#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/remote_base/pronto_protocol.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate.h"

#include "climate_ir_woleix.h"
#include "state_mapper.h"

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
    target_temperature = WOLEIX_TEMP_DEFAULT;
    mode = climate::CLIMATE_MODE_OFF;
    fan_mode = climate::CLIMATE_FAN_LOW;
}

WoleixClimate::WoleixClimate(WoleixACStateMachine *state_machine)
    : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX)
{
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
  if (reset_button_ != nullptr)
  {
    reset_button_->add_on_state_callback([this](bool state)
    {
      if (state)
      {
        ESP_LOGI(TAG, "Reset button pressed - resetting the internal state");
        reset_state();
        publish_state();
      }
    });
  }
}


void WoleixClimate::transmit_state()
{
  ESP_LOGD(TAG, "Transmitting state - Mode: %d, Temp: %.1f, Fan: %d",
           static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));

  // Map ESPHome Climate states to Woleix AC states
  WoleixPowerState woleix_power = StateMapper::esphome_to_woleix_power(mode != ClimateMode::CLIMATE_MODE_OFF);
  WoleixMode woleix_mode = StateMapper::esphome_to_woleix_mode(mode);
  WoleixFanSpeed woleix_fan_speed = StateMapper::esphome_to_woleix_fan_mode(fan_mode.value());
  
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
  
    // Sync internal state with state machine
    auto current_state = state_machine_->get_state();
    this->mode = StateMapper::woleix_to_esphome_power(current_state.power) ? StateMapper::woleix_to_esphome_mode(current_state.mode) : ClimateMode::CLIMATE_MODE_OFF;
    this->target_temperature = current_state.temperature;
    this->fan_mode = StateMapper::woleix_to_esphome_fan_mode(current_state.fan_speed);
    
    ESP_LOGD(TAG, "Synced internal state - Mode: %d, Temp: %.1f, Fan: %d",
           static_cast<int>(this->mode), this->target_temperature, static_cast<int>(this->fan_mode.value()));

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
    for (const WoleixSequence& sequence: command.sequences)
    {
      // Create ProntoData from the hex string
      remote_base::ProntoData pronto_data;
      pronto_data.data = sequence.pronto_hex;
      pronto_data.delta = 0;
      
      // Transmit using ProntoProtocol
      auto call = this->transmitter_->transmit();
      auto data = call.get_data();
      
      // Set carrier frequency (38.03 kHz)
      data->set_carrier_frequency(38030);
      
      // Encode the Pronto data
      remote_base::ProntoProtocol().encode(data, pronto_data);

      ESP_LOGD(TAG, "Transmitting %s", pronto_data.data.c_str());

      // Perform the transmission
      call.perform();

      ESP_LOGD(TAG, "Delay of %d", sequence.delay_ms);
      delay(sequence.delay_ms); // Small delay between commands
    }
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

}
}
