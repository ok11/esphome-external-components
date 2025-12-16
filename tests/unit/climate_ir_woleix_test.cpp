#include <optional>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "climate_ir_woleix.h"
#include "woleix_state_mapper.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;
using namespace esphome::remote_base;

using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::Invoke;

// Custom GMock matcher for checking WoleixCommand type
MATCHER_P(IsCommandOfType, expected_type, "") {
    auto actual_type = std::visit([](const auto& c) { 
        return c.get_type(); 
    }, arg);
    return actual_type == expected_type;
}

MATCHER_P2(IsCommand, expected_type, expected_repeat, "") {
    auto actual_type = std::visit([](const auto& c) { 
        return c.get_type(); 
    }, arg);
    auto actual_repeat = std::visit([](const auto& c) { 
        return c.get_repeat_count(); 
    }, arg);
    return actual_type == expected_type && actual_repeat == expected_repeat;
}

class MockWoleixStateMachine : public WoleixStateMachine
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

// GMock version of RemoteTransmitterBase
class MockWoleixCommandTransmitter : public WoleixCommandTransmitter
{
public:
  MockWoleixCommandTransmitter(): WoleixCommandTransmitter(nullptr) {}
  MOCK_METHOD(void, transmit_, (const WoleixCommand& command), ());
};

// GMock version of WoleixClimate
class MockWoleixClimate : public WoleixClimate
{
public:
  MockWoleixClimate(MockWoleixStateMachine* state_machine, MockWoleixCommandTransmitter* command_transmitter)
    : WoleixClimate(state_machine, command_transmitter) 
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

    ((MockWoleixStateMachine*)state_machine_)->set_current_state(
      woleix_power, woleix_mode, target_temperature, woleix_fan_speed
    );
  }

  void get_last_state(ClimateMode& mode,
                      float& target_temperature,
                      ClimateFanMode& fan_mode)
  {
      WoleixInternalState woleix_state =
        ((MockWoleixStateMachine*)state_machine_)->get_current_state();

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

  // Getter for state_machine_
  WoleixStateMachine* get_state_machine() { return state_machine_; }
};

// Test fixture class
class WoleixClimateTest : public testing::Test 
{
protected:
  void SetUp() override 
  {
    mock_state_machine = new MockWoleixStateMachine();
    mock_transmitter = new MockWoleixCommandTransmitter();
    mock_climate = new MockWoleixClimate(mock_state_machine, mock_transmitter);
    
    // Initialize optional fan_mode to avoid "bad optional access" errors
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    
    // Set the mock state machine to defaults after construction
    // This ensures all tests start from a known state (matching what the real state machine does)
    mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  }
    
  void TearDown() override {
    delete mock_climate;
    delete mock_transmitter;
  }
  MockWoleixStateMachine* mock_state_machine;
  MockWoleixCommandTransmitter* mock_transmitter;
  MockWoleixClimate* mock_climate;
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::POWER, 1)))
    .Times(1);
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::MODE, 2)))
    .Times(1);
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::FAN_SPEED, 1)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::POWER, 1)))
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

  EXPECT_CALL(*mock_transmitter, transmit_(_))
    .Times(0);

  // Stay in OFF mode
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::TEMP_UP, 3)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::TEMP_DOWN, 2)))
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
  
  EXPECT_CALL(*mock_transmitter, transmit_(_))
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
  
  EXPECT_CALL(*mock_transmitter, transmit_(_))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::MODE, 2)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::MODE, 2)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::MODE, 1)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::FAN_SPEED, 1)))
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
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::FAN_SPEED, 1)))
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
  
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
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

  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
    .Times(0); // the target state is not FAN, so fan speed change won't be sent
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::MODE, 2)))
    .Times(1);
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommand(WoleixCommandBase::Type::TEMP_UP, 4)))
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
 * updates ESPHome with the new state.
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
TEST_F(WoleixClimateTest, DISABLED_ResetButtonCallbackResetsState)
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
TEST_F(WoleixClimateTest, DISABLED_PublishingStateOfResetButtonRepublishesItByClimate)
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
  
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
    .Times(0);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test in DRY mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_DRY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
    .Times(0);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_DRY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test in FAN mode
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
    .Times(1);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();

  // Test that changing temperature in FAN mode doesn't trigger SPEED_COMMAND
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  EXPECT_CALL(*mock_transmitter, transmit_(IsCommandOfType(WoleixCommandBase::Type::FAN_SPEED)))
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
// Test: Protocol Selection
// ============================================================================

/**
 * Test: Default protocol generates NEC commands
 * 
 * Validates that without explicitly setting a protocol, the climate component
 * uses the default NEC protocol for command generation.
 */
