#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_comm.h"
#include "woleix_constants.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::remote_base;

// Helper: Get the expected timings for a command
std::vector<int32_t> get_power_timings() 
{
    return 
    {
        5568, 2832, 528, 600, 528, 600, 528, 1680, 528, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
        528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 600, 528, 1680, 528, 600,
        528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600,
        528, 1704, 516, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
    };
}

std::vector<int32_t> get_temp_up_timings()
{
    return
    {
        5568, 2832, 516, 600, 528, 576, 528, 1704, 516, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
        528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 600,
        528, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 600,
        528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
    };
}

std::vector<int32_t> get_temp_down_timings() {
    return
    {
        5568, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
        528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680,
        528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
    };
}

std::vector<int32_t> get_mode_timings()
{
    return
    {
        5592, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
        516, 1704, 516, 1704, 516, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
        516, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1704, 516, 600,
        528, 600, 528, 1704, 516, 1704, 516, 1704, 516, 1680, 528, 18780
    };
}

std::vector<int32_t> get_speed_timings()
{
    return
    {
        5568, 2832, 516, 600, 528, 600, 528, 1656, 528, 600, 528, 600, 528, 600,
        528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1632, 552, 1704,
        516, 1704, 516, 1560, 624, 1632, 552, 600, 528, 600, 528, 1584, 600, 1704,
        516, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1704, 516, 600,
        528, 600, 528, 1704, 516, 1704, 516, 1656, 528, 1704, 516, 1704, 516, 18780
    };
}

// Helper: Check if transmitted data matches expected timings
bool check_timings_match
(
    const RemoteTransmitData& data, const std::vector<int32_t>& expected
)
{
    const auto& actual = data.get_data();
    if (actual.size() != expected.size()) {
        return false;
    }

    for (size_t i = 0; i < expected.size(); i++) {
        // Allow some tolerance for timing variations
        int32_t diff = std::abs((int32_t)actual[i] - expected[i]);
        if (diff > 50) {  // 50us tolerance
        return false;
        }
    }
    return true;
}


// Mock RemoteTransmitterBase
class MockRemoteTransmitterBase : public RemoteTransmitterBase
{
public:
  MOCK_METHOD(void, send_, (const ProntoProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait), ());
  MOCK_METHOD(void, send_, (const NECProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait), ());
};

class WoleixCommTest : public testing::Test 
{
protected:
  void SetUp() override 
  {
    mock_transmitter = new MockRemoteTransmitterBase();
    mock_command_transmitter = new WoleixCommandTransmitter(mock_transmitter);    
  }
    
  void TearDown() override {
    delete mock_command_transmitter;
    delete mock_transmitter;
  }

  MockRemoteTransmitterBase* mock_transmitter;
  WoleixCommandTransmitter* mock_command_transmitter;

};
// ============================================================================
// Test: WoleixProntoCommand
// ============================================================================

/**
 * Test: WoleixProntoCommand construction and getters
 * 
 * Validates that a WoleixProntoCommand is correctly constructed with the
 * specified type, delay, and repeat count, and that all getters return
 * the expected values.
 */
TEST(WoleixProntoCommandTest, ConstructionAndGetters)
{
    WoleixProntoCommand cmd(WoleixCommandBase::Type::POWER, 200, 1);
    
    EXPECT_EQ(cmd.get_type(), WoleixCommandBase::Type::POWER);
    EXPECT_EQ(cmd.get_delay_ms(), 200);
    EXPECT_EQ(cmd.get_repeat_count(), 1);
    EXPECT_EQ(cmd.get_pronto_hex(), POWER_PRONTO);
}

/**
 * Test: WoleixProntoCommand returns correct Pronto hex for each type
 * 
 * Validates that each command type returns the correct Pronto hex string
 * as defined in woleix_constants.h.
 */
TEST(WoleixProntoCommandTest, CorrectProntoHexForAllTypes)
{
    WoleixProntoCommand power_cmd(WoleixCommandBase::Type::POWER, 200);
    EXPECT_EQ(power_cmd.get_pronto_hex(), POWER_PRONTO);
    
    WoleixProntoCommand temp_up_cmd(WoleixCommandBase::Type::TEMP_UP, 200);
    EXPECT_EQ(temp_up_cmd.get_pronto_hex(), TEMP_UP_PRONTO);
    
    WoleixProntoCommand temp_down_cmd(WoleixCommandBase::Type::TEMP_DOWN, 200);
    EXPECT_EQ(temp_down_cmd.get_pronto_hex(), TEMP_DOWN_PRONTO);
    
    WoleixProntoCommand mode_cmd(WoleixCommandBase::Type::MODE, 200);
    EXPECT_EQ(mode_cmd.get_pronto_hex(), MODE_PRONTO);
    
    WoleixProntoCommand speed_cmd(WoleixCommandBase::Type::FAN_SPEED, 200);
    EXPECT_EQ(speed_cmd.get_pronto_hex(), SPEED_PRONTO);
}

