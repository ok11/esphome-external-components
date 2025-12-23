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
};