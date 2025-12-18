#pragma once

#include <string>
#include <cstdint>

namespace esphome {
namespace climate_ir_woleix {

/**
 * @brief Current component version.
 */
static const char *const VERSION = "0.2.4";

/**
 * @brief Tag for logging messages related to the Woleix climate component.
 */
static const char *const TAG = "climate_ir_woleix.climate";

/**
 * @brief Minimum temperature supported by the Woleix AC unit in Celsius.
 */
const float WOLEIX_TEMP_MIN = 15.0f;

/**
 * @brief Maximum temperature supported by the Woleix AC unit in Celsius.
 */
const float WOLEIX_TEMP_MAX = 30.0f;

/**
 * @name IR Command Definitions
 * NEC IR commands for Woleix AC remote control.
 * @{
 */

/**
 * @brief Woleix NEC address for IR communication.
 */
static const uint16_t ADDRESS_NEC   = 0xFB04;

/**
 * @brief NEC code for Power button (toggles AC unit on/off).
 */
static const uint16_t POWER_NEC     = 0xFB04;

/**
 * @brief NEC code for Temperature Up button (increases temperature by 1°C).
 */
static const uint16_t TEMP_UP_NEC   = 0xFA05;

/**
 * @brief NEC code for Temperature Down button (decreases temperature by 1°C).
 */
static const uint16_t TEMP_DOWN_NEC = 0xFE01;

/**
 * @brief NEC code for Mode button (cycles through COOL -> DEHUM -> FAN modes).
 */
static const uint16_t MODE_NEC      = 0xF20D;

/**
 * @brief NEC code for Speed/Fan button (toggles between LOW and HIGH fan speed).
 */
static const uint16_t SPEED_NEC     = 0xF906;

/**
 * @brief NEC code for Timer button (controls timer function, not currently used).
 */
static const uint16_t TIMER_NEC     = 0xFF00;

/** @} */  // End of IR Command Definitions


}  // namespace climate_ir_woleix
}  // namespace esphome
