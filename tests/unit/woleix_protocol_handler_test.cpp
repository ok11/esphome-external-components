#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_constants.h"
#include "woleix_command.h"
#include "woleix_protocol_handler.h"

#include "mock_scheduler.h"
#include "mock_queue.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::remote_base;

using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::Invoke;

class MockRemoteTransmitter : public RemoteTransmitterBase
{
public:
    struct TransmittedCommand
    {
        uint16_t address;
        uint16_t command;
        uint32_t repeats;
    };
    
    std::vector<TransmittedCommand> transmitted;
    
    void send_(const NECProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait) override
    {
        transmitted.push_back({data.address, data.command, repeats});
    }
    
    size_t transmit_count() const { return transmitted.size(); }
    
    void clear() { transmitted.clear(); }
    
    bool last_was(WoleixCommand::Type type) const
    {
        if (transmitted.empty()) return false;
        return transmitted.back().command == static_cast<uint16_t>(type);
    }
    
    size_t count_type(WoleixCommand::Type type) const
    {
        return std::count_if(transmitted.begin(), transmitted.end(),
            [type](const auto& cmd) { return cmd.command == static_cast<uint16_t>(type); });
    }
};

class MockWoleixProtocolHandler : public WoleixProtocolHandler
{
public:
    MockWoleixProtocolHandler(RemoteTransmitterBase* transmitter, MockWoleixCommandQueue* command_queue, MockScheduler* mock_scheduler)
        : WoleixProtocolHandler(mock_scheduler->get_setter(), mock_scheduler->get_canceller())
    {
        set_transmitter(transmitter);
        setup(command_queue);
    }
    bool is_in_setting_mode() const
    {
        return is_in_temp_setting_mode_();
    }

    void call_transmit(const WoleixCommand& cmd)
    {
        transmit_(cmd);
    }

    MOCK_METHOD(void, report, (const WoleixStatus&));
};


class ProtocolHandlerTest : public testing::Test
{
protected:
    void SetUp() override
    {
        mock_scheduler = new MockScheduler();
        mock_transmitter = new MockRemoteTransmitter();
        mock_command_queue = new MockWoleixCommandQueue();
        mock_protocol_handler = new MockWoleixProtocolHandler(
            mock_transmitter, mock_command_queue, mock_scheduler);
    }
    
    void TearDown() override
    {
        delete mock_protocol_handler;
        delete mock_command_queue;
        delete mock_transmitter;
        delete mock_scheduler;
    }
    
    // Helpers to enqueue commands
    void enqueue(WoleixCommand::Type type, uint32_t repeats = 1)
    {
        enqueue(WoleixCommand(type, ADDRESS_NEC, 0, repeats));
    }

    void enqueue(const WoleixCommand& command)
    {
        mock_command_queue->enqueue(command);
    }
    
    // Helper to process one command timeout
    void process_one()
    {
        mock_scheduler->fire_timeout("proto_next_cmd");
    }
    
    // Helper to drain queue without triggering setting mode timeout
    void drain_queue_fast()
    {
        while (!mock_command_queue->is_empty())
        {
            process_one();
        }
    }

    MockScheduler* mock_scheduler;
    MockRemoteTransmitter* mock_transmitter;
    MockWoleixCommandQueue* mock_command_queue;
    MockWoleixProtocolHandler* mock_protocol_handler;
};

// ============================================================================
// Basic Command Processing Tests
// ============================================================================

TEST_F(ProtocolHandlerTest, ProcessesSinglePowerCommand)
{
    uint16_t address = 0xFF00;
    WoleixCommand power(WoleixCommand::Type::POWER, address, 0, 3);
    enqueue(power);
    
    // Fire the polling timeout to process the command
    process_one();
    
    EXPECT_EQ(mock_transmitter->transmit_count(), 1);
    EXPECT_TRUE(mock_transmitter->last_was(WoleixCommand::Type::POWER));
    EXPECT_EQ(mock_transmitter->transmitted.at(0).address, address);
    EXPECT_EQ(mock_transmitter->transmitted.at(0).repeats, 3);
    EXPECT_TRUE(mock_command_queue->is_empty());
}

