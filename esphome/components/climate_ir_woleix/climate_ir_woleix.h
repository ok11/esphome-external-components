#pragma once

#include <cinttypes>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <deque>

#include "esphome/core/optional.h"

#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "woleix_constants.h"
#include "woleix_command.h"
#include "woleix_protocol_handler.h"
#include "woleix_state_mapper.h"
#include "woleix_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

using climate::ClimateMode;
using climate::ClimateFanMode;
using climate::ClimateTraits;
using climate_ir::ClimateIR;

/**
 * Climate IR controller for Woleix air conditioners.
 * 
 * This class provides ESPHome integration for Woleix AC units via infrared
 * remote control. It implements the Climate interface and uses an internal
 * WoleixStateMachine for IR command generation and state management.
 * 
 * Features:
 * - Temperature control (15-30°C in COOL mode)
 * - Mode control (COOL, DEHUM, FAN)
 * - Fan speed control (LOW, HIGH)
 * - Temperature sensor integration (required)
 * - Humidity sensor integration (optional)
 * - Reset button functionality (optional)
 * - NEC protocol IR transmission
 * 
 * Usage in ESPHome YAML:
 * @code{.yaml}
 * climate:
 *   - platform: climate_ir_woleix
 *     name: "Air Conditioner"
 *     transmitter_id: ir_transmitter
 *     sensor: room_temp
 *     humidity_sensor: room_humidity  # optional
 * @endcode
 * 
 * @see WoleixStateMachine
 */
class WoleixClimate
  : public ClimateIR,
    protected WoleixCommandQueueListener
{
public:
    /**
     * Default constructor.
     * 
     * Creates a new WoleixClimate instance with its own internal state machine and protocol handler.
     */
    WoleixClimate();

    /**
     * Setup method called once during initialization.
     * 
     * Calls the parent ClimateIR::setup() method and then initializes the
     * humidity sensor callback if a humidity sensor is configured.
     * This allows the climate controller to track current humidity levels.
     */
    void setup() override;

    /**
     * Set the transmitter for this climate device.
     * 
     * This method hides the base class (ClimateIR) method of the same name.
     * It calls the base class method and also sets the transmitter for the
     * protocol_handler_ object.
     * 
     * @param transmitter Pointer to the RemoteTransmitterBase object
     */
    void set_transmitter(RemoteTransmitterBase* transmitter)
    {
        ClimateIR::set_transmitter(transmitter);
        protocol_handler_->set_transmitter(transmitter);
    }

    /**
     * Set the humidity sensor for current humidity readings.
     * 
     * @param humidity_sensor Pointer to sensor providing humidity data
     */
    void set_humidity_sensor(sensor::Sensor* humidity_sensor) { humidity_sensor_ = humidity_sensor; }

    /**
     * Reset the state machine to default values.
     * 
     * Calls the reset() method of the internal state machine.
     * Note: This may not explicitly set all values to defaults,
     * but relies on the state machine's reset implementation.
     * Does not transmit any IR commands.
     */
    virtual void reset_state();

    /**
     * Check if the climate device is currently on.
     * 
     * @return true if the device is on, false otherwise
     */
    virtual bool is_on() { return this->state_machine_->get_state().power == WoleixPowerState::ON; }

protected:

    void hold()
    {
        ESP_LOGW(TAG, "Queue full (%d)", command_queue_->length());
        status_momentary_error("queue_full", 2000);
    }

    void resume()
    {
        ESP_LOGI(TAG, "Queue is empty again");
    }
    /**
     * Constructor with custom command queue, state machine and protocol handler.
     * 
     * @param command_queue Pointer to external command queue (for testing)
     * @param state_machine Pointer to external state machine (for testing)
     * @param protocol_handler Pointer to external protocol handler (for testing)
     */
    WoleixClimate(WoleixCommandQueue* command_queue, WoleixStateMachine* state_machine, WoleixProtocolHandler* protocol_handler);

    /**
     * Transmit the current state via IR.
     * 
     * Called by ESPHome when the climate state changes. Calculates necessary commands,
     * transmits them, and updates internal state.
     */
    void transmit_state() override;

    /**
     * Get the traits/capabilities of this climate device.
     * 
     * @return ClimateTraits describing supported features and temperature range
     */
    ClimateTraits traits() override;

    /**
     * Calculate commands needed to reach the target state.
     * 
     * @return Vector of WoleixCommand objects representing the necessary IR commands.
     */
    virtual void calculate_commands_();

    /**
     * Update internal state based on the current state machine state.
     */
    virtual void update_state_();

    std::unique_ptr<WoleixCommandQueue> command_queue_;         /**< Command queue for asynchronous execution */
    std::shared_ptr<WoleixStateMachine> state_machine_;         /**< State machine for command generation and state management */
    std::shared_ptr<WoleixProtocolHandler> protocol_handler_;   /**< Protocol handler for sending IR commands */

    sensor::Sensor* humidity_sensor_{nullptr};  /**< Optional humidity sensor */
};

}  // namespace climate_ir_woleix
}  // namespace esphome
