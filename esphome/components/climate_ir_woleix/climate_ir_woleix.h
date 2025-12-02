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
#include "woleix_ac_state_machine.h"

namespace esphome {
namespace climate_ir_woleix {

using climate::ClimateMode;
using climate::ClimateFanMode;
using climate::ClimateTraits;
using climate_ir::ClimateIR;

const float_t WOLEIX_TEMP_MIN = 15.0f;  /**< Minimum temperature in Celsius */
const float_t WOLEIX_TEMP_MAX = 30.0f;  /**< Maximum temperature in Celsius */

struct WoleixSequence {
    const std::string& pronto_hex;  /**< Pronto hex format IR command (reference to constant) */
    uint16_t delay_ms;               /**< Delay after command in milliseconds (0-65535) */
    
    /**
     * Equality comparison operator.
     * Compares both the string content and delay value.
     */
    bool operator==(const WoleixSequence& other) const {
        return pronto_hex == other.pronto_hex && delay_ms == other.delay_ms;
    }
};

struct WoleixCommand {
    const std::vector<WoleixSequence>& sequences;

    /**
     * Equality comparison operator.
     * Compares all sequences.
     */
    bool operator==(const WoleixCommand& other) const {
        return sequences == other.sequences;
    }
};

/**
 * @name IR Command Definitions
 * Pronto hex format IR commands for Woleix AC remote control.
 * Carrier frequency: 38.03 kHz (0x006D)
 * @{
 */

/** Power button - toggles AC unit on/off */
static const std::string POWER_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0018 0014 0043 0013 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** Temperature Up button - increases temperature by 1°C */
static const std::string TEMP_UP_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0017 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483"
    ;

/** Temperature Down button - decreases temperature by 1°C */
static const std::string TEMP_DOWN_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483"
    ;

/** Mode button - cycles through COOL -> DEHUM -> FAN modes */
static const std::string MODE_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0043 0013 0018 0014 0018 0014 0043 0013 0043 0013 0043 0013 0042 "
    "0014 0483";

/** Speed/Fan button - toggles between LOW and HIGH fan speed */
static const std::string SPEED_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0018 0014 0041 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0040 0016 0043 0013 0043 0013 003D 0019 0040 0015 0018 0014 003E "
    "0018 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 "
    "0013 0018 0014 0018 0014 0043 0013 0043 0013 0041 0014 0043 0013 0043 "
    "0013 0483";

/** Timer button - controls timer function (not currently used) */
static const std::string TIMER_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0042 0014 0018 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** @} */

static const WoleixCommand POWER_COMMAND      = {{{ POWER_PRONTO, 108 }}};
static const WoleixCommand TEMP_UP_COMMAND    = {{{ TEMP_UP_PRONTO, 108 }, { TEMP_UP_PRONTO, 500 }}};
static const WoleixCommand TEMP_DOWN_COMMAND  = {{{ TEMP_DOWN_PRONTO, 108 }, { TEMP_DOWN_PRONTO, 500 }}};
static const WoleixCommand MODE_COMMAND       = {{{ MODE_PRONTO, 108 }}};
static const WoleixCommand SPEED_COMMAND      = {{{ SPEED_PRONTO, 108 }}};
static const WoleixCommand TIMER_COMMAND      = {{{ TIMER_PRONTO, 108 }}};

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
    WoleixClimate(WoleixACStateMachine* state_machine);
    
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
    virtual void transmit_commands_();
    
    /**
     * Queue a Pronto hex command for later transmission.
     * 
     * @param pronto_hex Pronto format IR command string
     */
    virtual void enqueue_command_(const WoleixCommand& command);
    
    /**
     * Map ESPHome ClimateMode to WoleixMode.
     * 
     * @param climate_mode ESPHome climate mode
     * @return Corresponding Woleix AC mode
     */
    WoleixMode map_climate_mode_(ClimateMode climate_mode);
    
    /**
     * Map ESPHome ClimateFanMode to WoleixFanSpeed.
     * 
     * @param fan_mode ESPHome fan mode
     * @return Corresponding Woleix fan speed
     */
    WoleixFanSpeed map_fan_mode_(ClimateFanMode fan_mode);

    WoleixACStateMachine *state_machine_{nullptr};  /**< State machine for command generation */
    sensor::Sensor *humidity_sensor_{nullptr};      /**< Optional humidity sensor */
    binary_sensor::BinarySensor *reset_button_{nullptr};  /**< Optional reset button */
    std::vector<WoleixCommand> commands_;           /**< Queue of commands to transmit */
};

}  // namespace climate_ir_woleix
}  // namespace esphome