TEST_F(ProtocolHandlerTest, ProcessesMultipleRegularCommands)
{
    enqueue(WoleixCommand::Type::POWER);
    enqueue(WoleixCommand::Type::MODE);
    enqueue(WoleixCommand::Type::MODE);
    
    drain_queue_fast();
    
    EXPECT_EQ(mock_transmitter->transmit_count(), 3);
    EXPECT_EQ(mock_transmitter->count_type(WoleixCommand::Type::POWER), 1);
    EXPECT_EQ(mock_transmitter->count_type(WoleixCommand::Type::MODE), 2);
    EXPECT_TRUE(mock_command_queue->is_empty());
}

// ============================================================================
// Temperature Setting Mode Tests
// 
// IMPORTANT: The Woleix AC protocol requires n+1 IR commands to change
// temperature by n degrees:
//   - First press: enters "setting mode" (displays current temp, no change)
//   - Subsequent presses: actually change the temperature
// ============================================================================

TEST_F(ProtocolHandlerTest, TempCommandEntersSettingMode)
{
    enqueue(WoleixCommand::Type::TEMP_UP);
    
    process_one();
    
    // Should be in setting mode after first temp command
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    // First transmission enters setting mode
    EXPECT_EQ(mock_transmitter->transmit_count(), 1);
    
    // Setting mode timeout should be scheduled
    EXPECT_TRUE(mock_scheduler->has_timeout("proto_setting_mode"));
    
    // Command should still be in queue (will be sent again to actually change temp)
    EXPECT_FALSE(mock_command_queue->is_empty());
}

TEST_F(ProtocolHandlerTest, SingleTempCommandTransmitsTwice)
{
    // To change temp by 1 degree, we need 2 transmissions:
    // 1. Enter setting mode
    // 2. Actually change the temperature
    enqueue(WoleixCommand::Type::TEMP_UP);
    
    drain_queue_fast();
    
    // n+1 = 1+1 = 2 transmissions for 1 queued command
    EXPECT_EQ(mock_transmitter->transmit_count(), 2);
    EXPECT_TRUE(mock_command_queue->is_empty());
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
}

TEST_F(ProtocolHandlerTest, MultipleTempCommandsFollowNPlusOneRule)
{
    // Enqueue 5 temperature up commands (like going from 25°C to 30°C)
    // Expected transmissions: 1 (enter mode) + 5 (actual changes) = 6
    for (int i = 0; i < 5; i++)
    {
        enqueue(WoleixCommand::Type::TEMP_UP);
    }
    
    drain_queue_fast();
    
    // n+1 rule: 5 commands -> 6 transmissions
    EXPECT_EQ(mock_transmitter->transmit_count(), 6);
    EXPECT_TRUE(mock_command_queue->is_empty());
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    EXPECT_TRUE(mock_scheduler->has_timeout("proto_setting_mode"));
}

TEST_F(ProtocolHandlerTest, SettingModeTimeoutExitsSettingMode)
{
    enqueue(WoleixCommand::Type::TEMP_UP);
    drain_queue_fast();
    
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    
    // Now advance time past the 5-second timeout
    mock_scheduler->advance_time(5001);
    
    EXPECT_FALSE(mock_protocol_handler->is_in_setting_mode());
}

TEST_F(ProtocolHandlerTest, SettingModeTimeoutIsExtendedByEachTempCommand)
{
    enqueue(WoleixCommand::Type::TEMP_UP);
    process_one();  // Enter setting mode
    
    uint64_t first_timeout = mock_scheduler->get_timeout_time("proto_setting_mode");
    
    // Advance 2 seconds
    mock_scheduler->advance_time(2000);
    
    // Process the same command again (it's still in queue)
    process_one();
    
    // Timeout should have been extended
    uint64_t second_timeout = mock_scheduler->get_timeout_time("proto_setting_mode");
    EXPECT_GT(second_timeout, first_timeout);
    
    // Should still be in setting mode
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
}

