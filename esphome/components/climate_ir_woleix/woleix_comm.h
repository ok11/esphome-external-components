#pragma once

#include <cinttypes>
#include <string>
#include <vector>
#include <map>
#include <ranges>
#include <algorithm>
#include <cmath>
#include <variant>

#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/remote_base/nec_protocol.h"

#include "woleix_constants.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::RemoteTransmitterBase;

/**
 * @brief Represents a single Woleix IR command using the NEC protocol.
 * 
 * This class encapsulates an IR command that can be sent to a Woleix AC unit.
 * Each command consists of a command type, NEC address, optional delay, and repeat count.
 * 
 * The NEC protocol is used for all Woleix IR transmissions, with a fixed address
 * of 0xFB04 and various command codes for different operations.
 */
class WoleixCommand {
public:
    /**
     * @brief Command types corresponding to AC remote buttons.
     * 
     * Each type maps to a specific NEC command code defined in woleix_constants.h
     */
    enum class Type: uint16_t {
        POWER = POWER_NEC,        /**< Power on/off toggle */
        TEMP_UP = TEMP_UP_NEC,    /**< Increase temperature by 1°C */
        TEMP_DOWN = TEMP_DOWN_NEC,/**< Decrease temperature by 1°C */
        MODE = MODE_NEC,          /**< Cycle through operating modes */
        FAN_SPEED = SPEED_NEC     /**< Toggle fan speed LOW/HIGH */
    };
    /**
     * @brief Construct a new Woleix command.
     * 
     * @param type Command type (POWER, TEMP_UP, etc.)
     * @param address NEC protocol address (typically 0xFB04)
     * @param delay_ms Delay in milliseconds after transmitting this command (default: 0)
     * @param repeat_count Number of times to repeat this command (default: 1)
     */
    WoleixCommand(Type type, uint16_t address, uint32_t delay_ms = 0, uint32_t repeat_count = 1)
        : type_(type), address_(address), delay_ms_(delay_ms), repeat_count_(repeat_count) {}
    
    ~WoleixCommand() = default;

    /**
     * @brief Get the command type.
     * @return Command type enum value
     */
    Type get_type() const {
        return type_;
    };

    /**
     * @brief Get the NEC command code.
     * @return 16-bit NEC command code
     */
    uint16_t get_command() const {
        return static_cast<uint16_t>(type_);
    };
    
    /**
     * @brief Get the NEC protocol address.
     * @return 16-bit NEC address
     */
    uint16_t get_address() const {
        return address_;
    }

    /**
     * @brief Get the delay after transmission.
     * @return Delay in milliseconds
     */
    uint32_t get_delay_ms() const {
        return delay_ms_;
    }
    
    /**
     * @brief Get the number of times to repeat this command.
     * @return Repeat count
     */
    uint32_t get_repeat_count() const {
        return repeat_count_;
    }

    /**
     * @brief Equality comparison operator.
     * 
     * Compares type, address, delay, and repeat count.
     * 
     * @param other Command to compare with
     * @return true if commands are identical, false otherwise
     */
    bool operator==(const WoleixCommand& other) const {
        return type_ == other.type_ &&
            address_ == other.address_ &&
            delay_ms_ == other.delay_ms_ &&
            repeat_count_ == other.repeat_count_;
    }

protected:
    Type type_;
    uint16_t address_;    /**< NEC format IR address */
    uint32_t delay_ms_{0};  /**< Delay after command in milliseconds */
    uint32_t repeat_count_{1};  /**< Number of times to repeat the command */
};

/**
 * @brief Handles transmission of Woleix IR commands via NEC protocol.
 * 
 * This class wraps ESPHome's RemoteTransmitterBase to send IR commands
 * to Woleix AC units. It converts WoleixCommand objects into NEC protocol
 * data and transmits them through the configured IR transmitter.
 * 
 * The transmitter handles command delays and repeats automatically.
 */
class WoleixTransmitter {
public:
    /**
     * @brief Construct a new transmitter with the given IR transmitter base.
     * 
     * @param transmitter ESPHome remote transmitter to use for IR output
     */
    WoleixTransmitter(RemoteTransmitterBase* transmitter)
        : transmitter_(transmitter) {}
    
    virtual ~WoleixTransmitter() = default;

    /**
     * @brief Transmit a sequence of commands.
     * 
     * Sends each command in the vector sequentially, respecting delays and repeats.
     * 
     * @param commands Vector of commands to transmit
     */
    virtual void transmit_(std::vector<WoleixCommand>& commands) {
        for(const WoleixCommand& command: commands) {
            transmit_(command);
        }
    };
    
    /**
     * @brief Transmit a single command.
     * 
     * Converts the command to NEC protocol format and transmits via IR.
     * 
     * @param command Command to transmit
     */
    virtual void transmit_(const WoleixCommand& command);

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
    RemoteTransmitterBase* transmitter_;
};

}  // namespace climate_ir_woleix
}  // namespace esphome
