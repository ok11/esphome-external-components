#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "climate_ir_woleix.h"

#include "woleix_ac_state_machine.h"

using namespace esphome::climate_ir_woleix;

class MockWoleixACStateMachine : public WoleixACStateMachine {
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
};

// Test fixture for WoleixACStateMachine
class WoleixACStateMachineTest : public testing::Test
{
protected:
    void SetUp() override
    {
        state_machine_ = new MockWoleixACStateMachine();
    }
    
    void TearDown() override
    {
        delete state_machine_;
    }
    
    MockWoleixACStateMachine* state_machine_;
    
    // Helper to count specific commands in a vector
    int count_command(const std::vector<WoleixCommand>& commands, WoleixCommand target) {
        return std::count(commands.begin(), commands.end(), target);
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
TEST_F(WoleixACStateMachineTest, InitialStateIsCorrect)
{
    auto state = state_machine_->get_state();
    
    EXPECT_EQ(state.power, WoleixPowerState::ON);
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
TEST_F(WoleixACStateMachineTest, ResetRestoresDefaultState)
{
    // Change state
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        20.0f,
        WoleixFanSpeed::HIGH
    );
    state_machine_->get_commands(); // Clear commands
    
    // Reset
    state_machine_->reset();
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::ON);
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
TEST_F(WoleixACStateMachineTest, PowerOffFromOnSendsPowerCommand)
{
    // Start from ON (default state)
    state_machine_->set_target_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, PowerOnFromOffSendsPowerCommand)
{
    // First turn off
    state_machine_->set_target_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now turn back on
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,  // Request FAN mode
        20.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, PowerOffIgnoresOtherStateChanges)
{
    state_machine_->set_target_state(
        WoleixPowerState::OFF,
        WoleixMode::FAN,     // These should be ignored
        30.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, ModeTransitionCoolToDehum)
{
    // Start in COOL (default)
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // COOL -> DEHUM = 1 step
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: COOL→FAN mode transition requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from COOL to FAN requires
 * passing through DEHUM: COOL→DEHUM→FAN (2 steps).
 */
TEST_F(WoleixACStateMachineTest, ModeTransitionCoolToFan)
{
    // Start in COOL (default)
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // COOL -> DEHUM -> FAN = 2 steps
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 2);
}

/**
 * Test: DEHUM→FAN mode transition requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from DEHUM to FAN is the
 * next step in the cycle, requiring exactly 1 MODE command.
 */
TEST_F(WoleixACStateMachineTest, ModeTransitionDehumToFan)
{
    // First go to DEHUM
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now go to FAN
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // DEHUM -> FAN = 1 step
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: FAN→COOL mode transition requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from FAN to COOL wraps around
 * the cycle, requiring exactly 1 MODE command: FAN→COOL.
 */
TEST_F(WoleixACStateMachineTest, ModeTransitionFanToCool)
{
    // First go to FAN
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now go to COOL
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // FAN -> COOL = 1 step (wraps around)
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 1);
}

/**
 * Test: DEHUM→COOL mode transition requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from DEHUM to COOL requires
 * passing through FAN: DEHUM→FAN→COOL (2 steps).
 */
TEST_F(WoleixACStateMachineTest, ModeTransitionDehumToCool)
{
    // First go to DEHUM
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now go to COOL
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // DEHUM -> FAN -> COOL = 2 steps
    EXPECT_EQ(count_command(commands, MODE_COMMAND), 2);
}

/**
 * Test: No mode change means no MODE commands
 * 
 * When the target mode matches the current mode, no MODE commands
 * should be generated.
 */
TEST_F(WoleixACStateMachineTest, NoModeChangeGeneratesNoModeCommands)
{
    // Stay in COOL mode
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, TemperatureIncreaseInCoolMode)
{
    // Start in COOL at 25°C (default)
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        28.0f,  // +3 degrees
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 3);
    
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
TEST_F(WoleixACStateMachineTest, TemperatureDecreaseInCoolMode)
{
    // Start in COOL at 25°C (default)
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        20.0f,  // -5 degrees
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 5);
    
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
TEST_F(WoleixACStateMachineTest, TemperatureClampedToMinimum)
{
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        10.0f,  // Below min (15°C)
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // Should go from 25°C to 15°C (minimum)
    EXPECT_EQ(count_command(commands, TEMP_DOWN_COMMAND), 10);
}

/**
 * Test: Temperature values above maximum are clamped to 30°C
 * 
 * When a temperature above the maximum (30°C) is requested, the state
 * machine should clamp it to 30°C and generate the appropriate number
 * of TEMP_UP commands to reach the maximum.
 */
TEST_F(WoleixACStateMachineTest, TemperatureClampedToMaximum)
{
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        35.0f,  // Above max (30°C)
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // Should go from 25°C to 30°C (maximum)
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 5);
}

/**
 * Test: Temperature changes ignored in DEHUM mode
 * 
 * Temperature control is only available in COOL mode. When in DEHUM mode,
 * temperature change requests should be ignored and no temperature commands
 * should be generated.
 */
TEST_F(WoleixACStateMachineTest, TemperatureIgnoredInDehumMode)
{
    // First switch to DEHUM mode
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now try to change temperature
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        30.0f,  // Temperature change should be ignored
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, TemperatureIgnoredInFanMode)
{
    // First switch to FAN mode
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now try to change temperature
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        20.0f,  // Temperature change should be ignored
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, FanSpeedLowToHigh)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, FanSpeedHighToLow)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    // Now go to LOW
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, NoFanSpeedChangeGeneratesNoCommands)
{
    // Stay at LOW
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, CompleteStateChangeFromDefaults)
{
    // Change everything from defaults
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,        // COOL -> FAN (2 steps)
        30.0f,                  // 25 -> 30 (5 steps, but ignored in FAN mode)
        WoleixFanSpeed::HIGH    // LOW -> HIGH (1 step)
    );
    
    auto commands = state_machine_->get_commands();
    
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
TEST_F(WoleixACStateMachineTest, MultipleSequentialChanges)
{
    // Change 1: Mode and fan
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );

    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    auto commands1 = state_machine_->get_commands();
    
    // Change 2: Mode and temperature
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        20.0f,
        WoleixFanSpeed::HIGH
    );
    auto commands2 = state_machine_->get_commands();
    
    // Change 3: Power off
    state_machine_->set_target_state(
        WoleixPowerState::OFF,
        WoleixMode::COOL,
        20.0f,
        WoleixFanSpeed::HIGH
    );
    auto commands3 = state_machine_->get_commands();
    
    // Verify each sequence
    EXPECT_EQ(count_command(commands1, MODE_COMMAND), 1);   // COOL->DEHUM
    EXPECT_EQ(count_command(commands1, SPEED_COMMAND), 1);  // LOW->HIGH
    
    EXPECT_EQ(count_command(commands2, MODE_COMMAND), 1);   // DEHUM->FAN->COOL
    EXPECT_EQ(count_command(commands2, TEMP_DOWN_COMMAND), 5); // 25->20
    
    EXPECT_EQ(count_command(commands3, POWER_COMMAND), 1);  // Turn off
}

