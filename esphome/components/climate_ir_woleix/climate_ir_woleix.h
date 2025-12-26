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
#include "woleix_status.h"
#include "woleix_protocol_handler.h"
#include "woleix_state_mapper.h"
#include "woleix_state_manager.h"

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
 * WoleixStateManager for IR command generation and state management.
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
 * @see WoleixStateManager
 */
class WoleixClimate
  : public ClimateIR,
    public WoleixStateManager,
    public WoleixProtocolHandler,
    protected WoleixCommandQueueProducer,
    protected WoleixStatusObserver
{
public:
    /**
     * Default constructor.
     * 
     * Creates a new WoleixClimate instance with its own internal state manager and protocol handler.
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
    void set_transmitter(RemoteTransmitterBase* transmitter) override
    {
        ClimateIR::set_transmitter(transmitter);
        WoleixProtocolHandler::set_transmitter(transmitter);
    }

    /**
     * Set the humidity sensor for current humidity readings.
     * 
     * @param humidity_sensor Pointer to sensor providing humidity data
     */
    void set_humidity_sensor(sensor::Sensor* humidity_sensor) { humidity_sensor_ = humidity_sensor; }

    /**
     * Reset the state manager to default values.
     * 
     * Calls the reset() method of the internal state manager.
     * Note: This may not explicitly set all values to defaults,
     * but relies on the state manager's reset implementation.
     * Does not transmit any IR commands.
     */
    virtual void reset_state();

    /**
     * Check if the climate device is currently on.
     * 
     * @return true if the device is on, false otherwise
     */
    virtual bool is_on() { return get_state().power == WoleixPowerState::ON; }

protected:

    void observe(const WoleixStatusReporter& reporter, const WoleixStatus& status) override
    {
        if (status.get_severity() == WoleixStatus::Severity::WX_SEVERITY_ERROR)
        {
            ESP_LOGE(TAG, "Error (%s): %s", status.get_category().name, status.get_message().c_str());
            status_set_error(status.get_message().c_str());
        }
        if (status.get_severity() == WoleixStatus::Severity::WX_SEVERITY_WARNING)
        {
            ESP_LOGW(TAG, "Warning (%s): %s", status.get_category().name, status.get_message().c_str());
            status_set_warning(status.get_message().c_str());
        }
        if (status.get_severity() == WoleixStatus::Severity::WX_SEVERITY_INFO)
        {
            ESP_LOGI(TAG, "Info (%s): %s", status.get_category().name, status.get_message().c_str());
        }
        if (status.get_severity() == WoleixStatus::Severity::WX_SEVERITY_DEBUG)
        {
            ESP_LOGD(TAG, "Debug (%s): %s", status.get_category().name, status.get_message().c_str());
        }
    }

    void on_high_watermark() override
    {
        ESP_LOGW(TAG, "Queue at its high watermark (%d)", command_queue_->length());
        status_set_warning("Queue.AtHighWatermark");
        on_hold_ = true;
    }

    void on_low_watermark() override
    {
        ESP_LOGI(TAG, "Queue at its high watermark (%d)", command_queue_->length());
        on_hold_ = false;
    }

    void on_full() override
    {
        ESP_LOGE(TAG, "Queue full");
        status_set_error("Queue.Full");
    }

    void on_empty() override
    {
        ESP_LOGI(TAG, "Queue empty");
    }
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
    virtual bool enqueue_commands_();

    /**
     * Update internal state based on the current state machine state.
     */
    virtual void update_state_();

    std::unique_ptr<WoleixCommandQueue> command_queue_;         /**< Command queue for asynchronous execution */
//    std::shared_ptr<WoleixStateManager> state_manager_;         /**< State manager for command generation and state management */
//    std::shared_ptr<WoleixProtocolHandler> protocol_handler_;   /**< Protocol handler for sending IR commands */

    sensor::Sensor* humidity_sensor_{nullptr};  /**< Optional humidity sensor */
    bool on_hold_{false};                     /**< Flag indicating if command transmission is on hold */
};

}  // namespace climate_ir_woleix
}  // namespace esphome
