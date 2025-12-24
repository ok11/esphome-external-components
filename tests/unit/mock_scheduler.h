#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

#include "woleix_protocol_handler.h"

using esphome::climate_ir_woleix::WoleixCommandQueue;
using esphome::climate_ir_woleix::WoleixProtocolHandler;

/**
 * @brief A deterministic time-based mock scheduler for testing async behavior.
 * 
 * This scheduler maintains a virtual clock and fires timeouts in strict
 * chronological order. This is crucial for testing code with multiple
 * interacting timers (like the protocol handler's command polling and
 * setting mode timeout).
 * 
 * Key design decisions:
 * - Timeouts are stored with absolute fire times, not relative delays
 * - advance_time() fires ONE timeout at a time in chronological order
 * - Callbacks that schedule new timeouts don't affect the current advance
 * - Named timeouts can be inspected, cancelled, and manually fired
 */
class MockScheduler
{
public:
    struct ScheduledTimeout
    {
        std::string name;
        uint64_t fire_at_ms;  // Absolute time when this should fire
        std::function<void()> callback;
        
        bool operator<(const ScheduledTimeout& other) const
        {
            // Sort by fire time, then by name for determinism
            if (fire_at_ms != other.fire_at_ms)
                return fire_at_ms < other.fire_at_ms;
            return name < other.name;
        }
    };

    MockScheduler() : current_time_ms_(0) {}

    // ========================================================================
    // Interface for WoleixProtocolHandler
    // ========================================================================
    
    WoleixProtocolHandler::TimeoutSetter get_setter()
    {
        return [this](const std::string& name, uint32_t delay_ms, std::function<void()> callback)
        {
            // Cancel any existing timeout with same name first
            cancel_timeout_internal(name);
            
            uint64_t fire_at = current_time_ms_ + delay_ms;
            timeouts_.insert({name, fire_at, std::move(callback)});
        };
    }
    
    WoleixProtocolHandler::TimeoutCanceller get_canceller()
    {
        return [this](const std::string& name)
        {
            cancel_timeout_internal(name);
            cancelled_names_.insert(name);
        };
    }

    // ========================================================================
    // Test Control Methods
    // ========================================================================
    
    /**
     * @brief Get current virtual time in milliseconds.
     */
    uint64_t current_time() const { return current_time_ms_; }
    
    /**
     * @brief Advance time by the specified duration.
     * 
     * Fires all timeouts that become due during this time advancement,
     * in strict chronological order. If a callback schedules a new timeout
     * that falls within the advancement window, it will also be fired.
     * 
     * @param ms Milliseconds to advance
     * @return Number of timeouts that were fired
     */
    size_t advance_time(uint32_t ms)
    {
        uint64_t target_time = current_time_ms_ + ms;
        size_t fired_count = 0;
        
        while (!timeouts_.empty())
        {
            auto it = timeouts_.begin();
            if (it->fire_at_ms > target_time)
                break;
            
            // Move time to this timeout's fire time
            current_time_ms_ = it->fire_at_ms;
            
            // Extract and fire the callback
            auto callback = std::move(it->callback);
            timeouts_.erase(it);
            callback();
            fired_count++;
        }
        
        // Move to target time even if no more timeouts
        current_time_ms_ = target_time;
        return fired_count;
    }
    
    /**
     * @brief Advance time until a specific named timeout fires.
     * 
     * Useful for testing: "advance until the setting mode timeout fires"
     * 
     * @param name Name of the timeout to wait for
     * @param max_ms Maximum time to advance (safety limit)
     * @return true if the named timeout was fired, false if max_ms reached
     */
    bool advance_until(const std::string& name, uint32_t max_ms = 60000)
    {
        uint64_t deadline = current_time_ms_ + max_ms;
        
        while (current_time_ms_ < deadline && !timeouts_.empty())
        {
            auto it = timeouts_.begin();
            if (it->fire_at_ms > deadline)
                break;
            
            current_time_ms_ = it->fire_at_ms;
            std::string fired_name = it->name;
            auto callback = std::move(it->callback);
            timeouts_.erase(it);
            callback();
            
            if (fired_name == name)
                return true;
        }
        
        current_time_ms_ = deadline;
        return false;
    }
    
