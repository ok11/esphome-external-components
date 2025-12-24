#pragma once

#include <functional>
#include <set>
#include <string>
#include <cstdint>

#include "woleix_protocol_handler.h"

using esphome::climate_ir_woleix::WoleixProtocolHandler;

class MockScheduler
{
public:
    struct ScheduledTimeout
    {
        std::string name;
        uint32_t delay_ms;
        std::function<void()> callback;
    };
    
    std::vector<ScheduledTimeout> scheduled_timeouts;
    std::set<std::string> cancelled_timeouts;
    
    // The lambdas to inject into WoleixProtocolHandler
    WoleixProtocolHandler::TimeoutSetter get_setter()
    {
        return [this](const std::string& name, uint32_t delay_ms, std::function<void()> callback)
        {
            // Remove any existing timeout with same name
            scheduled_timeouts.erase(
                std::remove_if(scheduled_timeouts.begin(), scheduled_timeouts.end(),
                    [&name](const auto& t) { return t.name == name; }),
                scheduled_timeouts.end());
            scheduled_timeouts.push_back({name, delay_ms, std::move(callback)});
        };
    }
    
    WoleixProtocolHandler::TimeoutCanceller get_canceller()
    {
        return [this](const std::string& name)
        {
            cancelled_timeouts.insert(name);
            scheduled_timeouts.erase(
                std::remove_if(scheduled_timeouts.begin(), scheduled_timeouts.end(),
                    [&name](const auto& t) { return t.name == name; }),
                scheduled_timeouts.end());
        };
    }
    
    // Test helpers
    
    /// Fire a specific timeout by name
    bool fire_timeout(const std::string& name)
    {
        for (auto it = scheduled_timeouts.begin(); it != scheduled_timeouts.end(); ++it)
        {
            if (it->name == name) {
                auto callback = std::move(it->callback);
                scheduled_timeouts.erase(it);
                callback();
                return true;
            }
        }
        return false;
    }
    
    /// Fire all timeouts (in order of scheduling)
    void fire_all_timeouts()
    {
        while (!scheduled_timeouts.empty()) {
            auto callback = std::move(scheduled_timeouts.front().callback);
            scheduled_timeouts.erase(scheduled_timeouts.begin());
            callback();
        }
    }
    
    /// Simulate time passing - fire all timeouts with delay <= elapsed_ms
    void advance_time(uint32_t elapsed_ms)
    {
        std::vector<std::function<void()>> to_fire;
        
        auto it = scheduled_timeouts.begin();
        while (it != scheduled_timeouts.end())
        {
            if (it->delay_ms <= elapsed_ms)
            {
                to_fire.push_back(std::move(it->callback));
                it = scheduled_timeouts.erase(it);
            }
            else
            {
                it->delay_ms -= elapsed_ms;
                ++it;
            }
        }
        
        for (auto& callback : to_fire)
        {
            callback();
        }
    }
    
    /// Check if a timeout is pending
    bool has_timeout(const std::string& name) const
    {
        return std::any_of(scheduled_timeouts.begin(), scheduled_timeouts.end(),
            [&name](const auto& t) { return t.name == name; });
    }
    
    /// Check if a timeout was cancelled
    bool was_cancelled(const std::string& name) const
    {
        return cancelled_timeouts.count(name) > 0;
    }
    
    /// Clear history
    void reset()
    {
        scheduled_timeouts.clear();
        cancelled_timeouts.clear();
    }

    void run_until_empty()
    {
        while (!scheduled_timeouts.empty()) {
            auto callback = std::move(scheduled_timeouts.front().callback);
            scheduled_timeouts.erase(scheduled_timeouts.begin());
            callback();
        }
    }
};

/**
 * @brief Synchronous variant of MockScheduler for testing.
 * 
 * This scheduler extends MockScheduler with a run_until_empty() method that
 * processes all scheduled timeouts synchronously until the queue is empty.
 * This is useful for tests that need to verify command execution without
 * manually firing timeouts.
 * 
 * Usage:
 *   SynchronousMockScheduler scheduler;
 *   // ... set up protocol handler with scheduler.get_setter(), etc.
 *   climate->call_transmit_state();  // Enqueues commands
 *   scheduler.run_until_empty();     // Executes all commands synchronously
 *   // Now all EXPECT_CALL assertions have been satisfied
 */
class SynchronousMockScheduler : public MockScheduler
{
public:
    /**
     * @brief Execute all scheduled timeouts until the queue is empty.
     * 
     * This method processes timeouts in FIFO order, executing their callbacks
     * immediately. If a callback schedules more timeouts, they will be processed
     * in subsequent iterations of the loop.
     * 
     * This allows tests to execute asynchronous command sequences synchronously,
     * making it possible to verify all command transmissions happen during the
     * test method execution.
     */
    void run_until_empty()
    {
        while (!scheduled_timeouts.empty()) {
            auto callback = std::move(scheduled_timeouts.front().callback);
            scheduled_timeouts.erase(scheduled_timeouts.begin());
            callback();
        }
    }
};
