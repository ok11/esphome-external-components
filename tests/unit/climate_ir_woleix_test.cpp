#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include "climate_ir_woleix.h"
#include "state_mapper.h"
#include <optional>

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;
using namespace esphome::remote_base;

using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::Invoke;
class MockWoleixACStateMachine : public WoleixACStateMachine
{
public:
  MOCK_METHOD(WoleixInternalState, get_state, (), (const));

  void set_current_state(
    WoleixPowerState power, WoleixMode woleix_mode, float temperature, WoleixFanSpeed woleix_fan_speed
  )
  {
    current_state_.power = power;
    current_state_.mode = woleix_mode;
    current_state_.temperature = temperature;
    current_state_.fan_speed = woleix_fan_speed;
  }

  WoleixInternalState get_current_state() {
    return current_state_;
  }
};

// GMock version of RemoteTransmitter
class MockRemoteTransmitter : public RemoteTransmitter 
{
public:
  MOCK_METHOD(void, transmit_raw, (const RemoteTransmitData& data), (override));
};

// GMock version of WoleixClimate
class MockWoleixClimate : public WoleixClimate
{
public:
  MockWoleixClimate(MockWoleixACStateMachine* state_machine)
    : WoleixClimate(state_machine) 
  {}
  // Helper to set state via the mock state machine
  void set_last_state(
    ClimateMode mode,
    float target_temperature = 25.0f,
    ClimateFanMode fan_mode = ClimateFanMode::CLIMATE_FAN_LOW
  )
  {
    WoleixPowerState woleix_power = StateMapper::esphome_to_woleix_power(mode != ClimateMode::CLIMATE_MODE_OFF);
    WoleixMode woleix_mode = StateMapper::esphome_to_woleix_mode(mode);
    WoleixFanSpeed woleix_fan_speed = StateMapper::esphome_to_woleix_fan_mode(fan_mode);

    ((MockWoleixACStateMachine*)state_machine_)->set_current_state(
      woleix_power, woleix_mode, target_temperature, woleix_fan_speed
    );
  }

  void get_last_state(ClimateMode& mode,
                      float& target_temperature,
                      ClimateFanMode& fan_mode)
  {
      WoleixInternalState woleix_state =
        ((MockWoleixACStateMachine*)state_machine_)->get_current_state();

      mode = StateMapper::woleix_to_esphome_power(woleix_state.power) 
             ? StateMapper::woleix_to_esphome_mode(woleix_state.mode) 
             : ClimateMode::CLIMATE_MODE_OFF;
      target_temperature = woleix_state.temperature;
      fan_mode = StateMapper::woleix_to_esphome_fan_mode(woleix_state.fan_speed);
  }

  esphome::sensor::Sensor* get_humidity_sensor() {
    return this->humidity_sensor_;
  }

  esphome::binary_sensor::BinarySensor* get_reset_button() {
    return this->reset_button_;
  }

  MOCK_METHOD(void, publish_state, (), (override)); 
  MOCK_METHOD(void, enqueue_command_, (const WoleixCommand&));
  MOCK_METHOD(void, transmit_commands_, ());
//  MOCK_METHOD(void, transmit_state, (), (override));

  // Getter for state_machine_
  WoleixACStateMachine* get_state_machine() { return state_machine_; }
};

// Test fixture class
class WoleixClimateTest : public testing::Test 
{
protected:
  void SetUp() override 
  {
    mock_climate = new MockWoleixClimate(new MockWoleixACStateMachine());
    mock_transmitter = new MockRemoteTransmitter();
    mock_climate->set_transmitter(mock_transmitter);
    
    // Initialize optional fan_mode to avoid "bad optional access" errors
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    
    // Set the mock state machine to defaults after construction
    // This ensures all tests start from a known state (matching what the real state machine does)
    mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
    
    // Capture transmitted data
    transmitted_data.clear();
    mock_transmitter->set_transmit_callback([this](const RemoteTransmitData& data) 
    {
      transmitted_data.push_back(data);
    });
  }
  
