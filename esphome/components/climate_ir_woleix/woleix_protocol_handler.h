#pragma once

#include <cinttypes>
#include <string>
#include <functional>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/nec_protocol.h"

#include "woleix_constants.h"
#include "woleix_command.h"

namespace esphome
{
namespace climate_ir_woleix
{

using remote_base::RemoteTransmitterBase;

namespace WoleixCategory::ProtocolHandler
{
    inline constexpr auto WX_CATEGORY_INVALID_COMMAND_QUEUE = 
        Category::make(CategoryId::ProtocolHandler, 1, "ProtocolHandler.InvalidCommandQueue");
    inline constexpr auto WX_CATEGORY_COMMAND_QUEUE_NOT_SET = 
        Category::make(CategoryId::ProtocolHandler, 2, "ProtocolHandler.CommandQueueNotSet");
}

/**
 * Protocol states for temperature setting mode.
 */
enum class TempProtocolState: uint8_t
{
    IDLE,           ///< Not in temperature setting mode
    SETTING_ACTIVE  ///< In setting mode, subsequent temp commands go through directly
};


/**
 * @brief Handles transmission of Woleix IR commands via NEC protocol.
 * 
 * This class wraps ESPHome's RemoteTransmitterBase to send IR commands
 * to Woleix AC units. It converts WoleixCommand objects into NEC protocol
 * data and transmits them through the configured IR transmitter.
 * 
 * The transmitter handles command delays and repeats automatically.
 * IMPORTANT: Woleix Temperature Protocol
 * 
 * The Woleix AC requires n+1 IR commands to change temperature by n degrees:
 *   1. First command: Enters "setting mode" (displays current temp, no change)
 *   2. Commands 2-n+1: Actually change the temperature
 * 
 * The state machine queues n commands (logical changes).
 * The protocol handler transmits n+1 commands (physical protocol).
 */
class WoleixProtocolHandler
  : protected WoleixCommandQueueConsumer,
    protected WoleixStatusReporter
{
public:

    /// Function type for scheduling a timeout
    using TimeoutSetter = std::function<void(const std::string&, uint32_t, std::function<void()>)>;    
    /// Function type for cancelling a timeout
    using TimeoutCanceller = std::function<void(const std::string&)>;

    /**
     * @brief Construct a new transmitter with the given IR transmitter base.
     * 
     * @param transmitter ESPHome remote transmitter to use for IR output
     */
    WoleixProtocolHandler(TimeoutSetter set_timeout, TimeoutCanceller cancel_timeout)
      : set_timeout_(std::move(set_timeout)),
        cancel_timeout_(std::move(cancel_timeout)) 
    {}
    
    virtual ~WoleixProtocolHandler() { cleanup_(); } 

    /**
     * Execute commands synchronously (from caller's perspective).
     * 
     * Internally handles async timing for temperature setting mode,
     * but returns immediately. The caller doesn't need to wait or
     * handle completion - state machine has already updated.
     * 
     * @param commands Commands to execute
     */
    void setup(WoleixCommandQueue* command_queue);

    /**
     * Reset protocol state (e.g., after AC power cycle).
     * Cancels pending operations and resets to IDLE.
     */
    void reset();

    /**
     * @brief Set the IR transmitter base.
     * 
     * @param transmitter New transmitter to use
     */
    virtual void set_transmitter(RemoteTransmitterBase* transmitter)
    {
        transmitter_ = transmitter;
    }
    
    /**
     * @brief Get the current IR transmitter base.
     * 
     * @return Pointer to the current transmitter
     */
    virtual RemoteTransmitterBase* get_transmitter() const
    {
        return transmitter_;
    }

protected:

    /**
     * @brief Transmit a single command.
     * 
     * Converts the command to NEC protocol format and transmits via IR.
     * 
     * @param command Command to transmit
     */
    virtual void transmit_(const WoleixCommand& command);

    /**
     * Check if currently in temperature setting mode.
     */
    bool is_in_temp_setting_mode_() const { return temp_state_ == TempProtocolState::SETTING_ACTIVE; }

    // Protocol timing constants
    static constexpr uint32_t TEMP_SETTING_MODE_TIMEOUT_MS = 5000;
    static constexpr uint32_t TEMP_ENTER_DELAY_MS = 150;
    static constexpr uint32_t INTER_COMMAND_DELAY_MS = 200;

    // Timeout names
    static constexpr const char* TIMEOUT_SETTING_MODE = "proto_setting_mode";
    static constexpr const char* TIMEOUT_NEXT_COMMAND = "proto_next_cmd";

    /**
     * Process the next command in the queue.
     */
    void process_next_command_();

    /**
     * Handle a temperature command (TEMP_UP or TEMP_DOWN).
     * Manages setting mode entry and timeout extension.
     */
    void handle_temp_command_(const WoleixCommand& cmd);

    /**
     * Handle a non-temperature command.
     */
    void handle_regular_command_(const WoleixCommand& cmd);

    /**
     * Enter temperature setting mode.
     * Sends the first temp command and schedules the setting mode timeout.
     */
    void enter_setting_mode_(const WoleixCommand& cmd);

    /**
     * Extend the setting mode timeout.
     * Called after each temp command to reset the 5-second window.
     */
    void extend_setting_mode_timeout_();

    /**
     * Called when setting mode times out.
     */
    void on_setting_mode_timeout_();

    /**
     * Check if a command is a temperature command.
     */
    static bool is_temp_command_(const WoleixCommand& cmd);

    void cleanup_()
    {
        command_queue_->unregister_consumer(this);
        cancel_timeout_(TIMEOUT_SETTING_MODE);
        cancel_timeout_(TIMEOUT_NEXT_COMMAND);
    }

    void on_command() override
    {
        set_timeout_(TIMEOUT_NEXT_COMMAND, 0, [this]() { process_next_command_(); }); 
    }

private:

    // Command queue for async execution
    WoleixCommandQueue* command_queue_;
    RemoteTransmitterBase* transmitter_;
    TimeoutSetter set_timeout_;
    TimeoutCanceller cancel_timeout_;
    TempProtocolState temp_state_{TempProtocolState::IDLE};
    
    std::function<void()> on_complete_;
};

}  // namespace climate_ir_woleix
}  // namespace esphome
