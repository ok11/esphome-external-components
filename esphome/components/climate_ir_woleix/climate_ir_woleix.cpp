#include <memory>

#include "esphome/core/log.h"
#include "esphome/core/hal.h"
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
    command_queue_(std::make_unique<WoleixCommandQueue>(QUEUE_MAX_CAPACITY)),
    WoleixStateMachine(),
    WoleixProtocolHandler
    (
        [this](const std::string& name, uint32_t delay_ms, std::function<void()> callback) {
            this->set_timeout(name, delay_ms, std::move(callback));
        },
        [this](const std::string& name) {
            this->cancel_timeout(name);
        }
    )
{
    command_queue_->register_listener(std::shared_ptr<WoleixListener>(std::shared_ptr<WoleixClimate>{}, this));
    reset_state();
}

// /**
//  * Constructor accepting state machine and transmitter.
//  * 
//  * Initializes the climate controller with default settings and creates
//  * internal state machine and protocol handler instances.
//  */
// WoleixClimate::WoleixClimate(std::unique_ptr<WoleixCommandQueue> command_queue)
//   : ClimateIR(WOLEIX_TEMP_MIN, WOLEIX_TEMP_MAX),
//     command_queue_(std::move(command_queue)),
//     WoleixStateMachine(command_queue_.get()),
//     WoleixProtocolHandler
//     (
//         command_queue_.get(),
//         [this](const std::string& name, uint32_t delay_ms, std::function<void()> callback) {
//             this->set_timeout(name, delay_ms, std::move(callback));
//         },
//         [this](const std::string& name) {
//             this->cancel_timeout(name);
//         }
//     )
// {
// }

/**
 * Reset the state machine to default values.
 * 
 * This method resets the internal state tracking to device defaults:
 * power=ON, mode=COOL, temperature=25°C, fan_speed=LOW.
 * 
 * Used for state reconciliation when the physical AC unit has been
 * power cycled and returned to its factory defaults.
 * 
 * Note: This does not transmit any IR commands.
 */
void WoleixClimate::reset_state()
{
    command_queue_->reset();

    WoleixStateMachine::reset();
    WoleixProtocolHandler::reset();
    
    target_temperature = WOLEIX_TEMP_DEFAULT;
    mode = climate::CLIMATE_MODE_OFF;
    fan_mode = climate::CLIMATE_FAN_LOW;
}

/**
 * Setup method called once during initialization.
 * 
 * Logs component version, calls parent ClimateIR::setup(), and initializes
 * the humidity sensor callback if configured. This allows the climate
 * controller to track and publish current humidity levels.
 */
void WoleixClimate::setup()
{
    // Print out current version
    ESP_LOGI(TAG, "Version: %s", VERSION);

    // Call parent setup first
    ClimateIR::setup();

    WoleixStateMachine::setup(command_queue_.get());
    WoleixProtocolHandler::setup(command_queue_.get());

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

/**
 * Calculate commands needed to reach the target state.
 * 
 * Converts ESPHome climate states to Woleix-specific states using StateMapper,
 * then uses the state machine to generate the optimal command sequence.
 * 
 * @return Reference to vector of commands needed for the state transition
 */
void WoleixClimate::queue_commands_()
{
    WoleixInternalState target_state;
    // Map ESPHome Climate states to Woleix AC states
    target_state.power = StateMapper::esphome_to_woleix_power(mode != ClimateMode::CLIMATE_MODE_OFF);
    target_state.mode = StateMapper::esphome_to_woleix_mode(mode);
    target_state.fan_speed = StateMapper::esphome_to_woleix_fan_mode(fan_mode.value());
    target_state.temperature = target_temperature;
    
    // Generate command sequence via state machine
    WoleixStateMachine::move_to(target_state);
}

/**
 * Update internal ESPHome state based on the current state machine state.
 * 
 * Synchronizes the climate controller's state (mode, temperature, fan_mode)
 * with the internal state machine after command transmission. Uses StateMapper
 * to convert Woleix states to ESPHome equivalents.
 */
void WoleixClimate::update_state_()
{
    // Sync internal state with state machine
    auto current_state = get_state();
    mode = StateMapper::woleix_to_esphome_power(current_state.power) 
        ? StateMapper::woleix_to_esphome_mode(current_state.mode)
        : ClimateMode::CLIMATE_MODE_OFF;
    target_temperature = current_state.temperature;
    fan_mode = StateMapper::woleix_to_esphome_fan_mode(current_state.fan_speed);
}

/**
 * Transmit the current state via IR.
 * 
 * Called by ESPHome when the climate state changes. This method:
 * 1. Calculates the necessary commands to reach the target state
 * 2. Transmits those commands via IR
 * 3. Updates internal state to match the new state
 * 
 * If no commands are needed (already at target state), no transmission occurs.
 */
void WoleixClimate::transmit_state()
{
    if (on_hold_)
    {
        ESP_LOGW(TAG, "Transmission on hold due to full command queue");
    }
    else
    {
        ESP_LOGD(TAG, "Transmitting state - Mode: %d, Temp: %.1f, Fan: %d",
            static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));

        queue_commands_();
    }
    ESP_LOGD(TAG, "Reporting back state - Mode: %d, Temp: %.1f, Fan: %d",
        static_cast<int>(mode), target_temperature, static_cast<int>(fan_mode.value()));

    update_state_();
}

/**
 * Get the traits/capabilities of this climate device.
 * 
 * Defines what features this climate controller supports:
 * - Modes: OFF, COOL, DRY, FAN_ONLY
 * - Fan speeds: LOW, HIGH
 * - Temperature range: 15-30°C in 1°C steps
 * - Current temperature and humidity display
 * 
 * @return ClimateTraits describing supported features and ranges
 */
ClimateTraits WoleixClimate::traits()
{
    auto traits = ClimateTraits();

    traits.set_supported_modes
    ({
        ClimateMode::CLIMATE_MODE_OFF,
        ClimateMode::CLIMATE_MODE_COOL,
        ClimateMode::CLIMATE_MODE_DRY,
        ClimateMode::CLIMATE_MODE_FAN_ONLY
    });

    traits.set_supported_fan_modes
    ({
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