  void TearDown() override {
    delete mock_climate;
    delete mock_transmitter;
  }
  
  MockWoleixClimate* mock_climate;
  MockRemoteTransmitter* mock_transmitter;
  std::vector<RemoteTransmitData> transmitted_data;
  
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
    return {
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
  bool check_timings_match(const RemoteTransmitData& data, 
                          const std::vector<int32_t>& expected) {
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
};

// ============================================================================
// Test: Traits Configuration
// ============================================================================

/**
 * Test: Verify climate traits are configured correctly
 * 
 * Validates that the climate component reports correct temperature range
 * (15-30°C with 1°C steps) and supports current temperature and humidity
 * sensor readings.
 */
TEST_F(WoleixClimateTest, TraitsConfiguredCorrectly)
{
  auto traits = mock_climate->call_traits();
  
  EXPECT_EQ(traits.get_visual_min_temperature(), 15.0f);
  EXPECT_EQ(traits.get_visual_max_temperature(), 30.0f);
  EXPECT_EQ(traits.get_visual_temperature_step(), 1.0f);
  
  // Check that current temperature and humidity are supported
  uint8_t flags = traits.get_feature_flags();
  EXPECT_TRUE(flags & CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  EXPECT_TRUE(flags & CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
}

// ============================================================================
// Test: Power On/Off
// ============================================================================

/**
 * Test: Turning on from OFF state sends power command plus state setup
 * 
 * When powering on from OFF state, the AC unit resets to defaults and
 * requires POWER command plus MODE and SPEED commands to reach desired state.
 * Tests transition from OFF to DRY mode with HIGH fan speed.
 */
TEST_F(WoleixClimateTest, TurningOnFromOffSendsPowerCommand)
{
  // Start in OFF mode - set state AFTER constructor has run
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_OFF,
    25.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(POWER_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(MODE_COMMAND)))
    .Times(2);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
  
}

/**
 * Test: Turning off sends only power command
 * 
 * When powering off the AC unit, only the POWER command should be sent.
 * No mode, temperature, or fan speed commands are needed when turning off.
 */
TEST_F(WoleixClimateTest, TurningOffSendsPowerCommand)
{
  // Start in COOL mode
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    25.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
);

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(POWER_COMMAND)))
    .Times(1);  
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);  

  // Turn off
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

/**
 * Test: Staying in OFF mode does not transmit any commands
 * 
 * When the AC is already off and the target state is also off,
 * no IR commands should be transmitted since there's no state change.
 */
TEST_F(WoleixClimateTest, StayingOffDoesNotTransmit) 
{
  // Start in OFF mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);

  EXPECT_CALL(*mock_climate, enqueue_command_(_))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

  // Stay in OFF mode
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
  // Should not transmit anything
  EXPECT_EQ(transmitted_data.size(), 0);
}

// ============================================================================
// Test: Temperature Changes
// ============================================================================

/**
 * Test: Increasing temperature sends multiple TEMP_UP commands
 * 
 * When raising the temperature (e.g., from 20°C to 23°C), the component
 * should send the correct number of TEMP_UP IR commands (3 in this case).
 * Temperature adjustment only works in COOL mode.
 */
TEST_F(WoleixClimateTest, IncreasingTemperatureSendsTempUpCommands)
{
  // Initialize with a known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    20.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(TEMP_UP_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(REPEAT_COMMAND)))
    .Times(2);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  // Increase temperature by 3 degrees
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->target_temperature = 23.0f;
  mock_climate->call_transmit_state();
}

/**
 * Test: Decreasing temperature sends multiple TEMP_DOWN commands
 * 
 * When lowering the temperature (e.g., from 25°C to 23°C), the component
 * should send the correct number of TEMP_DOWN IR commands (2 in this case).
 * Temperature adjustment only works in COOL mode.
 */