TEST_F(ProtocolHandlerTest, TempCommandAfterTimeoutReEntersSettingMode)
{
    // First temp command sequence
    enqueue(WoleixCommand::Type::TEMP_UP);
    drain_queue_fast();
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    EXPECT_EQ(mock_transmitter->transmit_count(), 2);  // n+1 = 2
    
    // Let setting mode timeout
    mock_scheduler->advance_time(6000);
    EXPECT_FALSE(mock_protocol_handler->is_in_setting_mode());
    
    // Now send another temp command - should re-enter setting mode
    enqueue(WoleixCommand::Type::TEMP_DOWN);
    drain_queue_fast();
    
    // Should re-enter setting mode with n+1 rule again
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    EXPECT_EQ(mock_transmitter->transmit_count(), 4);  // 2 from before + 2 new (n+1)
}

// ============================================================================
// Mixed Command Sequence Tests
// ============================================================================

TEST_F(ProtocolHandlerTest, MixedSequencePowerThenTemp)
{
    enqueue(WoleixCommand::Type::POWER);
    enqueue(WoleixCommand::Type::TEMP_UP);
    enqueue(WoleixCommand::Type::TEMP_UP);
    
    drain_queue_fast();
    
    // POWER: 1 transmission
    // TEMP_UP x2: 1 (enter mode) + 2 (actual changes) = 3 transmissions
    // Total: 4
    EXPECT_EQ(mock_transmitter->transmit_count(), 4);
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    
    // Verify order
    EXPECT_EQ(mock_transmitter->transmitted[0].command, POWER_NEC);
    EXPECT_EQ(mock_transmitter->transmitted[1].command, TEMP_UP_NEC);  // Enter mode
    EXPECT_EQ(mock_transmitter->transmitted[2].command, TEMP_UP_NEC);  // First actual change
    EXPECT_EQ(mock_transmitter->transmitted[3].command, TEMP_UP_NEC);  // Second actual change
}

TEST_F(ProtocolHandlerTest, MixedSequenceTempThenMode)
{
    // TEMP_UP then MODE - the MODE should work even in setting mode
    enqueue(WoleixCommand::Type::TEMP_UP);
    enqueue(WoleixCommand::Type::MODE);
    
    drain_queue_fast();
    
    // TEMP_UP: 2 transmissions (n+1 rule)
    // MODE: 1 transmission
    // Total: 3
    EXPECT_EQ(mock_transmitter->transmit_count(), 3);
}

TEST_F(ProtocolHandlerTest, ComplexSequenceFromStateMachine)
{
    // Simulate a real transition: Power ON, then temp adjustment 25->28 (+3)
    // This is what the state machine would generate
    
    enqueue(WoleixCommand::Type::POWER);      // Turn on
    enqueue(WoleixCommand::Type::TEMP_UP);    // 25 -> 26
    enqueue(WoleixCommand::Type::TEMP_UP);    // 26 -> 27
    enqueue(WoleixCommand::Type::TEMP_UP);    // 27 -> 28
    
    drain_queue_fast();
    
    // POWER: 1
    // TEMP_UP x3: 1 (enter mode) + 3 (actual changes) = 4
    // Total: 5
    EXPECT_EQ(mock_transmitter->transmit_count(), 5);
    EXPECT_TRUE(mock_command_queue->is_empty());
    
    // Verify order
    EXPECT_EQ(mock_transmitter->transmitted[0].command, POWER_NEC);
    EXPECT_EQ(mock_transmitter->transmitted[1].command, TEMP_UP_NEC);  // Enter mode
    EXPECT_EQ(mock_transmitter->transmitted[2].command, TEMP_UP_NEC);  // +1
    EXPECT_EQ(mock_transmitter->transmitted[3].command, TEMP_UP_NEC);  // +2
    EXPECT_EQ(mock_transmitter->transmitted[4].command, TEMP_UP_NEC);  // +3
}

