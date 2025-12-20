#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <numeric>

#include "climate_ir_woleix.h"
#include "woleix_state_machine.h"

using namespace esphome::climate_ir_woleix;

// Custom GMock matcher for checking WoleixCommand type
MATCHER_P(IsCommandType, expected_type, "")
{
    return arg.get_type() == expected_type;
}

static const WoleixCommand::Type POWER_COMMAND = WoleixCommand::Type::POWER;
static const WoleixCommand::Type MODE_COMMAND = WoleixCommand::Type::MODE;
static const WoleixCommand::Type TEMP_UP_COMMAND = WoleixCommand::Type::TEMP_UP;
static const WoleixCommand::Type TEMP_DOWN_COMMAND = WoleixCommand::Type::TEMP_DOWN;
static const WoleixCommand::Type SPEED_COMMAND = WoleixCommand::Type::FAN_SPEED;

class MockWoleixStateMachine : public WoleixStateMachine
{
public:
    void set_current_state(
        WoleixPowerState power, WoleixMode mode, float temperature, WoleixFanSpeed fan_speed
    )
    {
        current_state_.power = power;
        current_state_.mode = mode;
        current_state_.temperature = temperature;
        current_state_.fan_speed = fan_speed;
    }

    using WoleixStateMachine::transit_to_state;
};

// Test fixture for WoleixStateMachine
class WoleixStateMachineTest : public testing::Test
{
protected:
    void SetUp() override
    {
        state_machine_ = new MockWoleixStateMachine();
    }
    
    void TearDown() override
    {
        delete state_machine_;
    }
    
    MockWoleixStateMachine* state_machine_;
    
    // Helper to count specific commands in a vector and sum their repeat counts
    int count_command(const std::vector<WoleixCommand>& commands, WoleixCommand::Type type)
    {
        return std::accumulate(commands.begin(), commands.end(), 0,
            [type](int sum, const WoleixCommand& cmd)
            {
                return sum + (cmd.get_type() == type ? cmd.get_repeat_count() : 0);
            }
        );
    }
};

// ============================================================================
// Test: Initialization and Default State
// ============================================================================

/**
 * Test: State machine initializes with correct default values
 * 
 * Verifies that on construction, the state machine starts with device defaults:
 * power=ON, mode=COOL, temperature=25°C, fan_speed=LOW
 */
TEST_F(WoleixStateMachineTest, InitialStateIsCorrect)
{
    auto state = state_machine_->get_state();
    
    EXPECT_EQ(state.power, WoleixPowerState::OFF);
    EXPECT_EQ(state.mode, WoleixMode::COOL);
    EXPECT_FLOAT_EQ(state.temperature, 25.0f);
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::LOW);
}

/**
 * Test: reset() restores state machine to defaults
 * 
 * Validates that calling reset() after changing state restores all values
 * to the device defaults, providing a way to recover from state sync issues.
 */
TEST_F(WoleixStateMachineTest, ResetRestoresDefaultState)
{
    // Set state
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        20.0f,
        WoleixFanSpeed::HIGH
    );

    // Reset
    state_machine_->reset();
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::OFF);
    EXPECT_EQ(state.mode, WoleixMode::COOL);
    EXPECT_FLOAT_EQ(state.temperature, 25.0f);
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::LOW);
}

// ============================================================================
// Test: Power State Transitions
// ============================================================================

/**
 * Test: Turning power OFF from ON sends single POWER command
 * 
 * Validates that transitioning from ON to OFF state generates exactly
 * one POWER IR command and updates the internal power state.
 */
TEST_F(WoleixStateMachineTest, PowerOffFromOnSendsPowerCommand)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    // Start from ON (default state)
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(count_command(commands, POWER_COMMAND), 1);
    
    // Verify state updated
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::OFF);
}

/**
 * Test: Turning power ON from OFF sends POWER command
 * 
 * When powering on from OFF, the state machine resets to defaults and
 * then generates commands to reach the requested state. Verifies POWER
 * command is sent and state is updated to ON.
 */