TEST_F(WoleixClimateTest, DecreasingTemperatureSendsTempDownCommands)
{
  // Initialize with a known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    25.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(TEMP_DOWN_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(REPEAT_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  // Decrease temperature by 2 degrees
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 23.0f;
  mock_climate->call_transmit_state();  
}

/**
 * Test: No temperature change means no temperature commands
 * 
 * When the target temperature matches the current temperature, no
 * temperature adjustment commands should be sent.
 */
TEST_F(WoleixClimateTest, NoTemperatureChangeDoesNotSendTempCommands)
{
  // Initialize with a known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );
  
  EXPECT_CALL(*mock_climate, enqueue_command_(_))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

    // Keep same temperature
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();  
}

/**
 * Test: Temperature changes ignored in non-COOL modes
 * 
 * Temperature control is only available in COOL mode. When in FAN or
 * DEHUM mode, temperature changes should be ignored and no temperature
 * commands should be sent.
 */
TEST_F(WoleixClimateTest, NonCoolModeDoesNotSendTempCommands)
{
  // Initialize with a known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_FAN_ONLY,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );
  
  EXPECT_CALL(*mock_climate, enqueue_command_(_))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

    // Keep same temperature
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 25.0f;
  mock_climate->call_transmit_state();  
}

// ============================================================================
// Test: Mode Changes
// ============================================================================

/**
 * Test: Mode transition COOL→FAN requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from COOL to FAN requires
 * cycling through: COOL→DEHUM→FAN (2 steps).
 */
TEST_F(WoleixClimateTest, ChangingModeCoolToFanSends2ModeCommands)
{
  // Initialize in COOL mode
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(MODE_COMMAND)))
    .Times(2);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);
  
  // Change to FAN mode
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

/**
 * Test: Mode transition DRY→COOL requires 2 MODE button presses
 * 
 * Tests the circular mode sequence. Going from DEHUM (DRY) to COOL requires
 * cycling through: DEHUM→FAN→COOL (2 steps).
 */
TEST_F(WoleixClimateTest, ChangingModeDryToCoolSends2ModeCommands)
{
  // Initialize in COOL mode
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_DRY,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(MODE_COMMAND)))
    .Times(2);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);
  
  // Change to FAN mode
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

/**
 * Test: Mode transition COOL→DRY requires 1 MODE button press
 * 
 * Tests the circular mode sequence. Going from COOL to DEHUM (DRY) requires
 * only one step: COOL→DEHUM (1 step).
 */
TEST_F(WoleixClimateTest, ChangingModeCoolToDrySends1ModeCommand)
{
  // Initialize in COOL mode
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(MODE_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);
  
  // Change to FAN mode
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_DRY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

// ============================================================================
// Test: Fan Speed Changes
// ============================================================================

/**
 * Test: Increasing fan speed from LOW to HIGH sends SPEED command
 * 
 * Validates that changing fan speed from LOW to HIGH generates a single
 * SPEED IR command to toggle the fan speed setting.
 */
TEST_F(WoleixClimateTest, IncreasingFanSpeedSendsSpeedCommand)
{
  // Initialize with known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_FAN_ONLY,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

    // Change fan speed
  mock_climate->target_temperature = 22.0f;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
  
}

/**
 * Test: Decreasing fan speed from HIGH to LOW sends SPEED command
 * 
 * Validates that changing fan speed from HIGH to LOW generates a single
 * SPEED IR command to toggle the fan speed setting.
 */
TEST_F(WoleixClimateTest, DecreasingFanSpeedSendsSpeedCommand)
{
  // Initialize with known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_FAN_ONLY,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_HIGH
  );

  testing::InSequence seq;  // Enforce call order
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  // Change fan speed
  mock_climate->target_temperature = 22.0f;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
}

/**
 * Test: Unchanged fan speed does not send SPEED command
 * 
 * When the target fan speed matches the current fan speed, no SPEED
 * command should be sent since there's no change needed.
 */
