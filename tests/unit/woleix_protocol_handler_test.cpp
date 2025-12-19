/* #include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_protocol_handler.h"

// Mock scheduler for testing
class MockScheduler : public ProtocolScheduler {
public:
    struct ScheduledTimeout
    {
        std::string name;
        uint32_t delay_ms;
        std::function<void()> callback;
    };
    
    std::vector<ScheduledTimeout> timeouts;
    uint32_t current_time{0};
    
    void schedule_timeout(const std::string& name, uint32_t delay_ms,
        std::function<void()> callback) override
    {
        timeouts.push_back({name, delay_ms, callback});
    }
    
    void cancel_timeout(const std::string& name) override
    {
        timeouts.erase(
            std::remove_if(timeouts.begin(), timeouts.end(),
                [&](const auto& t) { return t.name == name; }),
            timeouts.end());
    }
    
    uint32_t get_millis() const override { return current_time; }
    
    // Test helper: advance time and fire due timeouts
    void advance_time(uint32_t ms)
    {
        current_time += ms;
        // Fire timeouts that are due...
    }
};

TEST(ProtocolHandler, EntersSettingModeForTempCommand)
{
    MockTransmitter transmitter;
    MockScheduler scheduler;
    WoleixProtocolHandler handler(&transmitter, &scheduler);
    
    std::vector<WoleixCommand> commands = {
        WoleixCommand(WoleixCommand::Type::TEMP_UP, ADDRESS_NEC)
    };
    
    handler.execute(commands);
    
    EXPECT_TRUE(handler.is_in_temp_setting_mode());
    EXPECT_EQ(transmitter.transmitted_count(), 1);
    EXPECT_EQ(scheduler.timeouts.size(), 2);  // next_cmd + setting_mode timeout
} */