TEST_F(WoleixStateMachineTest, PowerOnFromOffSendsPowerCommand)
{
    // First turn off
    state_machine_->set_current_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Now turn back on
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,  // Request FAN mode
        20.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Should send: POWER (on), MODE x2 (COOL->DEHUM->FAN), TEMP_DOWN x5, SPEED
    EXPECT_EQ(count_command(commands, POWER_COMMAND), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::ON);
}

/**
 * Test: Power OFF ignores other state parameters
 * 
 * When turning power OFF, mode, temperature, and fan speed parameters
 * should be ignored. Only the POWER command is sent regardless of
 * other requested changes.
 */
TEST_F(WoleixStateMachineTest, PowerOffIgnoresOtherStateChanges)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::OFF,
        WoleixMode::FAN,     // These should be ignored
        30.0f,               // These should be ignored
        WoleixFanSpeed::HIGH // These should be ignored
    );
    
    // Only POWER command should be sent
    EXPECT_EQ(commands.size(), 1);
    EXPECT_EQ(count_command(commands, POWER_COMMAND), 1);
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 0);
    EXPECT_EQ(count_command(commands, SPEED_COMMAND), 0);
}

// ============================================================================
// Test: Mode Transitions (Circular)
// ============================================================================

/**
 * Test: COOL→DEHUM mode transition requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from COOL to DEHUM is the
 * first step in the cycle, requiring exactly 1 MODE command.
 */
TEST_F(WoleixStateMachineTest, ModeTransitionCoolToDehum)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // COOL -> DEHUM = 1 step
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: COOL→FAN mode transition requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from COOL to FAN requires
 * passing through DEHUM: COOL→DEHUM→FAN (2 steps).
 */
TEST_F(WoleixStateMachineTest, ModeTransitionCoolToFan)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // COOL -> DEHUM -> FAN = 2 steps
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 2);
}

/**
 * Test: DEHUM→FAN mode transition requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from DEHUM to FAN is the
 * next step in the cycle, requiring exactly 1 MODE command.
 */
TEST_F(WoleixStateMachineTest, ModeTransitionDehumToFan)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );

    // Now go to FAN
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // DEHUM -> FAN = 1 step
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: FAN→COOL mode transition requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from FAN to COOL wraps around
 * the cycle, requiring exactly 1 MODE command: FAN→COOL.
 */
TEST_F(WoleixStateMachineTest, ModeTransitionFanToCool)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Now go to COOL
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // FAN -> COOL = 1 step (wraps around)
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: DEHUM→COOL mode transition requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from DEHUM to COOL requires
 * passing through FAN: DEHUM→FAN→COOL (2 steps).
 */
TEST_F(WoleixStateMachineTest, ModeTransitionDehumToCool)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Now go to COOL
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // DEHUM -> FAN -> COOL = 2 steps
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 2);
}

/**
 * Test: No mode change means no MODE commands
 * 
 * When the target mode matches the current mode, no MODE commands
 * should be generated.
 */
TEST_F(WoleixStateMachineTest, NoModeChangeGeneratesNoModeCommands)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 0);
}

// ============================================================================
// Test: Temperature Adjustments (COOL mode only)
// ============================================================================

/**
 * Test: Temperature increase in COOL mode generates TEMP_UP commands
 * 
 * Validates that increasing temperature (e.g., 25°C to 28°C) in COOL mode
 * generates the correct number of TEMP_UP commands (3 in this case) and
 * updates the internal temperature state.
 */
TEST_F(WoleixStateMachineTest, TemperatureIncreaseInCoolMode)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        28.0f,  // +3 degrees
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 4);
    
    auto state = state_machine_->get_state();
    EXPECT_FLOAT_EQ(state.temperature, 28.0f);
}