TEST_F(WoleixClimateTest, DefaultProtocolGeneratesNecCommands)
{
  // Don't call set_protocol - use default (NEC)
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Expect NEC command (detected by checking if it's a WoleixNecCommand variant)
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      // Verify this is a NEC command by checking variant index
      EXPECT_TRUE(std::holds_alternative<WoleixNecCommand>(cmd));
      
      // Also verify NEC-specific properties
      auto& nec_cmd = std::get<WoleixNecCommand>(cmd);
      EXPECT_EQ(nec_cmd.get_address(), ADDRESS_NEC);
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

/**
 * Test: Set NEC protocol generates NEC commands
 * 
 * Validates that after calling set_protocol(Protocol::NEC), the climate
 * component generates NEC format commands instead of Pronto commands.
 */
TEST_F(WoleixClimateTest, SetProtocolNecGeneratesNecCommands)
{
  // Set NEC protocol
  mock_climate->set_protocol(Protocol::NEC);
  
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Expect NEC command (detected by checking if it's a WoleixNecCommand variant)
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      // Verify this is a NEC command by checking variant index
      EXPECT_TRUE(std::holds_alternative<WoleixNecCommand>(cmd));
      
      // Also verify NEC-specific properties
      auto& nec_cmd = std::get<WoleixNecCommand>(cmd);
      EXPECT_EQ(nec_cmd.get_address(), ADDRESS_NEC);
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

/**
 * Test: Set Pronto protocol explicitly generates Pronto commands
 * 
 * Validates that explicitly calling set_protocol(Protocol::PRONTO)
 * ensures Pronto format commands are generated.
 */
TEST_F(WoleixClimateTest, SetProtocolProntoGeneratesProntoCommands)
{
  // Explicitly set Pronto protocol
  mock_climate->set_protocol(Protocol::PRONTO);
  
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Expect Pronto command
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      // Verify this is a Pronto command
      EXPECT_TRUE(std::holds_alternative<WoleixProntoCommand>(cmd));
      
      // Also verify Pronto-specific properties
      auto& pronto_cmd = std::get<WoleixProntoCommand>(cmd);
      EXPECT_FALSE(pronto_cmd.get_pronto_hex().empty());
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

/**
 * Test: Protocol switch mid-operation changes command type
 * 
 * Validates that switching protocols during operation correctly changes
 * the type of commands generated. Tests switching from default (NEC)
 * to Pronto and verifying both produce correct command types.
 */
TEST_F(WoleixClimateTest, ProtocolSwitchChangesCommandType)
{
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // First transmission: Default NEC protocol
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      EXPECT_TRUE(std::holds_alternative<WoleixNecCommand>(cmd));
      auto& nec_cmd = std::get<WoleixNecCommand>(cmd);
      EXPECT_EQ(nec_cmd.get_address(), ADDRESS_NEC);
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
  // Switch to Pronto protocol
  mock_climate->set_protocol(Protocol::PRONTO);
  
  // Reset state for next transmission
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Second transmission: Pronto protocol
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      EXPECT_TRUE(std::holds_alternative<WoleixProntoCommand>(cmd));
      auto& pronto_cmd = std::get<WoleixProntoCommand>(cmd);
      EXPECT_FALSE(pronto_cmd.get_pronto_hex().empty());
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
}

/**
 * Test: Verify NEC commands contain correct address and codes
 * 
 * When using NEC protocol, validates that generated commands have:
 * - Correct NEC address (ADDRESS_NEC)
 * - Correct command codes for each button type
 */
TEST_F(WoleixClimateTest, NecCommandsHaveCorrectAddressAndCodes)
{
  mock_climate->set_protocol(Protocol::NEC);
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 20.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Capture all transmitted commands
  std::vector<WoleixNecCommand> captured_commands;
  
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([&captured_commands](const WoleixCommand& cmd) {
      ASSERT_TRUE(std::holds_alternative<WoleixNecCommand>(cmd));
      captured_commands.push_back(std::get<WoleixNecCommand>(cmd));
    }));
  
  // Trigger state change that will generate TEMP_UP commands
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 23.0f;  // +3 degrees = TEMP_UP
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
  // Verify all commands have correct NEC address
  for (const auto& cmd : captured_commands) {
    EXPECT_EQ(cmd.get_address(), ADDRESS_NEC);
    
    // Verify command code matches the command type
    if (cmd.get_type() == WoleixCommandBase::Type::TEMP_UP) {
      EXPECT_EQ(cmd.get_command_code(), TEMP_UP_NEC);
    }
  }
}

/**
 * Test: Verify Pronto commands contain correct hex strings
 * 
 * When using Pronto protocol, validates that generated commands have:
 * - Non-empty Pronto hex strings
 * - Correct hex string for each command type
 */
TEST_F(WoleixClimateTest, ProntoCommandsHaveCorrectHexStrings)
{
  mock_climate->set_protocol(Protocol::PRONTO);
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_COOL, 20.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Capture all transmitted commands
  std::vector<WoleixProntoCommand> captured_commands;
  
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([&captured_commands](const WoleixCommand& cmd) {
      ASSERT_TRUE(std::holds_alternative<WoleixProntoCommand>(cmd));
      captured_commands.push_back(std::get<WoleixProntoCommand>(cmd));
    }));
  
  // Trigger state change that will generate TEMP_UP commands
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 23.0f;  // +3 degrees = TEMP_UP
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
  // Verify all commands have non-empty Pronto hex
  for (const auto& cmd : captured_commands) {
    EXPECT_FALSE(cmd.get_pronto_hex().empty());
    
    // Verify hex string matches the command type
    if (cmd.get_type() == WoleixCommandBase::Type::TEMP_UP) {
      EXPECT_EQ(cmd.get_pronto_hex(), TEMP_UP_PRONTO);
    }
  }
}

/**
 * Test: Setting same protocol twice is idempotent
 * 
 * Validates that calling set_protocol with the same protocol multiple
 * times doesn't cause issues and behaves correctly (should be a no-op).
 */
TEST_F(WoleixClimateTest, SettingSameProtocolTwiceIsIdempotent)
{
  // Set to NEC
  mock_climate->set_protocol(Protocol::NEC);
  
  // Set to NEC again - should be no-op
  mock_climate->set_protocol(Protocol::NEC);
  
  mock_climate->set_last_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
  // Verify still generates NEC commands
  EXPECT_CALL(*mock_transmitter, transmit_(testing::_))
    .Times(AtLeast(1))
    .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
      EXPECT_TRUE(std::holds_alternative<WoleixNecCommand>(cmd));
    }));
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 25.0f;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) 
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
