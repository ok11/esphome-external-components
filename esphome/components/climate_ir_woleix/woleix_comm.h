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
#include "esphome/components/remote_base/pronto_protocol.h"
#include "esphome/components/remote_base/nec_protocol.h"

#include "woleix_constants.h"

namespace esphome {
namespace climate_ir_woleix {

using remote_base::RemoteTransmitterBase;

/**
* @brief Woleix IR commands based on NEC protocol.
*/

class WoleixCommand {
public:
    enum class Type: uint16_t {
        POWER = POWER_NEC,
        TEMP_UP = TEMP_UP_NEC,
        TEMP_DOWN = TEMP_DOWN_NEC,
        MODE = MODE_NEC,
        FAN_SPEED = SPEED_NEC
    };
    WoleixCommand(Type type, uint16_t address, uint32_t delay_ms = 0, uint32_t repeat_count = 1)
        : type_(type), address_(address), delay_ms_(delay_ms), repeat_count_(repeat_count) {}
    ~WoleixCommand() = default;

    Type get_type() const {
        return type_;
    };

    uint16_t get_command() const {
        return static_cast<uint16_t>(type_);
    };
    uint16_t get_delay_ms() const {
        return delay_ms_;
    }
    uint16_t get_repeat_count() const {
        return repeat_count_;
    }

    uint16_t get_address() const {
        return address_;
    }

    /**
    * Equality comparison operator.
    * Compares both the string content and delay value.
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
    uint16_t delay_ms_{0};  /**< Delay after command in milliseconds */
    uint16_t repeat_count_{1};  /**< Number of times to repeat the command */
};

/**
* @brief Transmitter class for sending Woleix IR commands.
*/
class WoleixTransmitter {
public:
    WoleixTransmitter(RemoteTransmitterBase* transmitter)
        : transmitter_(transmitter) {}
    virtual ~WoleixTransmitter() = default;

    virtual void transmit_(std::vector<WoleixCommand>& commands) {
        for(const WoleixCommand& command: commands) {
            transmit_(command);
        }
    };
    virtual void transmit_(const WoleixCommand& command);

    virtual void set_transmitter(RemoteTransmitterBase* transmitter)
    {
        transmitter_ = transmitter;
    }
    virtual RemoteTransmitterBase* get_transmitter() const
    {
        return transmitter_;
    }

protected:
    RemoteTransmitterBase* transmitter_;
};

}  // namespace climate_ir_woleix
}  // namespace esphome
