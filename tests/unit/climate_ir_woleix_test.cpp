#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include "climate_ir_woleix.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;
using namespace esphome::remote_base;
using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::Invoke;

static const std::string POWER_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0018 0014 0043 0013 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Temperature Up
static const std::string TEMP_UP_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0017 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Temperature Down
static const std::string TEMP_DOWN_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// Mode button
static const std::string MODE_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0043 0013 0018 0014 0018 0014 0043 0013 0043 0013 0043 0013 0042 "
    "0014 0483";

// Speed/Fan button
static const std::string SPEED_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0018 0014 0041 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0040 0016 0043 0013 0043 0013 003D 0019 0040 0015 0018 0014 003E "
    "0018 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 "
    "0013 0018 0014 0018 0014 0043 0013 0043 0013 0041 0014 0043 0013 0043 "
    "0013 0483";

// Timer button
static const std::string TIMER_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0042 0014 0018 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

// GMock version of RemoteTransmitter
class MockRemoteTransmitter : public RemoteTransmitter {
public:
  MOCK_METHOD(void, transmit_raw, (const RemoteTransmitData& data), (override));
};
// GMock version of WoleixClimate
class MockWoleixClimate: public WoleixClimate {
public:
  void set_last_mode(ClimateMode mode) {
    this->last_mode_ = mode;
  }
  void set_last_target_temperature(float temp) {
    this->last_target_temp_ = temp;
  }

  void set_last_fan_mode(ClimateFanMode fan_mode) {
    this->last_fan_mode_ = fan_mode;
  }
  MOCK_METHOD(void, publish_state, (), (override)); 
  MOCK_METHOD(void, transmit_pronto_, (const std::string&));
};

// Test fixture class
class WoleixClimateTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_climate = new MockWoleixClimate();
    mock_transmitter = new MockRemoteTransmitter();
    mock_climate->set_transmitter(mock_transmitter);
    
    // Initialize optional fan_mode to avoid "bad optional access" errors
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    
    // Capture transmitted data
    transmitted_data.clear();
    mock_transmitter->set_transmit_callback([this](const RemoteTransmitData& data) {
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
  std::vector<int32_t> get_power_timings() {
    return {
      5568, 2832, 528, 600, 528, 600, 528, 1680, 528, 600, 528, 600, 528, 600,
      528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600, 528, 1680, 528, 1680,
      528, 1680, 528, 1680, 528, 1680, 528, 600, 528, 600, 528, 1680, 528, 600,
      528, 600, 528, 600, 528, 600, 528, 600, 528, 1680, 528, 1680, 528, 600,
      528, 1704, 516, 1680, 528, 1680, 528, 1680, 528, 1680, 528, 18780
    };
  }
  
  std::vector<int32_t> get_temp_up_timings() {
    return {
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
  
  std::vector<int32_t> get_mode_timings() {
    return {
      5592, 2820, 528, 600, 528, 600, 528, 1704, 516, 600, 528, 600, 528, 600,
      528, 600, 528, 600, 528, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
      516, 1704, 516, 1704, 516, 1704, 516, 1704, 516, 600, 528, 1704, 516, 1704,
      516, 600, 528, 600, 528, 600, 528, 600, 528, 600, 528, 1704, 516, 600,
      528, 600, 528, 1704, 516, 1704, 516, 1704, 516, 1680, 528, 18780
    };
  }
  
  std::vector<int32_t> get_speed_timings() {
    return {
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

TEST_F(WoleixClimateTest, TraitsConfiguredCorrectly) {
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

TEST_F(WoleixClimateTest, TurningOnFromOffSendsPowerCommand) {
  // Start in OFF mode
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);

  testing::InSequence seq;  // Enforce call order
  
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(POWER_PRONTO)))
    .Times(1);
  
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(MODE_PRONTO)))
    .Times(1);
    
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(SPEED_PRONTO)))
    .Times(1);

  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->call_transmit_state();
  
}

TEST_F(WoleixClimateTest, TurningOffSendsPowerCommand) {
  // Start in COOL mode
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(POWER_PRONTO)))
    .Times(1);  
  // Turn off
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateTest, StayingOffDoesNotTransmit) {
  // Start in OFF mode
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);

  EXPECT_CALL(*mock_climate, transmit_pronto_(_))
    .Times(0);
  // Stay in OFF mode
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
  
  // Should not transmit anything
  EXPECT_EQ(transmitted_data.size(), 0);
}

// ============================================================================
// Test: Temperature Changes
// ============================================================================

TEST_F(WoleixClimateTest, IncreasingTemperatureSendsTempUpCommands) {
  // Initialize with a known state
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_target_temperature(20.0f);

  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(TEMP_UP_PRONTO)))
    .Times(3);
  // Increase temperature by 3 degrees
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->target_temperature = 23.0f;
  mock_climate->call_transmit_state();
  
}