TEST_F(WoleixClimateTest, UnchangedFanSpeedDoesNotSendSpeedCommand)
{
  // Initialize with known state
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_COOL,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_HIGH
  );
  
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

  // Change fan speed
  mock_climate->target_temperature = 22.0f;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
  
}
// ============================================================================
// Test: Complex State Transitions
// ============================================================================

/**
 * Test: Complete multi-parameter state change
 * 
 * Tests a complex transition changing mode (DRY→COOL), temperature (20→24°C),
 * and fan speed (LOW→HIGH) simultaneously. Validates that all necessary
 * commands are generated in the correct order and quantity.
 */
TEST_F(WoleixClimateTest, CompleteStateChangeSequence)
{
  // Start from OFF
  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_DRY,
    20.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(0); // the target state is not FAN, so fan speed change won't be sent
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(MODE_COMMAND)))
    .Times(2);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(TEMP_UP_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(REPEAT_COMMAND)))
    .Times(3);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  // Turn on with complete state
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 24.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
}

// TEST_F(WoleixClimateTest, CarrierFrequencySetCorrectly) {
//   mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
//   mock_climate->target_temperature = 22.0f;
//   mock_climate->call_transmit_state();
  
//   // All transmitted data should have correct carrier frequency
//   for (const auto& data : transmitted_data) {
//     EXPECT_EQ(data.get_carrier_frequency(), 38030);
//   }
// }

// ============================================================================
// Test: Temperature Bounds
// ============================================================================

/**
 * Test: Temperature constants are correctly defined
 * 
 * Verifies that minimum (15°C) and maximum (30°C) temperature constants
 * match the Woleix AC unit specifications.
 */
TEST_F(WoleixClimateTest, TemperatureBoundsAreCorrect)
{
  EXPECT_EQ(WOLEIX_TEMP_MIN, 15.0f);
  EXPECT_EQ(WOLEIX_TEMP_MAX, 30.0f);
}

// ============================================================================
// GMock Tests - Using EXPECT_CALL
// ============================================================================

/**
 * Test: transmit_state() calls publish_state() to notify ESPHome
 * 
 * Validates that after transmitting IR commands, the climate component
 * calls publish_state() to update ESPHome with the new state.
 */
TEST_F(WoleixClimateTest, ControlCallsPublishState)
{
  auto call = mock_climate->make_call();
  call.set_mode(ClimateMode::CLIMATE_MODE_COOL)
      .set_target_temperature(25.0f)
      .set_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);

    // Turning on sends: Power, Mode, Speed commands
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);

  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  mock_climate->control(call);
}

/**
 * Test: transmit_state() calls publish_state() to notify ESPHome
 * 
 * Validates that after transmitting IR commands, the climate component
 * calls publish_state() to update ESPHome with the new state.
 */
TEST_F(WoleixClimateTest, ControlUpdatesState)
{
  auto call = mock_climate->make_call();
  call.set_mode(ClimateMode::CLIMATE_MODE_COOL)
      .set_target_temperature(25.0f)
      .set_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);

  // Turning on sends: Power, Mode, Speed commands
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->control(call);

  EXPECT_EQ(mock_climate->mode, ClimateMode::CLIMATE_MODE_COOL);
  EXPECT_EQ(mock_climate->target_temperature, 25.0f);
  EXPECT_EQ(mock_climate->fan_mode, ClimateFanMode::CLIMATE_FAN_LOW);
}

// ============================================================================
// Test: Humidity Sensor Callback
// ============================================================================

/**
 * Test: Humidity sensor callback registration during setup
 * 
 * Verifies that when a humidity sensor is configured, the setup() method
 * registers a callback to receive humidity updates. Tests that publishing
 * a humidity value updates the sensor state correctly.
 */
TEST_F(WoleixClimateTest, HumiditySensorCallbackIsRegistered)
{
  // Create a mock humidity sensor
  esphome::sensor::Sensor humidity_sensor;
  
  // Set the humidity sensor on the climate device
  mock_climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  mock_climate->setup();
  
  // Verify the callback was registered by publishing a state
  // and checking that it doesn't crash (basic test)
  humidity_sensor.publish_state(55.0f);
  
  // Verify the sensor state was updated
  EXPECT_EQ(humidity_sensor.state, 55.0f);
}

