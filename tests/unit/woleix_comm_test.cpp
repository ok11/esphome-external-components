#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_constants.h"
#include "woleix_command.h"
#include "woleix_protocol_handler.h"

#include "mock_scheduler.h"
#include "mock_queue.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::remote_base;

// Mock RemoteTransmitterBase
class MockRemoteTransmitterBase : public RemoteTransmitterBase
{
public:
    MOCK_METHOD(void, send_, (const NECProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait), ());
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
    bool is_in_temp_setting_mode() const
    {
        return is_in_temp_setting_mode_();
    }
};

class WoleixProtocolHandlerTest : public testing::Test 
{
protected:
    void SetUp() override 
    {
        mock_queue = new MockWoleixCommandQueue();
        mock_scheduler = new MockScheduler(); 
        mock_transmitter = new MockRemoteTransmitterBase();
        mock_protocol_handler = new MockWoleixProtocolHandler(mock_transmitter, mock_queue, mock_scheduler);
    }
        
    void TearDown() override {
        delete mock_protocol_handler;
        delete mock_transmitter;
        delete mock_scheduler;
        delete mock_queue;
    }

    MockWoleixCommandQueue* mock_queue;
    MockScheduler* mock_scheduler;
    MockRemoteTransmitterBase* mock_transmitter;
    MockWoleixProtocolHandler* mock_protocol_handler;

};

// ============================================================================
// Test: WoleixNecCommand
// ============================================================================

/**
 * Test: WoleixNecCommand construction and getters
 * 
 * Validates that a WoleixNecCommand is correctly constructed with the
 * specified type, address, delay, and repeat count, and that all getters
 * return the expected values.
 */
TEST(WoleixNecCommandTest, ConstructionAndGetters)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0x00FF, 200, 1);
    
    EXPECT_EQ(cmd.get_type(), WoleixCommand::Type::POWER);
    EXPECT_EQ(cmd.get_address(), 0x00FF);
//    EXPECT_EQ(cmd.get_delay_ms(), 200);
    EXPECT_EQ(cmd.get_repeat_count(), 1);
    EXPECT_EQ(cmd.get_command(), POWER_NEC);
}

/**
 * Test: WoleixNecCommand returns correct command code for each type
 * 
 * Validates that each command type returns the correct NEC command code
 * as defined in woleix_constants.h.
 */
TEST(WoleixNecCommandTest, CorrectCommandCodeForAllTypes)
{
    uint16_t address = 0x00FF;
    
    WoleixCommand power_cmd(WoleixCommand::Type::POWER, address, 200);
    EXPECT_EQ(power_cmd.get_command(), POWER_NEC);
    
    WoleixCommand temp_up_cmd(WoleixCommand::Type::TEMP_UP, address, 200);
    EXPECT_EQ(temp_up_cmd.get_command(), TEMP_UP_NEC);
    
    WoleixCommand temp_down_cmd(WoleixCommand::Type::TEMP_DOWN, address, 200);
    EXPECT_EQ(temp_down_cmd.get_command(), TEMP_DOWN_NEC);
    
    WoleixCommand mode_cmd(WoleixCommand::Type::MODE, address, 200);
    EXPECT_EQ(mode_cmd.get_command(), MODE_NEC);
    
    WoleixCommand speed_cmd(WoleixCommand::Type::FAN_SPEED, address, 200);
    EXPECT_EQ(speed_cmd.get_command(), SPEED_NEC);
}

/**
 * Test: WoleixNecCommand equality operator
 * 
 * Validates that two WoleixNecCommand objects are equal if and only if
 * they have the same type, address, delay, repeat count, and command code.
 */
TEST(WoleixNecCommandTest, EqualityOperator)
{
    WoleixCommand cmd1(WoleixCommand::Type::POWER, 0x00FF, 200, 1);
    WoleixCommand cmd2(WoleixCommand::Type::POWER, 0x00FF, 200, 1);
    WoleixCommand cmd3(WoleixCommand::Type::MODE, 0x00FF, 200, 1);
    WoleixCommand cmd4(WoleixCommand::Type::POWER, 0x00FE, 200, 1);
    
    EXPECT_TRUE(cmd1 == cmd2);
    EXPECT_FALSE(cmd1 == cmd3);  // Different type
    EXPECT_FALSE(cmd1 == cmd4);  // Different address
}


// ============================================================================
// Test: WoleixProtocolHandler with NEC Commands
// ============================================================================

/**
 * Test: WoleixProtocolHandler handles NEC commands correctly
 * 
 * Validates that when a WoleixNecCommand is passed to the transmitter,
 * it correctly calls the underlying NECProtocol transmit method with
 * the expected parameters.
 */
TEST_F(WoleixProtocolHandlerTest, TransmitsNecCommand)
{
    // Create a POWER command
    uint16_t address = 0x00FF;
    WoleixCommand power_cmd(WoleixCommand::Type::POWER, address, 200, 3);
    
    // Expect the NEC transmit method to be called with correct parameters
    EXPECT_CALL(*mock_transmitter, send_(testing::An<const NECProtocol::ProtocolData&>(), 3, 200000))
        .Times(1)
        .WillOnce
        (
            testing::Invoke
            (
                [&power_cmd, address]
                (const NECProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait)
                {
                    EXPECT_EQ(data.address, address);
                    EXPECT_EQ(data.command, power_cmd.get_command());
                    EXPECT_EQ(data.command_repeats, 1);
                    EXPECT_EQ(repeats, 3);
                    EXPECT_EQ(wait, 200000);
                }
            )
        );
    
    // Transmit the command
//    mock_protocol_handler->execute(power_cmd);
}

/**
 * Test: WoleixCommandTransmitter handles multiple NEC commands
 * 
 * Validates that the transmitter correctly transmits a sequence of
 * multiple NEC commands.
 */
TEST_F(WoleixProtocolHandlerTest, TransmitsMultipleNecCommands)
{
    uint16_t address = 0x00FF;
    std::vector<WoleixCommand> commands;
    commands.push_back(WoleixCommand(WoleixCommand::Type::POWER, address, 200, 1));
    commands.push_back(WoleixCommand(WoleixCommand::Type::MODE, address, 200, 2));
    
    // Expect two NEC transmit calls
    EXPECT_CALL(*mock_transmitter, send_(testing::An<const NECProtocol::ProtocolData&>(), 
            testing::_, testing::_))
        .Times(2);
    
//    mock_protocol_handler->execute(commands);
}

TEST_F(WoleixProtocolHandlerTest, EntersSettingModeForTempCommand)
{
    const std::vector<WoleixCommand> commands = {
        WoleixCommand(WoleixCommand::Type::TEMP_UP, ADDRESS_NEC)
    };
    
//    mock_protocol_handler->execute(commands);
    
    EXPECT_TRUE(mock_protocol_handler->is_in_temp_setting_mode());
    EXPECT_EQ(mock_scheduler->scheduled_timeouts.size(), 2);  // next_cmd + setting_mode timeout
}
// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
