#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <numeric>
#include <set>
#include <optional>

#include "woleix_constants.h"
#include "woleix_status.h"

namespace esphome
{
namespace climate_ir_woleix
{

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

static constexpr float QUEUE_HIGH_WATERMARK = 0.8f;
static constexpr float QUEUE_LOW_WATERMARK = 0.2f;

/**
 * @brief Interface for classes that produce commands for the WoleixCommandQueue.
 * 
 * This interface defines methods that are called when the queue reaches certain states,
 * allowing producers to react accordingly.
 */
class WoleixCommandQueueProducer
{
public:
    /**
     * @brief Called when the queue reaches its high watermark.
     */
    virtual void on_queue_at_high_watermark() = 0;

    /**
     * @brief Called when the queue reaches its low watermark.
     */
    virtual void on_queue_at_low_watermark() = 0;

    /**
     * @brief Called when the queue becomes full.
     */
    virtual void on_queue_full() = 0;

    /**
     * @brief Called when the queue becomes empty.
     */
    virtual void on_queue_empty() = 0;
};

/**
 * @brief Interface for classes that consume commands from the WoleixCommandQueue.
 * 
 * This interface defines a method that is called when a new command is enqueued,
 * allowing consumers to react to new commands.
 */
class WoleixCommandQueueConsumer
{
public:
    /**
     * @brief Called when a new command is enqueued.
     */
    virtual void on_command_enqueued() = 0;
};

/**
 * @brief A queue for managing WoleixCommand objects.
 * 
 * This class implements a queue with a maximum capacity for WoleixCommand objects.
 * It provides methods for enqueueing and dequeueing commands, as well as notifying
 * producers and consumers about the queue's state.
 */
class WoleixCommandQueue
{
public:
    WoleixCommandQueue(size_t max_capacity)
        : max_capacity_(max_capacity), queue_(std::make_unique<std::deque<WoleixCommand>>())
    {}

    void register_producer(WoleixCommandQueueProducer* producer)
    {
        producers_.push_back(producer);
    }
    void unregister_producer(WoleixCommandQueueProducer* producer)
    {
        std::erase(producers_, producer);
    }

    void register_consumer(WoleixCommandQueueConsumer* consumer)
    {
        consumers_.push_back(consumer);
    }
    void unregister_consumer(WoleixCommandQueueConsumer* consumer)
    {
        std::erase(consumers_, consumer);
    }

    bool enqueue(const WoleixCommand& command)
    {
        if (queue_->size() == max_capacity_)
        {
            on_queue_full();
            return false;
        }
        else if (queue_->size() > max_capacity_ * QUEUE_HIGH_WATERMARK)
        {
            on_queue_at_high_watermark();
        }

        queue_->push_back(command);

        if (queue_->size() == 1)
        {
            on_command_enqueued();
        }
        return true;
    }
    bool enqueue(const std::vector<WoleixCommand>& commands)
    {
        if (queue_->size() + commands.size() > max_capacity_)
        {
            on_queue_full();
            return false;
        }
        else if (queue_->size() + commands.size() >= max_capacity_ * QUEUE_HIGH_WATERMARK)
        {
            on_queue_at_high_watermark();
        }

        queue_->insert(queue_->end(), commands.begin(), commands.end());

        if (queue_->size() == commands.size())
        {
            on_command_enqueued();
        }
        return true;
    }

    std::optional<WoleixCommand> get() const
    {
        if (queue_->empty()) return {};
        return queue_->front();
    }

    bool dequeue()
    {
        if (queue_->empty()) return false;
        if (queue_->size() <= max_capacity_ * QUEUE_LOW_WATERMARK)
        {
            on_queue_at_low_watermark();
        }

        queue_->pop_front();
        
        if (queue_->empty())
        {
            on_queue_empty();
        }
        return true;
    }

    void on_queue_at_high_watermark() const
    {
        for (const auto& producer : producers_)
        {
            producer->on_queue_at_high_watermark();
        }
    }

    void on_queue_at_low_watermark() const
    {
        for (const auto& producer : producers_)
        {
            producer->on_queue_at_low_watermark();
        }
    }

    void on_queue_empty() const
    {
        for (const auto& producer : producers_)
        {
            producer->on_queue_empty();
        }
    }

    void on_queue_full() const
    {
        for (const auto& producer : producers_)
        {
            producer->on_queue_full();
        }
    }

    void on_command_enqueued() const
    {
        for (const auto& consumer : consumers_)
        {
            consumer->on_command_enqueued();
        }
    }

    void reset() { queue_->clear(); }
    bool is_empty() const { return queue_->empty(); }
    uint16_t length() const { return queue_->size(); }
    size_t max_capacity() const { return max_capacity_; }

protected:

    size_t max_capacity_;
    std::unique_ptr<std::deque<WoleixCommand>> queue_;

    std::vector<WoleixCommandQueueProducer*> producers_;
    std::vector<WoleixCommandQueueConsumer*> consumers_;
};
    
} // namespace climate_ir_woleix
} // namespace esphome
