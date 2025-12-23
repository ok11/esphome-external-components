#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <numeric>
#include <set>

#include "woleix_constants.h"
#include "woleix_management.h"

namespace esphome {
namespace climate_ir_woleix {

/**
 * @brief Represents a single Woleix IR command using the NEC protocol.
 * 
 * This class encapsulates an IR command that can be sent to a Woleix AC unit.
 * Each command consists of a command type, NEC address, optional delay, and repeat count.
 * 
 * The NEC protocol is used for all Woleix IR transmissions, with a fixed address
 * of 0xFB04 and various command codes for different operations.
 */
class WoleixCommand
{
public:
    /**
     * @brief Command types corresponding to AC remote buttons.
     * 
     * Each type maps to a specific NEC command code defined in woleix_constants.h
     */
    enum class Type: uint16_t
    {
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
        : type_(type), address_(address), repeat_count_(repeat_count)
    {}
    
    ~WoleixCommand() = default;

    /**
     * @brief Get the command type.
     * @return Command type enum value
     */
    Type get_type() const { return type_; };

    /**
     * @brief Get the NEC command code.
     * @return 16-bit NEC command code
     */
    uint16_t get_command() const { return static_cast<uint16_t>(type_); };
    
    /**
     * @brief Get the NEC protocol address.
     * @return 16-bit NEC address
     */
    uint16_t get_address() const { return address_; }

    /**
     * @brief Get the number of times to repeat this command.
     * @return Repeat count
     */
    uint32_t get_repeat_count() const { return repeat_count_; }

    /**
     * @brief Equality comparison operator.
     * 
     * Compares type, address, delay, and repeat count.
     * 
     * @param other Command to compare with
     * @return true if commands are identical, false otherwise
     */
    bool operator==(const WoleixCommand& other) const
    {
        return type_ == other.type_ &&
            address_ == other.address_ &&
            repeat_count_ == other.repeat_count_;
    }

protected:
    Type type_;
    uint16_t address_;    /**< NEC format IR address */
    uint32_t repeat_count_{1};  /**< Number of times to repeat the command */
};

const WoleixStatus WX_STATUS_QUEUE_FULL = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_ERROR,
    WoleixStatus::Category::WX_CAT_QUEUE_FULL,
    "The command queue is full"
);

const WoleixStatus WX_STATUS_QUEUE_EMPTY = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_INFO,
    WoleixStatus::Category::WX_CAT_QUEUE_EMPTY,
    "The command queue is empty"
);

class WoleixCommandQueueListener: public WoleixListener {
public:

    virtual void notify(const WoleixReporter& reporter, const WoleixStatus& status)
    {
        if (status == WX_STATUS_QUEUE_FULL) hold();
        if (status == WX_STATUS_QUEUE_EMPTY) resume();
    }

    virtual void hold() = 0;
    virtual void resume() = 0;
};


class WoleixCommandQueue: public WoleixReporter
{
public:


    WoleixCommandQueue(size_t max_capacity)
        : max_capacity_(max_capacity), queue_(std::make_unique<std::deque<WoleixCommand>>())
    {}

    void register_listener(std::shared_ptr<WoleixListener> listener) { listeners_.insert(listener); }
    void unregister_listener(std::shared_ptr<WoleixListener> listener) { listeners_.erase(listener); }

    const void enqueue(const WoleixCommand& command)
    {
        if (queue_->size() > max_capacity_ * 0.8) notify_listeners(WX_STATUS_QUEUE_FULL);
        queue_->push_back(command);
    }
    const WoleixCommand& dequeue()
    {
        if (queue_->size() < max_capacity_ * 0.2) notify_listeners(WX_STATUS_QUEUE_EMPTY);
        const auto& cmd = queue_->front();
        queue_->pop_front();
        return cmd;
    }

    void notify_listeners(const WoleixStatus& status) const
    {
        for (const auto& listener : listeners_)
        {
            listener->notify(*this, status);
        }
    }
    void reset() { queue_.reset(); }
    bool is_empty() const { return queue_->empty(); }
    uint16_t length() const { return queue_->size(); }
    const WoleixCommand& get_command(int index) const { return queue_->at(index); }
    size_t max_capacity() const { return max_capacity_; }

protected:
    size_t max_capacity_;
    std::unique_ptr<std::deque<WoleixCommand>> queue_;

    std::set<std::shared_ptr<WoleixListener>> listeners_;
};
    
} // namespace climate_ir_woleix
} // namespace esphome
