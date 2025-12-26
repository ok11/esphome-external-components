#include <gtest/gtest.h>

#include "esphome/components/climate/climate_mode.h"

#include "woleix_state_mapper.h"
#include "woleix_state_manager.h"

using namespace esphome::climate_ir_woleix;
using namespace esphome::climate;

// ============================================================================
// Test: Mode Mapping (Woleix -> ESPHome)
// ============================================================================

/**
 * Test: COOL mode maps correctly to ESPHome
 * 
 * Validates that Woleix COOL mode maps to ESPHome CLIMATE_MODE_COOL.
 */
TEST(StateMapperTest, WoleixCoolMapsToEspHomeCool)
{
    auto mode = StateMapper::woleix_to_esphome_mode(WoleixMode::COOL);
    EXPECT_EQ(mode, ClimateMode::CLIMATE_MODE_COOL);
}

/**
 * Test: DEHUM mode maps correctly to ESPHome
 * 
 * Validates that Woleix DEHUM mode maps to ESPHome CLIMATE_MODE_DRY.
 */
TEST(StateMapperTest, WoleixDehumMapsToEspHomeDry)
{
    auto mode = StateMapper::woleix_to_esphome_mode(WoleixMode::DEHUM);
    EXPECT_EQ(mode, ClimateMode::CLIMATE_MODE_DRY);
}

/**
 * Test: FAN mode maps correctly to ESPHome
 * 
 * Validates that Woleix FAN mode maps to ESPHome CLIMATE_MODE_FAN_ONLY.
 */
TEST(StateMapperTest, WoleixFanMapsToEspHomeFanOnly)
{
    auto mode = StateMapper::woleix_to_esphome_mode(WoleixMode::FAN);
    EXPECT_EQ(mode, ClimateMode::CLIMATE_MODE_FAN_ONLY);
}

// ============================================================================
// Test: Mode Mapping (ESPHome -> Woleix)
// ============================================================================

/**
 * Test: ESPHome COOL mode maps correctly to Woleix
 * 
 * Validates that ESPHome CLIMATE_MODE_COOL maps to Woleix COOL mode.
 */
TEST(StateMapperTest, EspHomeCoolMapsToWoleixCool)
{
    auto mode = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_COOL);
    EXPECT_EQ(mode, WoleixMode::COOL);
}

/**
 * Test: ESPHome DRY mode maps correctly to Woleix
 * 
 * Validates that ESPHome CLIMATE_MODE_DRY maps to Woleix DEHUM mode.
 */
TEST(StateMapperTest, EspHomeDryMapsToWoleixDehum)
{
    auto mode = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_DRY);
    EXPECT_EQ(mode, WoleixMode::DEHUM);
}

/**
 * Test: ESPHome FAN_ONLY mode maps correctly to Woleix
 * 
 * Validates that ESPHome CLIMATE_MODE_FAN_ONLY maps to Woleix FAN mode.
 */
TEST(StateMapperTest, EspHomeFanOnlyMapsToWoleixFan)
{
    auto mode = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);
    EXPECT_EQ(mode, WoleixMode::FAN);
}

/**
 * Test: Unsupported ESPHome modes default to COOL
 * 
 * Validates that unsupported ESPHome climate modes (like AUTO, HEAT, etc.)
 * default to Woleix COOL mode.
 */
TEST(StateMapperTest, UnsupportedEspHomeModesDefaultToCool)
{
    auto mode_auto = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_AUTO);
    EXPECT_EQ(mode_auto, WoleixMode::COOL);
  
    auto mode_heat = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_HEAT);
    EXPECT_EQ(mode_heat, WoleixMode::COOL);
  
    auto mode_heat_cool = StateMapper::esphome_to_woleix_mode(ClimateMode::CLIMATE_MODE_HEAT_COOL);
    EXPECT_EQ(mode_heat_cool, WoleixMode::COOL);
}

// ============================================================================
// Test: Fan Speed Mapping (Woleix -> ESPHome)
// ============================================================================

/**
 * Test: LOW fan speed maps correctly to ESPHome
 * 
 * Validates that Woleix LOW fan speed maps to ESPHome CLIMATE_FAN_LOW.
 */
TEST(StateMapperTest, WoleixLowFanMapsToEspHomeLow)
{
    auto fan_mode = StateMapper::woleix_to_esphome_fan_mode(WoleixFanSpeed::LOW);
    EXPECT_EQ(fan_mode, ClimateFanMode::CLIMATE_FAN_LOW);
}

/**
 * Test: HIGH fan speed maps correctly to ESPHome
 * 
 * Validates that Woleix HIGH fan speed maps to ESPHome CLIMATE_FAN_HIGH.
 */
TEST(StateMapperTest, WoleixHighFanMapsToEspHomeHigh) {
    auto fan_mode = StateMapper::woleix_to_esphome_fan_mode(WoleixFanSpeed::HIGH);
    EXPECT_EQ(fan_mode, ClimateFanMode::CLIMATE_FAN_HIGH);
}

// ============================================================================
// Test: Fan Speed Mapping (ESPHome -> Woleix)
// ============================================================================

/**
 * Test: ESPHome LOW fan mode maps correctly to Woleix
 * 
 * Validates that ESPHome CLIMATE_FAN_LOW maps to Woleix LOW fan speed.
 */
TEST(StateMapperTest, EspHomeLowFanMapsToWoleixLow)
{
    auto fan_speed = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
    EXPECT_EQ(fan_speed, WoleixFanSpeed::LOW);
}

/**
 * Test: ESPHome HIGH fan mode maps correctly to Woleix
 * 
 * Validates that ESPHome CLIMATE_FAN_HIGH maps to Woleix HIGH fan speed.
 */