/**
 * Test: Temperature decrease in COOL mode generates TEMP_DOWN commands
 * 
 * Validates that decreasing temperature (e.g., 25°C to 20°C) in COOL mode
 * generates the correct number of TEMP_DOWN commands (5 in this case) and
 * updates the internal temperature state.
 */
TEST_F(WoleixStateMachineTest, TemperatureDecreaseInCoolMode)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        20.0f,  // -5 degrees
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 6);
    
    auto state = state_machine_->get_state();
    EXPECT_FLOAT_EQ(state.temperature, 20.0f);
}

/**
 * Test: Temperature values below minimum are clamped to 15°C
 * 
 * When a temperature below the minimum (15°C) is requested, the state
 * machine should clamp it to 15°C and generate the appropriate number
 * of TEMP_DOWN commands to reach the minimum.
 */
TEST_F(WoleixStateMachineTest, TemperatureClampedToMinimum)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        10.0f,  // Below min (15°C)
        WoleixFanSpeed::LOW
    );
    
    // Should go from 25°C to 15°C (minimum)
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 11);
}

/**
 * Test: Temperature values above maximum are clamped to 30°C
 * 
 * When a temperature above the maximum (30°C) is requested, the state
 * machine should clamp it to 30°C and generate the appropriate number
 * of TEMP_UP commands to reach the maximum.
 */
TEST_F(WoleixStateMachineTest, TemperatureClampedToMaximum)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        35.0f,  // Above max (30°C)
        WoleixFanSpeed::LOW
    );
    
    // Should go from 25°C to 30°C (maximum)
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 6);
}

/**
 * Test: Temperature changes ignored in DEHUM mode
 * 
 * Temperature control is only available in COOL mode. When in DEHUM mode,
 * temperature change requests should be ignored and no temperature commands
 * should be generated.
 */
TEST_F(WoleixStateMachineTest, TemperatureIgnoredInDehumMode)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Now try to change temperature
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        30.0f,  // Temperature change should be ignored
        WoleixFanSpeed::LOW
    );
    
    // No temperature commands in DEHUM mode
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 0);
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 0);
}

/**
 * Test: Temperature changes ignored in FAN mode
 * 
 * Temperature control is only available in COOL mode. When in FAN mode,
 * temperature change requests should be ignored and no temperature commands
 * should be generated.
 */
TEST_F(WoleixStateMachineTest, TemperatureIgnoredInFanMode)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Now try to change temperature
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        20.0f,  // Temperature change should be ignored
        WoleixFanSpeed::LOW
    );
    
    // No temperature commands in FAN mode
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 0);
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 0);
}

// ============================================================================
// Test: Fan Speed Transitions
// ============================================================================

/**
 * Test: Fan speed LOW→HIGH transition sends SPEED command
 * 
 * Validates that changing fan speed from LOW to HIGH generates exactly
 * one SPEED IR command and updates the internal fan speed state.
 */
TEST_F(WoleixStateMachineTest, FanSpeedLowToHigh)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    EXPECT_EQ(count_command(commands, SPEED_COMMAND), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::HIGH);
}

/**
 * Test: Fan speed HIGH→LOW transition sends SPEED command
 * 
 * Validates that changing fan speed from HIGH to LOW generates exactly
 * one SPEED IR command (toggle) and updates the internal fan speed state.
 */
TEST_F(WoleixStateMachineTest, FanSpeedHighToLow)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Now go to LOW
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(count_command(commands, SPEED_COMMAND), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::LOW);
}

/**
 * Test: No fan speed change means no SPEED commands
 * 
 * When the target fan speed matches the current fan speed, no SPEED
 * commands should be generated.
 */
TEST_F(WoleixStateMachineTest, NoFanSpeedChangeGeneratesNoCommands)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    EXPECT_EQ(count_command(commands, SPEED_COMMAND), 0);
}

// ============================================================================
// Test: Complex Multi-State Transitions
// ============================================================================

