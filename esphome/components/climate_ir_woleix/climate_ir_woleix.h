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
#include "esphome/components/remote_base/pronto_protocol.h"

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
 * remote control. It implements the Climate interface and coordinates with
 * the WoleixACStateMachine for IR command generation.
 * 
 * Features:
 * - Temperature control (15-30°C in COOL mode)
 * - Mode control (COOL, DEHUM, FAN)
 * - Fan speed control (LOW, HIGH)
 * - Temperature sensor integration (required)
 * - Humidity sensor integration (optional)
 * - Pronto hex format IR transmission
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
 * @see WoleixACStateMachine
 */
class WoleixClimate : public ClimateIR {

public:
    /**
     * Default constructor.
     * 
     * Creates a new WoleixClimate instance with its own internal state machine.
     */
    WoleixClimate();
    
    /**
     * Constructor with custom state machine.
     * 
     * @param state_machine Pointer to external state machine (for testing)
     */
    WoleixClimate(WoleixStateMachine* state_machine, WoleixCommandTransmitter* command_transmitter);
    
    /**
     * Setup method called once during initialization.
     * 
     * Initializes the humidity sensor callback if a humidity sensor is configured.
     * This allows the climate controller to track current humidity levels.
     */
    void setup() override;

    /**
     * Set the humidity sensor for current humidity readings.
     * 
     * @param humidity_sensor Pointer to sensor providing humidity data
     */
    void set_humidity_sensor(sensor::Sensor *humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }

    /**
     * Set the reset button to reconcile the state with the physical device.
     * 
     * @param humidity_sensor Pointer to the reset button
     */
    void set_reset_button(binary_sensor::BinarySensor *btn) { this->reset_button_ = btn; }

    /**
     * Reset the state machine to default values.
     * 
     * Resets internal state to: power=ON, mode=COOL, temperature=25°C, fan_speed=LOW
     * Does not transmit any IR commands.
     */
    virtual void reset_state();

    virtual bool is_on() { return this->state_machine_->get_state().power == WoleixPowerState::ON; }

protected:
    /**
     * Transmit the current state via IR.
     * 
     * Called by ESPHome when the climate state changes. Generates the necessary
     * IR command sequence and transmits it to the AC unit.
     */
    void transmit_state() override;
    
    /**
     * Get the traits/capabilities of this climate device.
     * 
     * @return ClimateTraits describing supported features and temperature range
     */
    ClimateTraits traits() override;
    
    /**
     * Transmit all queued Pronto IR commands.
     * 
     * Sends each command in the queue with appropriate delays between transmissions
     * to ensure the AC unit processes each command correctly.
     */
    virtual void transmit_commands_(std::vector<WoleixCommand>& commands);
    
    virtual const std::vector<WoleixCommand>& calculate_commands_();
    virtual void update_state_();

    WoleixStateMachine *state_machine_{nullptr};  /**< State machine for command generation */
    WoleixCommandTransmitter *command_transmitter_{nullptr};
    sensor::Sensor *humidity_sensor_{nullptr};      /**< Optional humidity sensor */
    binary_sensor::BinarySensor *reset_button_{nullptr};  /**< Optional reset button */
    std::vector<WoleixCommand> commands_;           /**< Queue of commands to transmit */
};

}  // namespace climate_ir_woleix
}  // namespace esphome