/**
 * Test: WoleixProntoCommand equality operator
 * 
 * Validates that two WoleixProntoCommand objects are equal if and only if
 * they have the same type, delay, repeat count, and pronto hex string.
 */
TEST(WoleixProntoCommandTest, EqualityOperator)
{
    WoleixProntoCommand cmd1(WoleixCommandBase::Type::POWER, 200, 1);
    WoleixProntoCommand cmd2(WoleixCommandBase::Type::POWER, 200, 1);
    WoleixProntoCommand cmd3(WoleixCommandBase::Type::MODE, 200, 1);
    
    EXPECT_TRUE(cmd1 == cmd2);
    EXPECT_FALSE(cmd1 == cmd3);
}

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
    WoleixNecCommand cmd(WoleixCommandBase::Type::POWER, 0x00FF, 200, 1);
    
    EXPECT_EQ(cmd.get_type(), WoleixCommandBase::Type::POWER);
    EXPECT_EQ(cmd.get_address(), 0x00FF);
    EXPECT_EQ(cmd.get_delay_ms(), 200);
    EXPECT_EQ(cmd.get_repeat_count(), 1);
    EXPECT_EQ(cmd.get_command_code(), POWER_NEC);
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
    
    WoleixNecCommand power_cmd(WoleixCommandBase::Type::POWER, address, 200);
    EXPECT_EQ(power_cmd.get_command_code(), POWER_NEC);
    
    WoleixNecCommand temp_up_cmd(WoleixCommandBase::Type::TEMP_UP, address, 200);
    EXPECT_EQ(temp_up_cmd.get_command_code(), TEMP_UP_NEC);
    
    WoleixNecCommand temp_down_cmd(WoleixCommandBase::Type::TEMP_DOWN, address, 200);
    EXPECT_EQ(temp_down_cmd.get_command_code(), TEMP_DOWN_NEC);
    
    WoleixNecCommand mode_cmd(WoleixCommandBase::Type::MODE, address, 200);
    EXPECT_EQ(mode_cmd.get_command_code(), MODE_NEC);
    
    WoleixNecCommand speed_cmd(WoleixCommandBase::Type::FAN_SPEED, address, 200);
    EXPECT_EQ(speed_cmd.get_command_code(), SPEED_NEC);
}

/**
 * Test: WoleixNecCommand equality operator
 * 
 * Validates that two WoleixNecCommand objects are equal if and only if
 * they have the same type, address, delay, repeat count, and command code.
 */
TEST(WoleixNecCommandTest, EqualityOperator)
{
    WoleixNecCommand cmd1(WoleixCommandBase::Type::POWER, 0x00FF, 200, 1);
    WoleixNecCommand cmd2(WoleixCommandBase::Type::POWER, 0x00FF, 200, 1);
    WoleixNecCommand cmd3(WoleixCommandBase::Type::MODE, 0x00FF, 200, 1);
    WoleixNecCommand cmd4(WoleixCommandBase::Type::POWER, 0x00FE, 200, 1);
    
    EXPECT_TRUE(cmd1 == cmd2);
    EXPECT_FALSE(cmd1 == cmd3);  // Different type
    EXPECT_FALSE(cmd1 == cmd4);  // Different address
}

// ============================================================================
// Test: WoleixCommandTransmitter with Pronto Commands
// ============================================================================

/**
 * Test: WoleixCommandTransmitter transmits Pronto commands correctly
 * 
 * Validates that when a WoleixProntoCommand is passed to the transmitter,
 * it correctly calls the underlying ProntoProtocol transmit method with
 * the expected parameters.
 */
TEST(WoleixCommandTransmitterTest, TransmitsProntoCommand)
{
    MockRemoteTransmitterBase mock_transmitter;
    WoleixCommandTransmitter transmitter(&mock_transmitter);
    
    // Create a POWER command
    WoleixProntoCommand power_cmd(WoleixCommandBase::Type::POWER, 200, 1);
    
    // Expect the Pronto transmit method to be called with correct parameters
    EXPECT_CALL(mock_transmitter, send_(testing::An<const ProntoProtocol::ProtocolData&>(), 1, 200))
        .Times(1)
        .WillOnce(testing::Invoke([&power_cmd](const ProntoProtocol::ProtocolData& data, 
                                                 uint16_t repeats, uint16_t wait) {
            EXPECT_EQ(data.data, power_cmd.get_pronto_hex());
            EXPECT_EQ(repeats, power_cmd.get_repeat_count());
            EXPECT_EQ(wait, power_cmd.get_delay_ms());
        }));
    
    // Transmit the command
    transmitter.transmit_(power_cmd);
}

