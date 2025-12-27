#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_constants.h"
#include "woleix_command.h"

using namespace esphome::climate_ir_woleix;

using esphome::climate_ir_woleix::POWER_NEC;
using esphome::climate_ir_woleix::MODE_NEC;
using esphome::climate_ir_woleix::SPEED_NEC;
using esphome::climate_ir_woleix::TEMP_UP_NEC;
using esphome::climate_ir_woleix::TEMP_DOWN_NEC;
using esphome::climate_ir_woleix::WoleixCommand;

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
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
