// #pragma once

// #include <vector>
// #include <functional>
// #include "woleix_comm.h"

// namespace esphome {
// namespace climate_ir_woleix {

// /**
//  * Callback interface for protocol handler to schedule async operations.
//  * This decouples the handler from ESPHome's Component class.
//  */
// class ProtocolScheduler {
// public:
//     virtual ~ProtocolScheduler() = default;
    
//     /**
//      * Schedule a callback to run after a delay.
//      * @param name Unique name for this timeout (for cancellation)
//      * @param delay_ms Delay in milliseconds
//      * @param callback Function to call
//      */
//     virtual void schedule_timeout(const std::string& name, uint32_t delay_ms, 
//                                   std::function<void()> callback) = 0;
    
//     /**
//      * Cancel a previously scheduled timeout.
//      * @param name Name of the timeout to cancel
//      */
//     virtual void cancel_timeout(const std::string& name) = 0;
    
//     /**
//      * Get current time in milliseconds (for tracking).
//      */
//     virtual uint32_t get_millis() const = 0;
// };

// /**
//  * Protocol states for temperature setting mode.
//  */
// enum class TempProtocolState {
//     IDLE,           ///< Not in temperature setting mode
//     SETTING_ACTIVE  ///< In setting mode, subsequent temp commands go through directly
// };

// /**
//  * Handles Woleix AC protocol timing and quirks.
//  * 
//  * This class manages the asynchronous aspects of communicating with the AC:
//  * - Temperature setting mode (enter, maintain, timeout)
//  * - Inter-command delays
//  * - Command sequencing
//  * 
//  * It is decoupled from ESPHome via the ProtocolScheduler interface,
//  * making it testable without ESPHome dependencies.
//  */
// class WoleixProtocolHandler {
// public:
//     /**
//      * Construct a protocol handler.
//      * 
//      * @param transmitter The underlying IR transmitter
//      * @param scheduler Interface for async scheduling (provided by Component)
//      */
//     WoleixProtocolHandler(WoleixTransmitter* transmitter, ProtocolScheduler* scheduler);
    
//     virtual ~WoleixProtocolHandler() = default;

//     /**
//      * Execute a sequence of commands with proper protocol handling.
//      * 
//      * Commands are executed asynchronously with appropriate delays.
//      * Temperature commands automatically handle setting mode.
//      * 
//      * @param commands Commands to execute
//      * @param on_complete Optional callback when all commands are done
//      */
//     virtual void execute(const std::vector<WoleixCommand>& commands,
//                         std::function<void()> on_complete = nullptr);

//     /**
//      * Check if currently in temperature setting mode.
//      */
//     bool is_in_temp_setting_mode() const { return temp_state_ == TempProtocolState::SETTING_ACTIVE; }

//     /**
//      * Reset protocol state (e.g., after AC power cycle).
//      * Cancels pending operations and resets to IDLE.
//      */
//     void reset();

//     /**
//      * Get the underlying transmitter (for direct access if needed).
//      */
//     WoleixTransmitter* get_transmitter() const { return transmitter_; }

// protected:
//     // Protocol timing constants
//     static constexpr uint32_t TEMP_SETTING_MODE_TIMEOUT_MS = 5000;
//     static constexpr uint32_t TEMP_ENTER_DELAY_MS = 150;
//     static constexpr uint32_t INTER_COMMAND_DELAY_MS = 200;

//     // Timeout names
//     static constexpr const char* TIMEOUT_SETTING_MODE = "proto_setting_mode";
//     static constexpr const char* TIMEOUT_NEXT_COMMAND = "proto_next_cmd";

//     /**
//      * Process the next command in the queue.
//      */
//     void process_next_command_();

//     /**
//      * Handle a temperature command (TEMP_UP or TEMP_DOWN).
//      * Manages setting mode entry and timeout extension.
//      */
//     void handle_temp_command_(const WoleixCommand& cmd);

//     /**
//      * Handle a non-temperature command.
//      */
//     void handle_regular_command_(const WoleixCommand& cmd);

//     /**
//      * Enter temperature setting mode.
//      * Sends the first temp command and schedules the setting mode timeout.
//      */
//     void enter_setting_mode_(const WoleixCommand& cmd);

//     /**
//      * Extend the setting mode timeout.
//      * Called after each temp command to reset the 5-second window.
//      */
//     void extend_setting_mode_timeout_();

//     /**
//      * Called when setting mode times out.
//      */
//     void on_setting_mode_timeout_();

//     /**
//      * Check if a command is a temperature command.
//      */
//     static bool is_temp_command_(const WoleixCommand& cmd);

//     /**
//      * Transmit a single command via IR.
//      */
//     void transmit_(const WoleixCommand& cmd);

// private:
//     WoleixTransmitter* transmitter_;
//     ProtocolScheduler* scheduler_;
    
//     TempProtocolState temp_state_{TempProtocolState::IDLE};
    
//     // Command queue for async execution
//     std::vector<WoleixCommand> command_queue_;
//     size_t current_command_index_{0};
//     std::function<void()> on_complete_callback_;
    
//     // Pending temp commands while entering setting mode
//     std::vector<WoleixCommand> pending_temp_commands_;
// };

// }  // namespace climate_ir_woleix
// }  // namespace esphome