#pragma once

#include <cstdint>
#include <memory>
#include <deque>
#include <numeric>
#include <set>

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

//using namespace esphome::climate_ir_woleix::CategoryId;

namespace StatusCategory::Queue
{
    inline constexpr auto AT_HIGH_WATERMARK = 
        Category::make(CategoryId::CommandQueue, 1, "CommandQueue.AtHighWatermark");
    inline constexpr auto AT_LOW_WATERMARK = 
        Category::make(CategoryId::CommandQueue, 2, "CommandQueue.AtLowWatermark");
    inline constexpr auto EMPTY = 
        Category::make(CategoryId::CommandQueue, 3, "CommandQueue.Empty");
    inline constexpr auto FULL = 
        Category::make(CategoryId::CommandQueue, 4, "CommandQueue.Full");
    inline constexpr auto COMMAND_ENQUEUED = 
        Category::make(CategoryId::CommandQueue, 5, "CommandQueue.CommandEnqueued");
}

const WoleixStatus WX_STATUS_QUEUE_AT_HIGH_WATERMARK = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_WARNING,
    StatusCategory::Queue::AT_HIGH_WATERMARK,
    "The command queue is at high watermark"
);

const WoleixStatus WX_STATUS_QUEUE_AT_LOW_WATERMARK = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_INFO,
    StatusCategory::Queue::AT_LOW_WATERMARK,
    "The command queue is at low watermark"
);

const WoleixStatus WX_STATUS_QUEUE_EMPTY = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_INFO,
    StatusCategory::Queue::EMPTY,
    "The command queue is empty"
);

const WoleixStatus WX_STATUS_QUEUE_FULL = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_ERROR,
    StatusCategory::Queue::FULL,
    "The command queue is full"
);

const WoleixStatus WX_STATUS_QUEUE_COMMAND_ENQUEUED = WoleixStatus
(
    WoleixStatus::Severity::WX_SEVERITY_INFO,
    StatusCategory::Queue::COMMAND_ENQUEUED,
    "A command is enqueued into the command queue"
);

static constexpr float QUEUE_HIGH_WATERMARK = 0.8f;
static constexpr float QUEUE_LOW_WATERMARK = 0.2f;

class WoleixCommandQueueProducer: public WoleixListener
{
public:

    virtual void notify(const WoleixReporter& reporter, const WoleixStatus& status)
    {
        if (status == WX_STATUS_QUEUE_FULL) stop_enqueing();
        if (status == WX_STATUS_QUEUE_AT_HIGH_WATERMARK) hold_enqueing();
        if (status == WX_STATUS_QUEUE_AT_LOW_WATERMARK) resume_enqueing();
    }

    virtual void hold_enqueing() = 0;
    virtual void resume_enqueing() = 0;
    virtual void stop_enqueing() = 0;
};

class WoleixCommandQueueConsumer: public WoleixListener
{
public:

    virtual void notify(const WoleixReporter& reporter, const WoleixStatus& status)
    {
        if (status == WX_STATUS_QUEUE_COMMAND_ENQUEUED) start_processing();
    }

    virtual void start_processing() = 0;
};

class WoleixCommandQueue: protected WoleixReporter
{
public:


    WoleixCommandQueue(size_t max_capacity)
        : max_capacity_(max_capacity), queue_(std::make_unique<std::deque<WoleixCommand>>())
    {}

    void register_producer(WoleixCommandQueueProducer* producer)
    {
        register_listener(producer); 
    }
    void unregister_producer(WoleixCommandQueueProducer* producer)
    {
        unregister_listener(producer);
    }

    void register_consumer(WoleixCommandQueueConsumer* consumer)
    {
        register_listener(consumer);
    }
    void unregister_consumer(WoleixCommandQueueConsumer* consumer)
    {
        unregister_listener(consumer);
    }

    void enqueue(const WoleixCommand& command)
    {
        if (queue_->size() == max_capacity_)
        {
            notify_listeners(WX_STATUS_QUEUE_FULL);
        }
        else if (queue_->size() > max_capacity_ * QUEUE_HIGH_WATERMARK)
        {
            notify_listeners(WX_STATUS_QUEUE_AT_HIGH_WATERMARK);
        }

        queue_->push_back(command);

        if (queue_->size() == 1)
        {
            notify_listeners(WX_STATUS_QUEUE_COMMAND_ENQUEUED);
        }
    }
    const WoleixCommand& get() const
    {
        if (queue_->empty()) throw std::out_of_range("Next called on empty WoleixCommandQueue");
        return queue_->front();
    }
    void dequeue()
    {
        if (queue_->empty())
        {
            throw std::out_of_range("Dequeue called on empty WoleixCommandQueue");
        }
        if (queue_->size() < max_capacity_ * QUEUE_LOW_WATERMARK)
        {
            notify_listeners(WX_STATUS_QUEUE_AT_LOW_WATERMARK);
        }

        queue_->pop_front();
        
        if (queue_->empty())
        {
            notify_listeners(WX_STATUS_QUEUE_EMPTY);
        }
    }

    void notify_listeners(const WoleixStatus& status) const
    {
        for (const auto& listener : listeners_)
        {
            listener->notify(*this, status);
        }
    }
    void reset() { queue_->clear(); }
    bool is_empty() const { return queue_->empty(); }
    uint16_t length() const { return queue_->size(); }
    size_t max_capacity() const { return max_capacity_; }

protected:

    void register_listener(WoleixListener* listener) { listeners_.push_back(listener); }
    void unregister_listener(WoleixListener* listener) { std::erase(listeners_, listener); }

    size_t max_capacity_;
    std::unique_ptr<std::deque<WoleixCommand>> queue_;

    std::vector<WoleixListener*> listeners_;
};
    
} // namespace climate_ir_woleix
} // namespace esphome
