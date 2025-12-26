#include <optional>
#include <functional>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "esphome/components/remote_base/remote_base.h"

#include "climate_ir_woleix.h"
#include "woleix_state_mapper.h"
#include "woleix_command.h"
#include "woleix_protocol_handler.h"
#include "woleix_status.h"

#include "mock_queue.h"
#include "mock_scheduler.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;
using namespace esphome::remote_base;

using testing::_;
using testing::Return;
using testing::AtLeast;
using testing::Invoke;

// namespace WoleixCategory::Testing
// {
//     inline constexpr auto WX_CATEGORY_JUST_TESTING =
//         Category::make(CategoryId::Testing, 1, "Testing.Whatever");
// }

// Custom GMock matcher for checking WoleixCommand type
MATCHER_P(IsCommandOfType, expected_type, "")
{
    return arg.get_type() == expected_type;
}

class MockRemoteTransmitterBase : public RemoteTransmitterBase
{
public:
    MOCK_METHOD(void, send_, (const NECProtocol::ProtocolData& data, uint32_t repeats, uint32_t wait), ());
};


// GMock version of WoleixClimate
// Mock for WoleixStatusReporter
class MockWoleixStatusReporter : public WoleixStatusReporter {
public:
    MOCK_METHOD(void, report_status, (const WoleixStatus& status));
};

class MockWoleixClimate : public WoleixClimate
{
public:
    MockWoleixClimate() : WoleixClimate()
    {
        ON_CALL(*this, get_state())
            .WillByDefault(Invoke
            (
                [this]() -> const WoleixInternalState&
                {
                    return WoleixClimate::get_state();  // Call the real method
                }
            )
        );

    }
    
    // Make transmit_state public for testing
    using WoleixClimate::transmit_state;
    using WoleixClimate::traits;

    // Expose on_hold_ flag for testing
    void set_on_hold(bool on_hold) { on_hold_ = on_hold; }
    bool get_on_hold() { return on_hold_; }
    
    void call_update_state() { update_state_(); }

    // Helper to set state via the mock state machine
    void set_climate_state
    (
        ClimateMode mode,
        float target_temperature = 25.0f,
        ClimateFanMode fan_mode = ClimateFanMode::CLIMATE_FAN_LOW
    )
    {
        WoleixPowerState woleix_power = StateMapper::esphome_to_woleix_power(mode != ClimateMode::CLIMATE_MODE_OFF);
        WoleixMode woleix_mode = StateMapper::esphome_to_woleix_mode(mode);
        WoleixFanSpeed woleix_fan_speed = StateMapper::esphome_to_woleix_fan_mode(fan_mode);

        set_internal_state(
            woleix_power, woleix_mode, target_temperature, woleix_fan_speed
        );
    }

    void get_climate_state(ClimateMode& mode, float& target_temperature, ClimateFanMode& fan_mode)
    {
        WoleixInternalState woleix_state = get_internal_state();

        mode = StateMapper::woleix_to_esphome_power(woleix_state.power) 
                ? StateMapper::woleix_to_esphome_mode(woleix_state.mode) 
                : ClimateMode::CLIMATE_MODE_OFF;
        target_temperature = woleix_state.temperature;
        fan_mode = StateMapper::woleix_to_esphome_fan_mode(woleix_state.fan_speed);
    }

    void set_internal_state
    (
        WoleixPowerState power, WoleixMode woleix_mode, float temperature, WoleixFanSpeed woleix_fan_speed
    )
    {
        current_state_.power = power;
        current_state_.mode = woleix_mode;
        current_state_.temperature = temperature;
        current_state_.fan_speed = woleix_fan_speed;
    }

    WoleixInternalState get_internal_state()
    {
        return current_state_;
    }

    void set_scheduler(MockScheduler* scheduler)
    {
        scheduler_ = scheduler;
    }
    
    // Add convenience method to run until empty
    void run_until_empty()
    {
        if (scheduler_ && command_queue_)
        {
            // Process commands one at a time without triggering setting mode timeout
            while (!command_queue_->is_empty())
            {
                scheduler_->fire_timeout("proto_next_cmd");
            }
        }
    }
    