/**
 * Test: WoleixCommandTransmitter handles multiple Pronto commands
 * 
 * Validates that the transmitter correctly transmits a sequence of
 * multiple Pronto commands.
 */
TEST(WoleixCommandTransmitterTest, TransmitsMultipleProntoCommands)
{
    MockRemoteTransmitterBase mock_transmitter;
    WoleixCommandTransmitter transmitter(&mock_transmitter);
    
    std::vector<WoleixCommand> commands;
    commands.push_back(WoleixProntoCommand(WoleixCommandBase::Type::POWER, 200, 1));
    commands.push_back(WoleixProntoCommand(WoleixCommandBase::Type::MODE, 200, 2));
    
    // Expect two Pronto transmit calls
    EXPECT_CALL(mock_transmitter, send_(testing::An<const ProntoProtocol::ProtocolData&>(), 
                                         testing::_, testing::_))
        .Times(2);
    
    transmitter.transmit_(commands);
}

// ============================================================================
// Test: WoleixCommandTransmitter with NEC Commands
// ============================================================================

/**
 * Test: WoleixCommandTransmitter transmits NEC commands correctly
 * 
 * Validates that when a WoleixNecCommand is passed to the transmitter,
 * it correctly calls the underlying NECProtocol transmit method with
 * the expected parameters.
 */
TEST(WoleixCommandTransmitterTest, TransmitsNecCommand)
{
    MockRemoteTransmitterBase mock_transmitter;
    WoleixCommandTransmitter transmitter(&mock_transmitter);
    
    // Create a POWER command
    uint16_t address = 0x00FF;
    WoleixNecCommand power_cmd(WoleixCommandBase::Type::POWER, address, 200, 3);
    
    // Expect the NEC transmit method to be called with correct parameters
    EXPECT_CALL(mock_transmitter, send_(testing::An<const NECProtocol::ProtocolData&>(), 3, 200))
        .Times(1)
        .WillOnce(testing::Invoke([&power_cmd, address](const NECProtocol::ProtocolData& data, 
                                                          uint16_t repeats, uint16_t wait) {
            EXPECT_EQ(data.address, address);
            EXPECT_EQ(data.command, power_cmd.get_command_code());
            EXPECT_EQ(data.command_repeats, 3);
            EXPECT_EQ(repeats, 3);
            EXPECT_EQ(wait, 200);
        }));
    
    // Transmit the command
    transmitter.transmit_(power_cmd);
}

/**
 * Test: WoleixCommandTransmitter handles multiple NEC commands
 * 
 * Validates that the transmitter correctly transmits a sequence of
 * multiple NEC commands.
 */
TEST(WoleixCommandTransmitterTest, TransmitsMultipleNecCommands)
{
    MockRemoteTransmitterBase mock_transmitter;
    WoleixCommandTransmitter transmitter(&mock_transmitter);
    
    uint16_t address = 0x00FF;
    std::vector<WoleixCommand> commands;
    commands.push_back(WoleixNecCommand(WoleixCommandBase::Type::POWER, address, 200, 1));
    commands.push_back(WoleixNecCommand(WoleixCommandBase::Type::MODE, address, 200, 2));
    
    // Expect two NEC transmit calls
    EXPECT_CALL(mock_transmitter, send_(testing::An<const NECProtocol::ProtocolData&>(), 
                                         testing::_, testing::_))
        .Times(2);
    
    transmitter.transmit_(commands);
}

/**
 * Test: WoleixCommandTransmitter handles mixed Pronto and NEC commands
 * 
 * Validates that the transmitter correctly transmits a sequence containing
 * both Pronto and NEC commands.
 */
TEST(WoleixCommandTransmitterTest, TransmitsMixedCommands)
{
    MockRemoteTransmitterBase mock_transmitter;
    WoleixCommandTransmitter transmitter(&mock_transmitter);
    
    uint16_t address = 0x00FF;
    std::vector<WoleixCommand> commands;
    commands.push_back(WoleixProntoCommand(WoleixCommandBase::Type::POWER, 200, 1));
    commands.push_back(WoleixNecCommand(WoleixCommandBase::Type::MODE, address, 200, 2));
    commands.push_back(WoleixProntoCommand(WoleixCommandBase::Type::TEMP_UP, 400, 1));
    
    // Expect three transmit calls total (Pronto and NEC methods)
    EXPECT_CALL(mock_transmitter, send_(testing::An<const ProntoProtocol::ProtocolData&>(), 
                                         testing::_, testing::_))
        .Times(2);  // 2 Pronto commands
    
    EXPECT_CALL(mock_transmitter, send_(testing::An<const NECProtocol::ProtocolData&>(), 
                                         testing::_, testing::_))
        .Times(1);  // 1 NEC command
    
    transmitter.transmit_(commands);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