TEST(StateMapperTest, EspHomeHighFanMapsToWoleixHigh)
{
    auto fan_speed = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
    EXPECT_EQ(fan_speed, WoleixFanSpeed::HIGH);
}

/**
 * Test: Unsupported ESPHome fan modes default to LOW
 * 
 * Validates that unsupported ESPHome fan modes (like AUTO, MEDIUM, etc.)
 * default to Woleix LOW fan speed.
 */
TEST(StateMapperTest, UnsupportedEspHomeFanModesDefaultToLow)
{
    auto fan_auto = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
    EXPECT_EQ(fan_auto, WoleixFanSpeed::LOW);
  
    auto fan_medium = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
    EXPECT_EQ(fan_medium, WoleixFanSpeed::LOW);
  
    auto fan_focus = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_FOCUS);
    EXPECT_EQ(fan_focus, WoleixFanSpeed::LOW);
  
    auto fan_diffuse = StateMapper::esphome_to_woleix_fan_mode(ClimateFanMode::CLIMATE_FAN_DIFFUSE);
    EXPECT_EQ(fan_diffuse, WoleixFanSpeed::LOW);
}

// ============================================================================
// Test: Power State Mapping (Woleix -> ESPHome)
// ============================================================================

/**
 * Test: Woleix ON power state maps to true
 * 
 * Validates that Woleix ON power state maps to boolean true for ESPHome.
 */
TEST(StateMapperTest, WoleixOnPowerMapsToTrue)
{
    bool power = StateMapper::woleix_to_esphome_power(WoleixPowerState::ON);
    EXPECT_TRUE(power);
}

/**
 * Test: Woleix OFF power state maps to false
 * 
 * Validates that Woleix OFF power state maps to boolean false for ESPHome.
 */
TEST(StateMapperTest, WoleixOffPowerMapsToFalse)
{
    bool power = StateMapper::woleix_to_esphome_power(WoleixPowerState::OFF);
    EXPECT_FALSE(power);
}

// ============================================================================
// Test: Power State Mapping (ESPHome -> Woleix)
// ============================================================================

/**
 * Test: ESPHome true power maps to Woleix ON
 * 
 * Validates that ESPHome boolean true maps to Woleix ON power state.
 */
TEST(StateMapperTest, EspHomeTruePowerMapsToWoleixOn)
{
    auto power = StateMapper::esphome_to_woleix_power(true);
    EXPECT_EQ(power, WoleixPowerState::ON);
}

/**
 * Test: ESPHome false power maps to Woleix OFF
 * 
 * Validates that ESPHome boolean false maps to Woleix OFF power state.
 */
TEST(StateMapperTest, EspHomeFalsePowerMapsToWoleixOff)
{
    auto power = StateMapper::esphome_to_woleix_power(false);
    EXPECT_EQ(power, WoleixPowerState::OFF);
}

// ============================================================================
// Test: Round-trip Conversions
// ============================================================================

/**
 * Test: Mode conversions are reversible
 * 
 * Validates that converting a mode from Woleix to ESPHome and back
 * results in the original Woleix mode.
 */
TEST(StateMapperTest, ModeConversionsAreReversible)
{
    // COOL
    auto cool_esphome = StateMapper::woleix_to_esphome_mode(WoleixMode::COOL);
    auto cool_woleix = StateMapper::esphome_to_woleix_mode(cool_esphome);
    EXPECT_EQ(cool_woleix, WoleixMode::COOL);
  
    // DEHUM
    auto dehum_esphome = StateMapper::woleix_to_esphome_mode(WoleixMode::DEHUM);
    auto dehum_woleix = StateMapper::esphome_to_woleix_mode(dehum_esphome);
    EXPECT_EQ(dehum_woleix, WoleixMode::DEHUM);
  
    // FAN
    auto fan_esphome = StateMapper::woleix_to_esphome_mode(WoleixMode::FAN);
    auto fan_woleix = StateMapper::esphome_to_woleix_mode(fan_esphome);
    EXPECT_EQ(fan_woleix, WoleixMode::FAN);
}

/**
 * Test: Fan speed conversions are reversible
 * 
 * Validates that converting a fan speed from Woleix to ESPHome and back
 * results in the original Woleix fan speed.
 */
TEST(StateMapperTest, FanSpeedConversionsAreReversible)
{
    // LOW
    auto low_esphome = StateMapper::woleix_to_esphome_fan_mode(WoleixFanSpeed::LOW);
    auto low_woleix = StateMapper::esphome_to_woleix_fan_mode(low_esphome);
    EXPECT_EQ(low_woleix, WoleixFanSpeed::LOW);
  
    // HIGH
    auto high_esphome = StateMapper::woleix_to_esphome_fan_mode(WoleixFanSpeed::HIGH);
    auto high_woleix = StateMapper::esphome_to_woleix_fan_mode(high_esphome);
    EXPECT_EQ(high_woleix, WoleixFanSpeed::HIGH);
}

/**
 * Test: Power state conversions are reversible
 * 
 * Validates that converting a power state from Woleix to ESPHome and back
 * results in the original Woleix power state.
 */
TEST(StateMapperTest, PowerStateConversionsAreReversible)
{
    // ON
    auto on_esphome = StateMapper::woleix_to_esphome_power(WoleixPowerState::ON);
    auto on_woleix = StateMapper::esphome_to_woleix_power(on_esphome);
    EXPECT_EQ(on_woleix, WoleixPowerState::ON);
  
    // OFF
    auto off_esphome = StateMapper::woleix_to_esphome_power(WoleixPowerState::OFF);
    auto off_woleix = StateMapper::esphome_to_woleix_power(off_esphome);
    EXPECT_EQ(off_woleix, WoleixPowerState::OFF);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