    // Override Component's set_timeout to use our mock scheduler
    void set_timeout(const std::string& name, uint32_t delay_ms, std::function<void()>&& callback) override
    {
        if (scheduler_)
        {
            scheduler_->get_setter()(name, delay_ms, std::move(callback));
        }
    }
    
    // Override Component's cancel_timeout to use our mock scheduler
    bool cancel_timeout(const std::string& name) override
    {
        if (scheduler_)
        {
            scheduler_->get_canceller()(name);
        }
        return true;
    }

    esphome::sensor::Sensor* get_humidity_sensor()
    {
        return this->humidity_sensor_;
    }

    void enqueue_commands(const std::vector<WoleixCommand>& commands)
    {
        command_queue_->enqueue(commands);
    }

    MOCK_METHOD(const WoleixInternalState&, get_state, (), (const, override));
    MOCK_METHOD(void, publish_state, (), (override)); 
    MOCK_METHOD(void, transmit_, (const WoleixCommand& command), ());
    MOCK_METHOD(void, report_status, (const WoleixStatus& status), (override));
    
    // Make observe method public for testing
    using WoleixClimate::observe;
        
    MockScheduler* scheduler_{nullptr};
};

// Test fixture class
class WoleixClimateTest : public testing::Test 
{
protected:
    void SetUp() override 
    {
        mock_transmitter = new MockRemoteTransmitterBase();
        mock_scheduler = new MockScheduler();
        mock_climate = new testing::NiceMock<MockWoleixClimate>();
        mock_climate->set_scheduler(mock_scheduler);
        mock_climate->set_transmitter(mock_transmitter);    
        mock_climate->setup();
    }
    
    void TearDown() override
    {
        delete mock_climate;
        delete mock_scheduler;
        delete mock_transmitter;
    }

    const std::vector<WoleixCommand>& fill_commands(size_t number)
    {
        commands.clear();
        WoleixCommand cmd(WoleixCommand::Type::POWER, 0, 1);
        for (int i = 0; i < number; i++)
        {
            commands.push_back(cmd);
        }
        return commands;
    }

    MockRemoteTransmitterBase* mock_transmitter;
    MockScheduler* mock_scheduler;
    testing::NiceMock<MockWoleixClimate>* mock_climate;

    std::vector<WoleixCommand> commands;
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
    auto traits = mock_climate->traits();
  
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_OFF,
        25.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::POWER)))
        .Times(1);
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::MODE)))
        .Times(2);
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(1);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->target_temperature = 25.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->call_transmit_state();
 
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state(
        ClimateMode::CLIMATE_MODE_COOL,
        25.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
);

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::POWER)))
        .Times(1);  

    // Turn off
    mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
    mock_climate->target_temperature = 25.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();

    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);

    EXPECT_CALL(*mock_climate, transmit_(_))
        .Times(0);

    // Stay in OFF mode
    mock_climate->mode = ClimateMode::CLIMATE_MODE_OFF;
    mock_climate->target_temperature = 25.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

// ============================================================================
// Test: Temperature Changes
// ============================================================================

/**
 * Test: Increasing temperature sends multiple TEMP_UP commands
 * 
 * When raising the temperature (e.g., from 20°C to 23°C), the component
 * should send n+1 TEMP_UP IR commands (4 in this case: 1 to enter setting
 * mode + 3 to actually change the temperature).
 * Temperature adjustment only works in COOL mode.
 */
TEST_F(WoleixClimateTest, IncreasingTemperatureSendsTempUpCommands)
{
    // Initialize with a known state
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_COOL,
        20.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    // n+1 rule: 3 degree change = 4 transmissions (1 enter setting mode + 3 actual)
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::TEMP_UP)))
        .Times(4);

    // Increase temperature by 3 degrees
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->target_temperature = 23.0f;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