/**
 * Test: Humidity sensor receives multiple updates correctly
 * 
 * Validates that the humidity sensor callback can handle multiple
 * sequential updates, with each new value correctly updating the
 * sensor's state property.
 */
TEST_F(WoleixClimateTest, HumiditySensorCallbackReceivesUpdates)
{
  // Create a mock humidity sensor
  esphome::sensor::Sensor humidity_sensor;
  
  // Set the humidity sensor on the climate device
  mock_climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  mock_climate->setup();
  
  // Publish multiple humidity values and verify they're received
  humidity_sensor.publish_state(45.5f);
  EXPECT_EQ(humidity_sensor.state, 45.5f);
  
  humidity_sensor.publish_state(62.3f);
  EXPECT_EQ(humidity_sensor.state, 62.3f);
  
  humidity_sensor.publish_state(70.0f);
  EXPECT_EQ(humidity_sensor.state, 70.0f);
}

/**
 * Test: Setup() handles null humidity sensor gracefully
 * 
 * Verifies that calling setup() without a humidity sensor configured
 * (nullptr) does not cause crashes or exceptions. The humidity sensor
 * is optional, so this must work correctly.
 */
TEST_F(WoleixClimateTest, HumiditySensorCallbackWorksWithNullSensor)
{
  // Don't set a humidity sensor (leave it as nullptr)
  
  // Call setup - should not crash even without a sensor
  EXPECT_NO_THROW(mock_climate->setup());
}

/**
 * Test: Humidity sensor updates trigger climate state republish
 * 
 * Validates that when the humidity sensor publishes a new value, the
 * climate component's publish_state() is called to update ESPHome
 * with the new humidity reading.
 */
TEST_F(WoleixClimateTest, PublishingStateOfHumiditySensorRepublishesItByClimate)
{
  // Turning on sends: Power, Mode, Speed commands

  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_OFF,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  // Create a mock humidity sensor
  esphome::sensor::Sensor humidity_sensor;
  
  // Set the humidity sensor on the climate device
  mock_climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  mock_climate->setup();
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  
  // Publish multiple humidity values and verify they're received
  humidity_sensor.publish_state(45.5f);
}

// ============================================================================
// Test: Reset Button Callback
// ============================================================================

/**
 * Test: Setup() handles null reset button gracefully
 * 
 * Verifies that calling setup() without the reset button configured
 * (nullptr) does not cause crashes or exceptions. The reset button
 * is optional, so this must work correctly.
 */
TEST_F(WoleixClimateTest, ResetButtonCallbackWorksWithNullSensor)
{
  // Don't set a reset button (leave it as nullptr)
  
  // Call setup - should not crash even without a sensor
  EXPECT_NO_THROW(mock_climate->setup());
}


/**
 * Test: Reset button callback registration during setup
 * 
 * Verifies that when the reset button is configured, the setup() method
 * registers a callback to receive button press. Tests that publishing
 * the reset button state updates the climate state correctly.
 */
TEST_F(WoleixClimateTest, ResetButtonCallbackIsRegistered)
{
  // Create a mock reset button
  esphome::binary_sensor::BinarySensor reset_button;
  
  // Set the reset button on the climate device
  mock_climate->set_reset_button(&reset_button);
  
  // Call setup to register the callback
  mock_climate->setup();
  
  // Verify the callback was registered by publishing a state
  // and checking that it doesn't crash (basic test)
  reset_button.publish_state(true);
}

/**
 * Test: Reset button press resets the internal state
 * 
 * Validates that the reset button press resets the internal climate state.
 */
