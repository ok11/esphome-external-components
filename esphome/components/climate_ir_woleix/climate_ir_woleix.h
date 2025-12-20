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
#include "esphome/components/binary_sensor/binary_sensor.h"

#include "woleix_constants.h"
#include "woleix_state_mapper.h"
#include "woleix_state_machine.h"
#include "woleix_comm.h"

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
class WoleixClimate : public ClimateIR
{
public:
    /**
     * Default constructor.
     * 
     * Creates a new WoleixClimate instance with its own internal state machine and command transmitter.
     */
    WoleixClimate()
        : WoleixClimate(new WoleixStateMachine(), new WoleixTransmitter(nullptr))
    {
    }

    /**
     * Constructor with custom state machine and command transmitter.
     * 
     * @param state_machine Pointer to external state machine (for testing)
     * @param command_transmitter Pointer to external command transmitter (for testing)
     */
    WoleixClimate(WoleixStateMachine* state_machine, WoleixTransmitter* command_transmitter);

    /**
     * Setup method called once during initialization.
     * 
     * Calls the parent ClimateIR::setup() method and then initializes the
     * humidity sensor callback if a humidity sensor is configured.
     * This allows the climate controller to track current humidity levels.
     */
    void setup() override;

    /**
     * Set the humidity sensor for current humidity readings.
     * 
     * @param humidity_sensor Pointer to sensor providing humidity data
     */
    void set_humidity_sensor(sensor::Sensor *humidity_sensor) { humidity_sensor_ = humidity_sensor; }

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
     * Transmit all queued IR commands.
     * 
     * Sends each command in the queue with appropriate delays between transmissions
     * to ensure the AC unit processes each command correctly.
     */
    virtual void transmit_commands_(std::vector<WoleixCommand>& commands);

    /**
     * Calculate commands needed to reach the target state.
     * 
     * @return Vector of WoleixCommand objects representing the necessary IR commands.
     */
    virtual const std::vector<WoleixCommand>& calculate_commands_();

    /**
     * Update internal state based on the current state machine state.
     */
    virtual void update_state_();

    WoleixStateMachine *state_machine_{nullptr};  /**< State machine for command generation and state management */
    WoleixTransmitter *command_transmitter_{nullptr};  /**< Command transmitter for sending IR commands */
    sensor::Sensor *humidity_sensor_{nullptr};  /**< Optional humidity sensor */
    std::vector<WoleixCommand> commands_;  /**< Queue of commands to transmit */
};

}  // namespace climate_ir_woleix
}  // namespace esphome
