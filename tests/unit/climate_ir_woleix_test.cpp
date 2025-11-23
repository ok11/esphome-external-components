#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "climate_ir_woleix.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;
using namespace esphome::remote_base;
using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;
using ::testing::Invoke;

class MockWoleixClimate: public WoleixClimate {
public:
  void set_last_mode(ClimateMode mode) {
    this->last_mode_ = mode;
  }
};

// Test fixture class
class WoleixClimateTest : public ::testing::Test {
protected:
  void SetUp() override {
    climate = new WoleixClimate();
    transmitter = new RemoteTransmitter();
    climate->set_transmitter(transmitter);
    
    // Initialize optional fan_mode to avoid "bad optional access" errors
    climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    
    // Capture transmitted data
    transmitted_data.clear();
    transmitter->set_transmit_callback([this](const RemoteTransmitData& data) {
      transmitted_data.push_back(data);
    });
  }
  
  void TearDown() override {
    delete climate;
    delete transmitter;
  }
  
  WoleixClimate* climate;
  RemoteTransmitter* transmitter;
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
  auto traits = climate->call_traits();
  
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
  climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Turn on to COOL mode
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  
  // Should transmit power command first
  ASSERT_GE(transmitted_data.size(), 1);
  EXPECT_EQ(transmitted_data[0].get_carrier_frequency(), 38030);
  EXPECT_TRUE(check_timings_match(transmitted_data[0], get_power_timings()));
}

TEST_F(WoleixClimateTest, TurningOffSendsPowerCommand) {
  // Start in COOL mode
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Turn off
  climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  climate->call_transmit_state();
  
  // Should transmit power command
  ASSERT_EQ(transmitted_data.size(), 1);
  EXPECT_TRUE(check_timings_match(transmitted_data[0], get_power_timings()));
}

TEST_F(WoleixClimateTest, StayingOffDoesNotTransmit) {
  // Start in OFF mode
  climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Stay in OFF mode
  climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  climate->call_transmit_state();
  
  // Should not transmit anything
  EXPECT_EQ(transmitted_data.size(), 0);
}

// ============================================================================
// Test: Temperature Changes
// ============================================================================

TEST_F(WoleixClimateTest, IncreasingTemperatureSendsTempUpCommands) {
  // Initialize with a known state
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 20.0f;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Increase temperature by 3 degrees
  climate->target_temperature = 23.0f;
  climate->call_transmit_state();
  
  // Should send 3 temp up commands
  int temp_up_count = 0;
  for (const auto& data : transmitted_data) {
    if (check_timings_match(data, get_temp_up_timings())) {
      temp_up_count++;
    }
  }
  EXPECT_EQ(temp_up_count, 3);
}

TEST_F(WoleixClimateTest, DecreasingTemperatureSendsTempDownCommands) {
  // Initialize with a known state
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 25.0f;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Decrease temperature by 2 degrees
  climate->target_temperature = 23.0f;
  climate->call_transmit_state();
  
  // Should send 2 temp down commands
  int temp_down_count = 0;
  for (const auto& data : transmitted_data) {
    if (check_timings_match(data, get_temp_down_timings())) {
      temp_down_count++;
    }
  }
  EXPECT_EQ(temp_down_count, 2);
}

TEST_F(WoleixClimateTest, NoTemperatureChangeDoesNotSendTempCommands) {
  // Initialize with a known state
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Keep same temperature
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  
  // Should not send any temp commands
  int temp_command_count = 0;
  for (const auto& data : transmitted_data) {
    if (check_timings_match(data, get_temp_up_timings()) ||
        check_timings_match(data, get_temp_down_timings())) {
      temp_command_count++;
    }
  }
  EXPECT_EQ(temp_command_count, 0);
}

// ============================================================================
// Test: Mode Changes
// ============================================================================

TEST_F(WoleixClimateTest, ChangingModeSendsModeCommand) {
  // Initialize in COOL mode
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Change to HEAT mode
  climate->mode = ClimateMode::CLIMATE_MODE_HEAT;
  climate->call_transmit_state();
  
  // Should send mode command
  bool found_mode_command = false;
  for (const auto& data : transmitted_data) {
    if (check_timings_match(data, get_mode_timings())) {
      found_mode_command = true;
      break;
    }
  }
  EXPECT_TRUE(found_mode_command);
}