/**
 * Test: Decreasing temperature sends multiple TEMP_DOWN commands
 * 
 * When lowering the temperature (e.g., from 25°C to 23°C), the component
 * should send n+1 TEMP_DOWN IR commands (3 in this case: 1 to enter setting
 * mode + 2 to actually change the temperature).
 * Temperature adjustment only works in COOL mode.
 */
TEST_F(WoleixClimateTest, DecreasingTemperatureSendsTempDownCommands)
{
    // Initialize with a known state
    mock_climate->set_climate_state(
        ClimateMode::CLIMATE_MODE_COOL,
        25.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    // n+1 rule: 2 degree change = 3 transmissions (1 enter setting mode + 2 actual)
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::TEMP_DOWN)))
        .Times(3);

    // Decrease temperature by 2 degrees
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 23.0f;
    mock_climate->transmit_state();  
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state(
        ClimateMode::CLIMATE_MODE_COOL,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );
  
    EXPECT_CALL(*mock_climate, transmit_(_))
        .Times(0);

        // Keep same temperature
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 22.0f;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();  
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
    mock_climate->set_climate_state(
        ClimateMode::CLIMATE_MODE_FAN_ONLY,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );
  
    EXPECT_CALL(*mock_climate, transmit_(_))
        .Times(0);

        // Keep same temperature
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->target_temperature = 25.0f;
    mock_climate->transmit_state();  
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state(
        ClimateMode::CLIMATE_MODE_COOL,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::MODE)))
        .Times(2);
  
    // Change to FAN mode
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->target_temperature = 22.0f;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_DRY,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::MODE)))
        .Times(2);
  
    // Change to FAN mode
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 22.0f;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_COOL,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::MODE)))
        .Times(1);
  
    // Change to FAN mode
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_DRY;
    mock_climate->target_temperature = 22.0f;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_FAN_ONLY,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(1);

        // Change fan speed
    mock_climate->target_temperature = 22.0f;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_FAN_ONLY,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_HIGH
    );

    testing::InSequence seq;  // Enforce call order
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(1);

    // Change fan speed
    mock_climate->target_temperature = 22.0f;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_COOL,
        22.0f,
        ClimateFanMode::CLIMATE_FAN_HIGH
    );
  
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(0);

    // Change fan speed
    mock_climate->target_temperature = 22.0f;
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
 * 
 * Note: Temperature change follows n+1 rule (4 degree change = 5 transmissions:
 * 1 to enter setting mode + 4 to actually change the temperature).
 */
TEST_F(WoleixClimateTest, CompleteStateChangeSequence)
{
    // Start from DRY mode
    mock_climate->set_climate_state
    (
        ClimateMode::CLIMATE_MODE_DRY,
        20.0f,
        ClimateFanMode::CLIMATE_FAN_LOW
    );

    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(0); // the target state is not FAN, so fan speed change won't be sent
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::MODE)))
        .Times(2);
    // n+1 rule: 4 degree change (20→24) = 5 transmissions
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::TEMP_UP)))
        .Times(5);

    // Turn on with complete state
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 24.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);

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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
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

    mock_climate->set_climate_state
    (
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
 * Test: Fan speed is only transmitted in FAN mode
 * 
 * Validates that the SPEED_COMMAND is only sent when the climate
 * is in FAN mode. In other modes (COOL, DRY), fan speed changes
 * should not trigger a SPEED_COMMAND.
 */
TEST_F(WoleixClimateTest, FanSpeedOnlyTransmittedInFanMode)
{
    // Test in COOL mode
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_COOL, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(0);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 22.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();

    // Test in DRY mode
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_DRY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(0);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_DRY;
    mock_climate->target_temperature = 22.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();

    // Test in FAN mode
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(1);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->target_temperature = 22.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_HIGH;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();

    // Test that changing temperature in FAN mode doesn't trigger SPEED_COMMAND
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_FAN_ONLY, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::FAN_SPEED)))
        .Times(0);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_FAN_ONLY;
    mock_climate->target_temperature = 24.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_COOL, 20.0f, ClimateFanMode::CLIMATE_FAN_LOW);

    // Trigger a state change: stay in COOL mode, change temperature to 24°C
    // This ensures temperature commands are actually generated (only works in COOL mode)
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 24.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;

    mock_climate->transmit_state();
    mock_climate->run_until_empty();

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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    // Expect NEC command (detected by checking if it's a WoleixNecCommand variant)
    EXPECT_CALL(*mock_climate, transmit_(testing::_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
            // Verify NEC-specific properties
            EXPECT_EQ(cmd.get_address(), ADDRESS_NEC);
        }));
  
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 25.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