// ============================================================================
// Timing Verification Tests
// ============================================================================

TEST_F(ProtocolHandlerTest, CommandDelaysAreRespected)
{
    enqueue(WoleixCommand::Type::POWER);
    enqueue(WoleixCommand::Type::MODE);
    
    process_one();  // Process POWER
    EXPECT_EQ(mock_transmitter->transmit_count(), 1);
    
    // Next command should be scheduled with inter-command delay
    EXPECT_TRUE(mock_scheduler->has_timeout("proto_next_cmd"));
    
    // The second command shouldn't fire until the delay passes
    uint32_t remaining = mock_scheduler->time_until("proto_next_cmd");
    EXPECT_GT(remaining, 0);  // There should be some delay
}

TEST_F(ProtocolHandlerTest, SettingModeEnterDelayIsRespected)
{
    enqueue(WoleixCommand::Type::TEMP_UP);
    enqueue(WoleixCommand::Type::TEMP_UP);
    
    // Process first TEMP_UP - enters setting mode
    process_one();
    EXPECT_EQ(mock_transmitter->transmit_count(), 1);
    
    // The next command timeout should have the enter delay
    // (TEMP_ENTER_DELAY_MS = 150ms in protocol handler)
    EXPECT_TRUE(mock_scheduler->has_timeout("proto_next_cmd"));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ProtocolHandlerTest, ResetClearsSettingMode)
{
    enqueue(WoleixCommand::Type::TEMP_UP);
    drain_queue_fast();
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
    
    mock_protocol_handler->reset();
    
    EXPECT_FALSE(mock_protocol_handler->is_in_setting_mode());
}

TEST_F(ProtocolHandlerTest, LargeTemperatureChange)
{
    // Simulate going from 15°C to 30°C (+15 degrees)
    // Need 15 queued commands -> 16 transmissions (n+1 rule)
    for (int i = 0; i < 15; i++)
    {
        enqueue(WoleixCommand::Type::TEMP_UP);
    }
    
    drain_queue_fast();
    
    EXPECT_EQ(mock_transmitter->transmit_count(), 16);  // n+1 = 15+1
    EXPECT_TRUE(mock_protocol_handler->is_in_setting_mode());
}

TEST_F(ProtocolHandlerTest, TransmitterNotSet)
{
    mock_protocol_handler->set_transmitter(nullptr);

    EXPECT_CALL(*mock_protocol_handler, report(_))
        .Times(1)
        .WillOnce(Invoke([](const WoleixStatus& status) {
            EXPECT_EQ(status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_ERROR);
            EXPECT_EQ(status.get_category(), WoleixCategory::ProtocolHandler::WX_CATEGORY_TRANSMITTER_NOT_SET);
        }));
    
    mock_protocol_handler->call_transmit(WoleixCommand(WoleixCommand::Type::POWER, 0xFFFF));
}

TEST_F(ProtocolHandlerTest, MixedTempUpAndDown)
{
    // Unusual but possible: UP then DOWN
    // Each direction change would need to re-enter setting mode if we
    // had timed out, but if we're still in setting mode, they chain
    enqueue(WoleixCommand::Type::TEMP_UP);
    enqueue(WoleixCommand::Type::TEMP_DOWN);
    
    drain_queue_fast();
    
    // TEMP_UP: 2 (enter + change)
    // TEMP_DOWN: 1 (already in setting mode)
    // Total: 3
    EXPECT_EQ(mock_transmitter->transmit_count(), 3);
    EXPECT_EQ(mock_transmitter->count_type(WoleixCommand::Type::TEMP_UP), 2);
    EXPECT_EQ(mock_transmitter->count_type(WoleixCommand::Type::TEMP_DOWN), 1);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