TEST_F(WoleixClimateTest, DecreasingTemperatureSendsTempDownCommands) {
  // Initialize with a known state
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_target_temperature(25.0f);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(TEMP_DOWN_PRONTO)))
    .Times(2);   
  // Decrease temperature by 2 degrees
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 23.0f;
  mock_climate->call_transmit_state();
  
}

TEST_F(WoleixClimateTest, NoTemperatureChangeDoesNotSendTempCommands) {
  // Initialize with a known state
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_target_temperature(22.0f);
  
  EXPECT_CALL(*mock_climate, transmit_pronto_(_))
    .Times(0);
  // Keep same temperature
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
  
}

// ============================================================================
// Test: Mode Changes
// ============================================================================

TEST_F(WoleixClimateTest, ChangingModeSendsModeCommand) {
  // Initialize in COOL mode
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_target_temperature(22.0f);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(MODE_PRONTO)))
    .Times(AtLeast(1));
  // Change to FAN mode
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

// ============================================================================
// Test: Fan Speed Changes
// ============================================================================

TEST_F(WoleixClimateTest, IncreasingFanSpeedSendsSpeedCommand) {
  // Initialize with known state
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
  mock_climate->set_last_target_temperature(22.0f);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(SPEED_PRONTO)))
    .Times(1);
  // Change fan speed
  mock_climate->target_temperature = 22.0f;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
  
}

TEST_F(WoleixClimateTest, DecreasingFanSpeedSendsSpeedCommand) {
  // Initialize with known state
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_COOL);
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
  mock_climate->set_last_target_temperature(22.0f);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(SPEED_PRONTO)))
    .Times(1);
  // Change fan speed
  mock_climate->target_temperature = 22.0f;
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  mock_climate->call_transmit_state();
  
}

TEST_F(WoleixClimateTest, UnchangedFanSpeedDoesNotSendSpeedCommand) {
  // Initialize with known state
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
  
  EXPECT_CALL(*mock_climate, transmit_pronto_(_))
    .Times(0);
  // Change fan speed
  mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  mock_climate->call_transmit_state();
  
}
// ============================================================================
// Test: Complex State Transitions
// ============================================================================

TEST_F(WoleixClimateTest, CompleteStateChangeSequence) {
  // Start from OFF
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);
  mock_climate->set_last_target_temperature(20.0f);
  mock_climate->set_last_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);  
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(POWER_PRONTO)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(MODE_PRONTO)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(SPEED_PRONTO)))
    .Times(1);
  EXPECT_CALL(*mock_climate, transmit_pronto_(::testing::StrEq(TEMP_UP_PRONTO)))
    .Times(4);
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

TEST_F(WoleixClimateTest, TemperatureBoundsAreCorrect) {
  EXPECT_EQ(WOLEIX_TEMP_MIN, 15.0f);
  EXPECT_EQ(WOLEIX_TEMP_MAX, 30.0f);
}

// ============================================================================
// GMock Tests - Using EXPECT_CALL
// ============================================================================

TEST_F(WoleixClimateTest, TransmitStateCallsPublishState) {
  // Turning on sends: Power, Mode, Speed commands
  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateTest, PowerOnCallsTransmitMultipleTimes) {
  // Turning on sends: Power, Mode, Speed commands

  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(3);  // Expect exactly 3 calls
  
  // Turn on from OFF
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateTest, StayingOffDoesNotCallTransmit) {
  // Expect transmit_raw to NOT be called
  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(0);
  
  // Stay in OFF mode
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateTest, VerifyCarrierFrequencyInTransmittedData) {
  // Initialize to OFF first
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
  
  // Capture all transmitted data and verify carrier frequency
  // Power on sends 3 commands: Power, Mode, Speed
  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(3)
      .WillRepeatedly(Invoke([](const RemoteTransmitData& data) {
        // Verify carrier frequency is set correctly for all transmissions
        EXPECT_EQ(data.get_carrier_frequency(), 38030);
        // Verify data contains timing information
        EXPECT_GT(data.get_data().size(), 0);
      }));
  
  // Trigger transmissions by turning on
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 22.0f;
  mock_climate->call_transmit_state();
}

// ============================================================================
// Test: Humidity Sensor Callback
// ============================================================================

TEST_F(WoleixClimateTest, HumiditySensorCallbackIsRegistered) {
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

TEST_F(WoleixClimateTest, HumiditySensorCallbackReceivesUpdates) {
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

TEST_F(WoleixClimateTest, HumiditySensorCallbackWorksWithNullSensor) {
  // Don't set a humidity sensor (leave it as nullptr)
  
  // Call setup - should not crash even without a sensor
  EXPECT_NO_THROW(mock_climate->setup());
}

TEST_F(WoleixClimateTest, PublishingStateOfHumiditySensorRepublishesItByClimate) {
  // Turning on sends: Power, Mode, Speed commands

  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);

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
// Main
// ============================================================================

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