/**
 * Test: NEC commands
 * 
 * Validates that the climate component generates proper NEC format commands.
 */
TEST_F(WoleixClimateTest, SetProtocolNecGeneratesNecCommands)
{
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_OFF, 25.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    // Expect NEC command (detected by checking if it's a WoleixNecCommand variant)
    EXPECT_CALL(*mock_climate, transmit_(testing::_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([](const WoleixCommand& cmd) {
            // Verify this is a NEC command by checking variant index
      
            EXPECT_EQ(cmd.get_address(), ADDRESS_NEC);
        }));
  
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 25.0f;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;

    mock_climate->transmit_state();
    mock_climate->run_until_empty();
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
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_COOL, 20.0f, ClimateFanMode::CLIMATE_FAN_LOW);
  
    // Capture all transmitted commands
    std::vector<WoleixCommand> captured_commands;
  
    EXPECT_CALL(*mock_climate, transmit_(testing::_))
        .Times(AtLeast(1))
        .WillRepeatedly(Invoke([&captured_commands](const WoleixCommand& cmd) {
            captured_commands.push_back(cmd);
        }));
  
    // Trigger state change that will generate TEMP_UP commands
    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 23.0f;  // +3 degrees = TEMP_UP
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;
    mock_climate->transmit_state();
    mock_climate->run_until_empty();
  
    // Verify all commands have correct NEC address
    for (const auto& cmd : captured_commands) {
        EXPECT_EQ(cmd.get_address(), ADDRESS_NEC);
    
        // Verify command code matches the command type
        if (cmd.get_type() == WoleixCommand::Type::TEMP_UP) {
            EXPECT_EQ(cmd.get_command(), TEMP_UP_NEC);
        }
    }
}

TEST_F(WoleixClimateTest, SetTransmitterPropagation)
{
  auto mock_transmitter = std::make_shared<MockRemoteTransmitterBase>();

  mock_climate->set_transmitter(mock_transmitter.get());

  // Verify that the transmitter was correctly set in the WoleixTransmitter
  EXPECT_EQ(mock_climate->get_transmitter(), mock_transmitter.get());
}

TEST_F(WoleixClimateTest, PowerOnAfterReset)
{
    mock_climate->reset_state();

    EXPECT_CALL(*mock_climate, transmit_(IsCommandOfType(WoleixCommand::Type::POWER)))
        .Times(1);

    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = WOLEIX_TEMP_DEFAULT;
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;

    mock_climate->transmit_state();
    mock_climate->run_until_empty();
}

// ============================================================================
// Test: Transmission on hold
// ============================================================================

TEST_F(WoleixClimateTest, TransmitStateOnHoldTrue)
{
    mock_climate->set_on_hold(true);

    EXPECT_CALL(*mock_climate, report_status(testing::_))
        .WillOnce
        (
            testing::Invoke
            (
                [](const WoleixStatus& status)
                {
                    EXPECT_EQ(status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_WARNING);
                    EXPECT_EQ(status.get_category(), WoleixCategory::Core::WX_CATEGORY_ENQUEING_ON_HOLD);
                }
            )
        );
    mock_climate->transmit_state();
}

TEST_F(WoleixClimateTest, TransmitStateOnHoldFalse)
{
    mock_climate->set_on_hold(false);

    EXPECT_CALL(*mock_climate, report_status).Times(0);

    mock_climate->transmit_state();
}

