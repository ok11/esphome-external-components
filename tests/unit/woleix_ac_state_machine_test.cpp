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
class WoleixACStateMachineTest : public ::testing::Test
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
    int count_command(const std::vector<std::string>& commands, const std::string& target) {
        return std::count(commands.begin(), commands.end(), target);
    }
};

// ============================================================================
// Test: Initialization and Default State
// ============================================================================

TEST_F(WoleixACStateMachineTest, InitialStateIsCorrect)
{
    auto state = state_machine_->get_state();
    
    EXPECT_EQ(state.power, WoleixPowerState::ON);
    EXPECT_EQ(state.mode, WoleixMode::COOL);
    EXPECT_FLOAT_EQ(state.temperature, 25.0f);
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::LOW);
}

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
    EXPECT_EQ(count_command(commands, POWER_PRONTO), 1);
    
    // Verify state updated
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::OFF);
}

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
    EXPECT_EQ(count_command(commands, POWER_PRONTO), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.power, WoleixPowerState::ON);
}

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
    EXPECT_EQ(count_command(commands, POWER_PRONTO), 1);
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 0);
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 0);
}

// ============================================================================
// Test: Mode Transitions (Circular)
// ============================================================================

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 1);
}

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 2);
}

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 1);
}

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 1);
}

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 2);
}

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
    
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 0);
}

// ============================================================================
// Test: Temperature Adjustments (COOL mode only)
// ============================================================================

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
    
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 3);
    
    auto state = state_machine_->get_state();
    EXPECT_FLOAT_EQ(state.temperature, 28.0f);
}

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
    
    EXPECT_EQ(count_command(commands, TEMP_DOWN_PRONTO), 5);
    
    auto state = state_machine_->get_state();
    EXPECT_FLOAT_EQ(state.temperature, 20.0f);
}

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
    EXPECT_EQ(count_command(commands, TEMP_DOWN_PRONTO), 10);
}

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
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 5);
}

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
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 0);
    EXPECT_EQ(count_command(commands, TEMP_DOWN_PRONTO), 0);
}

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
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 0);
    EXPECT_EQ(count_command(commands, TEMP_DOWN_PRONTO), 0);
}

// ============================================================================
// Test: Fan Speed Transitions
// ============================================================================

TEST_F(WoleixACStateMachineTest, FanSpeedLowToHigh)
{
    // Start at LOW (default)
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::HIGH);
}

TEST_F(WoleixACStateMachineTest, FanSpeedHighToLow)
{
    // First set to HIGH
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    state_machine_->get_commands(); // Clear commands
    
    // Now go to LOW
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 1);
    
    auto state = state_machine_->get_state();
    EXPECT_EQ(state.fan_speed, WoleixFanSpeed::LOW);
}

TEST_F(WoleixACStateMachineTest, NoFanSpeedChangeGeneratesNoCommands)
{
    // Stay at LOW
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::COOL,
        25.0f,
        WoleixFanSpeed::LOW
    );
    
    auto commands = state_machine_->get_commands();
    
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 0);
}

// ============================================================================
// Test: Complex Multi-State Transitions
// ============================================================================

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
    EXPECT_EQ(count_command(commands, MODE_PRONTO), 2);
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 1);
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 0);  // Ignored in FAN mode
}

TEST_F(WoleixACStateMachineTest, MultipleSequentialChanges)
{
    // Change 1: Mode and fan
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::DEHUM,
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
    EXPECT_EQ(count_command(commands1, MODE_PRONTO), 1);   // COOL->DEHUM
    EXPECT_EQ(count_command(commands1, SPEED_PRONTO), 1);  // LOW->HIGH
    
    EXPECT_EQ(count_command(commands2, MODE_PRONTO), 2);   // DEHUM->FAN->COOL
    EXPECT_EQ(count_command(commands2, TEMP_DOWN_PRONTO), 5); // 25->20
    
    EXPECT_EQ(count_command(commands3, POWER_PRONTO), 1);  // Turn off
}

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
        WoleixMode::COOL,
        28.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands = state_machine_->get_commands();
    
    // First command should be POWER
    EXPECT_EQ(commands[0], POWER_PRONTO);
    
    // Should contain all necessary commands
    EXPECT_GT(count_command(commands, MODE_PRONTO), 0);
    EXPECT_GT(count_command(commands, TEMP_UP_PRONTO), 0);
    EXPECT_EQ(count_command(commands, SPEED_PRONTO), 1);
}

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

TEST_F(WoleixACStateMachineTest, GetCommandsClearsQueue)
{
    state_machine_->set_target_state(
        WoleixPowerState::ON,
        WoleixMode::FAN,
        25.0f,
        WoleixFanSpeed::HIGH
    );
    
    auto commands1 = state_machine_->get_commands();
    auto commands2 = state_machine_->get_commands();
    
    EXPECT_GT(commands1.size(), 0);
    EXPECT_EQ(commands2.size(), 0);  // Queue should be empty
}

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
    EXPECT_EQ(count_command(commands, TEMP_UP_PRONTO), 3);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