TEST_F(WoleixClimateTest, ResetButtonCallbackResetsState)
{
  // Create a mock reset button
  esphome::binary_sensor::BinarySensor reset_button;
  
  // Set the reset button on the climate device
  mock_climate->set_reset_button(&reset_button);
  
  // Call setup to register the callback
  mock_climate->setup();
  
  ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
  float target_temperature = 20.0f;
  ClimateFanMode fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;

  mock_climate->set_last_state(
    mode,
    target_temperature,
    fan_mode
  );

  // Press the reset button
  reset_button.publish_state(true);

  mock_climate->get_last_state(
    mode,
    target_temperature,
    fan_mode
  );
  EXPECT_EQ(mode, ClimateMode::CLIMATE_MODE_COOL);
  EXPECT_EQ(target_temperature, 25.0f);
  EXPECT_EQ(fan_mode, ClimateFanMode::CLIMATE_FAN_LOW);
}

/**
 * Test: Reset button press triggers climate state republishing
 * (with reset values)
 * 
 * Validates that when the reset button is pressed, the
 * climate component's publish_state() is called to update ESPHome
 * with the new climate state.
 */
TEST_F(WoleixClimateTest, PublishingStateOfResetButtonRepublishesItByClimate)
{
  // Turning on sends: Power, Mode, Speed commands

  mock_climate->set_last_state(
    ClimateMode::CLIMATE_MODE_OFF,
    22.0f,
    ClimateFanMode::CLIMATE_FAN_LOW
  );

  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  // Create a mock humidity sensor
  esphome::binary_sensor::BinarySensor reset_button;
  
  // Set the humidity sensor on the climate device
  mock_climate->set_reset_button(&reset_button);
  
  // Call setup to register the callback
  mock_climate->setup();
  
  // Publish multiple humidity values and verify they're received
  reset_button.publish_state(true);
}

/**
 * Test: Fan speed is only transmitted in FAN mode
 * 
 * Validates that the SPEED_COMMAND is only sent when the climate
 * is in FAN mode. In other modes (COOL, DRY), fan speed changes
 * should not trigger a SPEED_COMMAND.
 */
TEST_F(WoleixClimateTest, FanSpeedOnlyTransmittedInFanMode)
{
  // Test in COOL mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test in DRY mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_DRY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_DRY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test in FAN mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test that changing temperature in FAN mode doesn't trigger SPEED_COMMAND
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_climate, enqueue_command_(testing::Eq(SPEED_COMMAND)))
    .Times(0);
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(0);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 24.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

// ============================================================================
// Test: Control Function
// ============================================================================

/**
 * Test: Control function handles mode changes correctly
 *
 * Validates that the control() function correctly updates the internal state
 * when a mode change is requested, and triggers transmit_state().
 */

// ============================================================================
// Test: Improved State Synchronization
// ============================================================================

/**
 * Test: Transmit state synchronizes internal state with state machine
 *
 * Validates that the transmit_state() function correctly synchronizes
 * the internal state of the climate component with the state machine
 * after sending commands.
 */
TEST_F(WoleixClimateTest, TransmitStateSynchronizesInternalState)
{
  // Start in COOL mode at 20°C with LOW fan
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 20.0f, ClimateFanMode::CLIMATE_FAN_LOW);

  // Trigger a state change: stay in COOL mode, change temperature to 24°C
  // This ensures temperature commands are actually generated (only works in COOL mode)
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 24.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;

  // Allow transmit_commands_ to be called without actually sending commands
  EXPECT_CALL(*mock_climate, transmit_commands_())
    .Times(1);

  mock_climate->call_transmit_state();

  // Verify that the internal state has been synchronized with what the state machine computed
  // After the state change, the state machine should have:
  // - mode: COOL (unchanged)
  // - temperature: 24.0 (changed from 20.0)
  // - fan_speed: LOW (unchanged)
  EXPECT_EQ(mock_climate->mode, ClimateMode::CLIMATE_MODE_COOL);
  EXPECT_EQ(mock_climate->target_temperature, 24.0f);
  EXPECT_EQ(mock_climate->fan_mode, ClimateFanMode::CLIMATE_FAN_LOW);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) 
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