TEST_F(WoleixClimateTest, EnqueueCommandsFailure)
{
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_COOL, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);

    mock_climate->enqueue_commands(fill_commands(QUEUE_MAX_CAPACITY - 2));
    mock_climate->set_on_hold(false);

    EXPECT_CALL(*mock_climate, report_status(testing::_))
        .WillOnce
        (
            testing::Invoke
            (
                [](const WoleixStatus& status)
                {
                    EXPECT_EQ(status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_ERROR);
                    EXPECT_EQ(status.get_category(), WoleixCategory::Core::WX_CATEGORY_ENQUEING_FAILED);
                }
            )
        );

    mock_climate->mode = ClimateMode::CLIMATE_MODE_COOL;
    mock_climate->target_temperature = 25.0f; // 3 commands
    mock_climate->fan_mode = ClimateFanMode::CLIMATE_FAN_LOW;

    mock_climate->transmit_state();
}

/**
 * In case if a state transition (list of commands) cannot be enqueued
 * for transmission (e.g. the queue is full), the state must be 
 * "rolled back" to the previous one. 
 */
TEST_F(WoleixClimateTest, UpdateStateRollbackTest)
{
    // 1. Set initial "real" device state (what get_state() should return on rollback)
    WoleixInternalState actual_device_state =
        WoleixInternalStateBuilder()
            .power(WoleixPowerState::ON)
            .mode(WoleixMode::DEHUM)
            .temperature(20.0f)
            .fan(WoleixFanSpeed::HIGH)
            .build();

    // 2. Simulate optimistic update — Climate UI shows the *requested* state
    mock_climate->set_climate_state(ClimateMode::CLIMATE_MODE_COOL, 22.0f, ClimateFanMode::CLIMATE_FAN_LOW);

    // 3. Mock get_state() to return actual device state (command failed, device didn't change)
    EXPECT_CALL(*mock_climate, get_state())
        .WillOnce(testing::ReturnRef(actual_device_state));

    // 4. Trigger rollback
    mock_climate->call_update_state();

    // 5. Verify UI rolled back to actual device state
    EXPECT_EQ(mock_climate->mode, StateMapper::woleix_to_esphome_mode(actual_device_state.mode));
    EXPECT_EQ(mock_climate->target_temperature, 20.0f);
    EXPECT_EQ(mock_climate->fan_mode, StateMapper::woleix_to_esphome_fan_mode(actual_device_state.fan_speed));
}

// ============================================================================
// Test: Observe Method
// ============================================================================

TEST_F(WoleixClimateTest, ObserveMethodTest)
{
    // Create a mock WoleixStatus object
    WoleixStatus mock_status(WoleixStatus::Severity::WX_SEVERITY_INFO, Category::make(99, 99, "Testing.Testing"), "Test message");

    // Create a mock WoleixStatusReporter
    MockWoleixStatusReporter mock_reporter;

    // Set up expectations
    EXPECT_CALL(*mock_climate, report_status(testing::_))
        .Times(1)
        .WillOnce(testing::Invoke([&mock_status](const WoleixStatus& status) {
            EXPECT_EQ(status.get_severity(), mock_status.get_severity());
            EXPECT_EQ(status.get_category(), mock_status.get_category());
        }));

    // Call observe method
    mock_climate->observe(mock_reporter, mock_status);
}

// ============================================================================
// Test: Report Status Method
// ============================================================================

TEST_F(WoleixClimateTest, ReportStatusMethodTest)
{
    // Create a WoleixStatus object
    WoleixStatus mock_status(WoleixStatus::Severity::WX_SEVERITY_WARNING, Category::make(99, 99, "Testing.Testing"), "Test warning message");

    EXPECT_CALL(*mock_climate, report_status(testing::_))
        .Times(1)
        .WillOnce(testing::Invoke([&mock_status](const WoleixStatus& status) {
            EXPECT_EQ(status.get_severity(), mock_status.get_severity());
            EXPECT_EQ(status.get_category(), mock_status.get_category());
        }));

    // Call report_status method
    mock_climate->report_status(mock_status);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) 
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
