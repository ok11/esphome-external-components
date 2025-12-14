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

class WoleixTransmitter;

class WoleixCommandBase {
public:
    enum class Type {
        POWER,
        TEMP_UP,
        TEMP_DOWN,
        MODE,
        FAN_SPEED
    };
    WoleixCommandBase(Type type, uint32_t delay_ms = 0, uint32_t repeat_count = 1)
        : type_(type), delay_ms_(delay_ms), repeat_count_(repeat_count) {}
    virtual ~WoleixCommandBase() = default;

    virtual Type get_type() const {
        return type_;
    };

    /**
     * Equality comparison operator.
     * Compares both the string content and delay value.
     */
    bool operator==(const WoleixCommandBase& other) const {
        return type_ == other.type_ &&
               delay_ms_ == other.delay_ms_ &&
               repeat_count_ == other.repeat_count_;
    }
    uint16_t get_delay_ms() const {
        return delay_ms_;
    }
    uint8_t get_repeat_count() const {
        return repeat_count_;
    }

protected:
    Type type_;
    uint16_t delay_ms_{0};  /**< Delay after command in milliseconds */
    uint8_t repeat_count_{1};  /**< Number of times to repeat the command */
};

class WoleixProntoCommand: public WoleixCommandBase {
public:
    WoleixProntoCommand(Type type, uint32_t delay_ms, uint32_t repeat_count = 1)
        : WoleixCommandBase(type, delay_ms, repeat_count),
          pronto_hex_(get_pronto_hex_for_type(type)) {}
    
    virtual const std::string& get_pronto_hex_for_type(Type type) const {
        static const std::map<Type, std::string> pronto_map = {
            {Type::POWER, POWER_PRONTO},
            {Type::TEMP_UP, TEMP_UP_PRONTO},
            {Type::TEMP_DOWN, TEMP_DOWN_PRONTO},
            {Type::MODE, MODE_PRONTO},
            {Type::FAN_SPEED, SPEED_PRONTO}
        };
        return pronto_map.at(type);
    }

    const std::string& get_pronto_hex() const {
        return pronto_hex_;
    }
    /**
     * Equality comparison operator.
     * Compares both the string content and delay value.
     */
    bool operator==(const WoleixProntoCommand& other) const {
        return WoleixCommandBase::operator==(other) && 
            pronto_hex_ == other.pronto_hex_;
    }

protected:
    std::string pronto_hex_;  /**< Pronto format IR command string */
};

class WoleixNecCommand: public WoleixCommandBase {
public:
    WoleixNecCommand(Type type, uint16_t address, uint32_t delay_ms, uint32_t repeat_count = 1)
        : WoleixCommandBase(type, delay_ms, repeat_count),
          address_(address),
          command_code_(get_value_for_type(type)) {}
    
    virtual uint16_t get_value_for_type(Type type) const {
        static const std::map<Type,  uint16_t> pronto_map = {
            {Type::POWER, POWER_NEC},
            {Type::TEMP_UP, TEMP_UP_NEC},
            {Type::TEMP_DOWN, TEMP_DOWN_NEC},
            {Type::MODE, MODE_NEC},
            {Type::FAN_SPEED, SPEED_NEC}
        };
        return pronto_map.at(type);
    }

    const uint16_t& get_command_code() const {
        return command_code_;
    }

    uint16_t get_address() const {
        return address_;
    }
    /**
     * Equality comparison operator.
     * Compares both the string content and delay value.
     */
    bool operator==(const WoleixNecCommand& other) const {
        return WoleixCommandBase::operator==(other) && 
            command_code_ == other.command_code_ &&
            address_ == other.address_;
    }

protected:
    uint16_t address_;       /**< NEC format IR address */
    uint16_t command_code_;  /**< NEC format IR command code */
};

// Variant holding all possible types
using WoleixCommand = std::variant<WoleixProntoCommand, WoleixNecCommand>;

class WoleixTransmitter {
public:
    WoleixTransmitter(RemoteTransmitterBase* transmitter)
        : transmitter_(transmitter) {}
    virtual ~WoleixTransmitter() = default;
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

class WoleixCommandTransmitter: public WoleixTransmitter {
public:
    WoleixCommandTransmitter(RemoteTransmitterBase* transmitter)
        : WoleixTransmitter(transmitter) {}
    virtual ~WoleixCommandTransmitter() = default;

    virtual void transmit_(std::vector<WoleixCommand>& commands) {
        for(const WoleixCommand& command: commands) {
            transmit_(command);
        }
    };
    virtual void transmit_(const WoleixCommand& command) {
        std::visit(std::ref(*this), command);
    };
    virtual void operator()(const WoleixProntoCommand& command);
    virtual void operator()(const WoleixNecCommand& command);
};

/** @name WoleixProntoTransmitter
 * Transmits IR commands in Pronto hex format for Woleix AC units.
 * Uses a carrier frequency of 38.03 kHz.
 * @{ */

class WoleixProntoTransmitter : WoleixCommandTransmitter {
public:
    WoleixProntoTransmitter(RemoteTransmitterBase* transmitter)
        : WoleixCommandTransmitter(transmitter) {}

    void transmit_(const std::string& pronto_hex);
};
/** @} */

/** @name WoleixNecTransmitter
 * Transmits IR commands in NEC format for Woleix AC units.
 * @{ */

class WoleixNecTransmitter : WoleixCommandTransmitter {
public:
    WoleixNecTransmitter(RemoteTransmitterBase* transmitter)
        : WoleixCommandTransmitter(transmitter) {}

    void transmit_(uint16_t address, uint16_t command_code, uint8_t repeat_count);
};
/** @} */

}  // namespace climate_ir_woleix
}  // namespace esphome
