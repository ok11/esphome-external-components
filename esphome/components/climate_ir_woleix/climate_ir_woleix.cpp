#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include "esphome/components/remote_base/pronto_protocol.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate.h"

#include "climate_ir_woleix.h"

namespace esphome
{
namespace climate_ir_woleix
{

using esphome::climate::ClimateMode;
using esphome::climate::ClimateFanMode;
using esphome::climate::ClimateFeature;
using esphome::climate::ClimateTraits;

WoleixClimate::WoleixClimate()
    : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX),
      state_machine_(new WoleixStateMachine()),
      command_transmitter_(new WoleixTransmitter(nullptr))
{
    target_temperature = WOLEIX_TEMP_DEFAULT;
    mode = climate::CLIMATE_MODE_OFF;
    fan_mode = climate::CLIMATE_FAN_LOW;
}

WoleixClimate::WoleixClimate(WoleixStateMachine *state_machine, WoleixTransmitter *command_transmitter)
    : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX),
      state_machine_(state_machine),
      command_transmitter_(command_transmitter)
{
}

void WoleixClimate::reset_state()
{
  state_machine_->reset();
}

void WoleixClimate::setup()
{
  // Print out current version
  ESP_LOGI(TAG, "Version: %s", VERSION);

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

const std::vector<WoleixCommand>& WoleixClimate::calculate_commands_()
{
  // Map ESPHome Climate states to Woleix AC states
  WoleixPowerState woleix_power = StateMapper::esphome_to_woleix_power(mode != ClimateMode::CLIMATE_MODE_OFF);
  WoleixMode woleix_mode = StateMapper::esphome_to_woleix_mode(mode);
  WoleixFanSpeed woleix_fan_speed = StateMapper::esphome_to_woleix_fan_mode(fan_mode.value());
  
  // Generate command sequence via state machine
  return state_machine_->transit_to_state(woleix_power, woleix_mode, target_temperature, woleix_fan_speed);
}

void WoleixClimate::update_state_()
{
    // Sync internal state with state machine
    auto current_state = state_machine_->get_state();
    mode = StateMapper::woleix_to_esphome_power(current_state.power) 
            ? StateMapper::woleix_to_esphome_mode(current_state.mode)
            : ClimateMode::CLIMATE_MODE_OFF;
    target_temperature = current_state.temperature;
    fan_mode = StateMapper::woleix_to_esphome_fan_mode(current_state.fan_speed);
    
    ESP_LOGD(TAG, "Synced internal state - Mode: %d, Temp: %.1f, Fan: %d",
           static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));
}

void WoleixClimate::transmit_state()
{
  ESP_LOGD(TAG, "Transmitting state - Mode: %d, Temp: %.1f, Fan: %d",
    static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));

  auto commands = calculate_commands_();

  if (!commands.empty())
  {
    // Send them over the IR transmitter
    transmit_commands_(commands);

    ESP_LOGD(TAG, "Transmitted climate state - Mode: %d, Temp: %.1f, Fan: %d",
      static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));
  
    update_state_();
  }
  else
  {
    ESP_LOGI(TAG, "No commands generated to reach the target state");
  }
}

void WoleixClimate::transmit_commands_(std::vector<WoleixCommand>& commands)
{
    if (command_transmitter_->get_transmitter() == nullptr) {
        command_transmitter_->set_transmitter(transmitter_);
    }
    command_transmitter_->transmit_(commands);
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