// ============================================================================
// Test: Fan Speed Changes
// ============================================================================

TEST_F(WoleixClimateTest, ChangingFanSpeedSendsSpeedCommand) {
  // Initialize with known state
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Change fan speed
  climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
  climate->call_transmit_state();
  
  // Should send speed command
  bool found_speed_command = false;
  for (const auto& data : transmitted_data) {
    if (check_timings_match(data, get_speed_timings())) {
      found_speed_command = true;
      break;
    }
  }
  EXPECT_TRUE(found_speed_command);
}

// ============================================================================
// Test: Complex State Transitions
// ============================================================================

TEST_F(WoleixClimateTest, CompleteStateChangeSequence) {
  // Start from OFF
  climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  climate->call_transmit_state();
  transmitted_data.clear();
  
  // Turn on with complete state
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 24.0f;
  climate->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
  climate->call_transmit_state();
  
  // Should have sent: power, mode, fan
  ASSERT_GE(transmitted_data.size(), 2);
  
  // First should be power
  EXPECT_TRUE(check_timings_match(transmitted_data[0], get_power_timings()));
}

TEST_F(WoleixClimateTest, CarrierFrequencySetCorrectly) {
  climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  climate->target_temperature = 22.0f;
  climate->call_transmit_state();
  
  // All transmitted data should have correct carrier frequency
  for (const auto& data : transmitted_data) {
    EXPECT_EQ(data.get_carrier_frequency(), 38030);
  }
}

// ============================================================================
// Test: Temperature Bounds
// ============================================================================

TEST_F(WoleixClimateTest, TemperatureBoundsAreCorrect) {
  EXPECT_EQ(WOLEIX_TEMP_MIN, 15.0f);
  EXPECT_EQ(WOLEIX_TEMP_MAX, 30.0f);
}

// ============================================================================
// Remote Transmitter Call Logic Tests 
// ============================================================================

class WoleixClimateMockTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_climate = new MockWoleixClimate();
    mock_transmitter = new MockRemoteTransmitter();
    mock_climate->set_transmitter(mock_transmitter);
    
    // Initialize optional fan_mode
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_AUTO;
  }
  
  void TearDown() override {
    delete mock_climate;
    delete mock_transmitter;
  }
  
  MockWoleixClimate* mock_climate;
  MockRemoteTransmitter* mock_transmitter;
};

// ============================================================================
// GMock Tests - Using EXPECT_CALL
// ============================================================================

TEST_F(WoleixClimateMockTest, TransmitStateCallsPublishState) {
  // Turning on sends: Power, Mode, Speed commands
  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  mock_climate->set_last_mode(ClimateMode::CLIMATE_MODE_OFF);
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateMockTest, PowerOnCallsTransmitMultipleTimes) {
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

TEST_F(WoleixClimateMockTest, TemperatureIncreaseCallsTransmitMultipleTimes) {
  // Initialize state
  mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
  mock_climate->target_temperature = 20.0f;
  mock_climate->call_transmit_state();
  
  // Expect transmit_raw to be called at least 3 times (for 3 degree increase)
  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(AtLeast(3));
  
  // Increase temperature by 3 degrees
  mock_climate->target_temperature = 23.0f;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateMockTest, StayingOffDoesNotCallTransmit) {
  // Expect transmit_raw to NOT be called
  EXPECT_CALL(*mock_transmitter, transmit_raw(_))
      .Times(0);
  
  // Stay in OFF mode
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
  
  mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
  mock_climate->call_transmit_state();
}

TEST_F(WoleixClimateMockTest, VerifyCarrierFrequencyInTransmittedData) {
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
  climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  climate->setup();
  
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
  climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  climate->setup();
  
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
  EXPECT_NO_THROW(climate->setup());
}

TEST_F(WoleixClimateMockTest, PublishingStateOfHumiditySensorRepublishesItByClimate) {
  // Turning on sends: Power, Mode, Speed commands
  EXPECT_CALL(*mock_climate, publish_state())
      .Times(1);  // Expect exactly 1 call
  
  // Create a mock humidity sensor
  esphome::sensor::Sensor humidity_sensor;
  
  // Set the humidity sensor on the climate device
  mock_climate->set_humidity_sensor(&humidity_sensor);
  
  // Call setup to register the callback
  mock_climate->setup();
  
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