/**
 * Test: Command ordering follows correct priority
 * 
 * When powering on and changing multiple parameters, commands should be
 * generated in the correct order: POWER first, then MODE, TEMP, and FAN.
 * This ensures the AC unit processes commands in the expected sequence.
 */
TEST_F(WoleixACStateMachineTest, CommandOrderingIsCorrect)
{
    // Power on from off should process in order: POWER, MODE, TEMP, FAN
    state_machine_->set_current_state(
        WoleixPowerState::OFF,
        WoleixMode::DEHUM,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands(); // Turn off
    
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        28.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(commands.size(), 3);

    // First command should be POWER
    EXPECT_EQ(commands[0], POWER_COMMAND);
    // Second command must be MODE (DEHUM->FAN)
    EXPECT_EQ(commands[1], MODE_COMMAND);
    // Third command must be SPEED (LOW->HIGH)
    EXPECT_EQ(commands[2], SPEED_COMMAND);
}

/**
 * Test: No commands generated when state doesn't change
 * 
 * Validates that setting the same target state twice in a row generates
 * commands only on the first call. The second call should return an empty
 * command queue since there's no state change.
 */
TEST_F(WoleixACStateMachineTest, EmptyCommandsAfterNoChange)
{
    // Set a state
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    state_machine_->get_commands();
    
    // Set same state again
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // No state change, no commands
    EXPECT_EQ(commands.size(), 0);
}

// ============================================================================
// Test: Edge Cases
// ============================================================================

/**
 * Test: get_commands() clears the command queue
 * 
 * Validates that calling get_commands() is side-effect free,
 * in this case returns the queued commands until the next target 
 * state is set. Subsequent calls should return the same vector
 * until new state changes generate other commands.
 */
TEST_F(WoleixACStateMachineTest, GetCommandsClearsQueue)
{
    state_machine_->set_current_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::HIGH
    );

    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands1 = state_machine_->get_commands();
    auto commands2 = state_machine_->get_commands();
    
    EXPECT_GT(commands1.size(), 0);
    EXPECT_GT(commands2.size(), 0);  // Queue should be not empty
    EXPECT_EQ(commands1, commands2); // Both calls return the same commands
}

/**
 * Test: Fractional temperatures are rounded correctly
 * 
 * Validates that fractional temperature values (e.g., 27.5°C) are properly
 * rounded to the nearest integer (28°C) when calculating the number of
 * temperature adjustment commands needed.
 */
TEST_F(WoleixACStateMachineTest, TemperatureRoundingHandled)
{
    // Test fractional temperatures
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        27.5f,  // Should round to 28
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    // 25 -> 28 = 3 steps (rounds up)
    EXPECT_EQ(count_command(commands, TEMP_UP_COMMAND), 3);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