/**
 * Test: Complete multi-parameter state change from defaults
 * 
 * Tests changing all parameters simultaneously (mode, temperature, fan speed).
 * Validates that temperature is correctly ignored when target mode doesn't
 * support temperature control (FAN mode in this case).
 */
TEST_F(WoleixStateMachineTest, CompleteStateChangeFromDefaults)
{
    state_machine_->reset();

    // Change everything from defaults
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,        // COOL -> FAN (2 steps)
        30.0f,                  // 25 -> 30 (5 steps, but ignored in FAN mode)
        WoleixFanSpeed::HIGH    // LOW -> HIGH (1 step)
    );
    
    // Should have: 2x MODE, 1x SPEED
    // Temperature is ignored because target mode is FAN
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 2);
    EXPECT_EQ(count_command(commands, SPEED_COMMAND), 1);
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 0);  // Ignored in FAN mode
}

/**
 * Test: Multiple sequential state changes
 * 
 * Validates that the state machine correctly handles a series of state
 * changes, with each change building on the previous state. Tests that
 * the command queue is properly cleared between changes.
 */
TEST_F(WoleixStateMachineTest, MultipleSequentialChanges)
{
    // Change 1: Mode and fan
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands1 = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Change 2: Mode and temperature
    auto commands2 = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        20.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Change 3: Power off
    auto commands3 = state_machine_->transit_to_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        20.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Verify each sequence
    EXPECT_EQ(count_command(commands1, MODE_COMMAND), 1);   // COOL->DEHUM
    EXPECT_EQ(count_command(commands1, SPEED_COMMAND), 1);  // LOW->HIGH
    
    EXPECT_EQ(count_command(commands2, MODE_COMMAND), 1);   // DEHUM->FAN->COOL
    EXPECT_EQ(count_command(commands2, TEMP_DOWN_COMMAND), 6); // 25->20
    
    EXPECT_EQ(count_command(commands3, POWER_COMMAND), 1);  // Turn off
}

/**
 * Test: Command ordering follows correct priority
 * 
 * When powering on and changing multiple parameters, commands should be
 * generated in the correct order: POWER first, then MODE, TEMP, and FAN.
 * This ensures the AC unit processes commands in the expected sequence.
 */
TEST_F(WoleixStateMachineTest, CommandOrderingIsCorrect)
{
    // Power on from off should process in order: POWER, MODE, TEMP, FAN
    state_machine_->set_current_state(
        WoleixPowerState::OFF,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        28.0f,
        WoleixFanSpeed::HIGH
    );
    
    EXPECT_EQ(commands.size(), 3);

    // First command should be POWER
    EXPECT_EQ(commands[0].get_type(), POWER_COMMAND);
    // Second command must be MODE (DEHUM->FAN)
    EXPECT_EQ(commands[1].get_type(), MODE_COMMAND);
    // Third command must be SPEED (LOW->HIGH)
    EXPECT_EQ(commands[2].get_type(), SPEED_COMMAND);
}

/**
 * Test: No commands generated when state doesn't change
 * 
 * Validates that setting the same target state twice in a row generates
 * commands only on the first call. The second call should return an empty
 * command queue since there's no state change.
 */
TEST_F(WoleixStateMachineTest, EmptyCommandsAfterNoChange)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // Set same state again
    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    // No state change, no commands
    EXPECT_EQ(commands.size(), 0);
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

/**
 * Test: Fractional temperatures are rounded correctly
 * 
 * Validates that fractional temperature values (e.g., 27.5°C) are properly
 * rounded to the nearest integer (28°C) when calculating the number of
 * temperature adjustment commands needed.
 */
TEST_F(WoleixStateMachineTest, TemperatureRoundingHandled)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );

    auto commands = state_machine_->transit_to_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        27.5f,  // Should round to 28
        WoleixFanSpeed::LOW
    );
    
    // 25 -> 28 = 3 steps (rounds up)
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 4);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