    /**
     * @brief Fire exactly one timeout (the next one due).
     * 
     * Time advances to that timeout's scheduled time.
     * 
     * @return Name of the fired timeout, or empty string if none pending
     */
    std::string fire_next()
    {
        if (timeouts_.empty())
            return "";
        
        auto it = timeouts_.begin();
        current_time_ms_ = it->fire_at_ms;
        std::string name = it->name;
        auto callback = std::move(it->callback);
        timeouts_.erase(it);
        callback();
        return name;
    }
    
    /**
     * @brief Fire a specific timeout by name, regardless of time.
     * 
     * Does NOT advance time. Useful for testing specific scenarios.
     * 
     * @param name Timeout name to fire
     * @return true if found and fired, false otherwise
     */
    bool fire_timeout(const std::string& name)
    {
        for (auto it = timeouts_.begin(); it != timeouts_.end(); ++it)
        {
            if (it->name == name)
            {
                auto callback = std::move(it->callback);
                timeouts_.erase(it);
                callback();
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Process commands until queue is empty, respecting timing.
     * 
     * This advances time as needed to fire command processing timeouts,
     * but STOPS before firing the setting mode timeout unless the queue
     * is already empty.
     * 
     * @param queue The command queue to drain
     * @param max_iterations Safety limit
     */
    void drain_queue(WoleixCommandQueue* queue, size_t max_iterations = 100)
    {
        size_t iterations = 0;
        while (!queue->is_empty() && iterations++ < max_iterations)
        {
            // Fire the next command processing timeout
            if (!fire_timeout("proto_next_cmd"))
            {
                // No command timeout pending, try advancing a bit
                advance_time(100);
            }
        }
    }

    // ========================================================================
    // Inspection Methods
    // ========================================================================
    
    bool has_timeout(const std::string& name) const
    {
        return std::any_of(timeouts_.begin(), timeouts_.end(),
            [&name](const auto& t) { return t.name == name; });
    }
    
    /**
     * @brief Get the scheduled fire time for a named timeout.
     * @throws std::runtime_error if not found
     */
    uint64_t get_timeout_time(const std::string& name) const
    {
        for (const auto& t : timeouts_)
        {
            if (t.name == name)
                return t.fire_at_ms;
        }
        throw std::runtime_error("Timeout not found: " + name);
    }
    
    /**
     * @brief Get time remaining until a named timeout fires.
     */
    uint32_t time_until(const std::string& name) const
    {
        uint64_t fire_at = get_timeout_time(name);
        if (fire_at <= current_time_ms_)
            return 0;
        return static_cast<uint32_t>(fire_at - current_time_ms_);
    }
    
    bool was_cancelled(const std::string& name) const
    {
        return cancelled_names_.count(name) > 0;
    }
    
    size_t pending_count() const { return timeouts_.size(); }
    
    /**
     * @brief Get all pending timeout names (for debugging).
     */
    std::vector<std::string> pending_names() const
    {
        std::vector<std::string> names;
        for (const auto& t : timeouts_)
            names.push_back(t.name);
        return names;
    }
    
    void reset()
    {
        timeouts_.clear();
        cancelled_names_.clear();
        current_time_ms_ = 0;
    }

    void cancel_timeout_internal(const std::string& name)
    {
        for (auto it = timeouts_.begin(); it != timeouts_.end(); )
        {
            if (it->name == name)
                it = timeouts_.erase(it);
            else
                ++it;
        }
    }

    std::set<ScheduledTimeout> timeouts_;  // Sorted by fire time
    std::set<std::string> cancelled_names_;
    uint64_t current_time_ms_;
};
