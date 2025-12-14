#pragma once

#include <string>

namespace esphome {
namespace climate_ir_woleix {

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
 * Pronto hex format IR commands for Woleix AC remote control.
 * Carrier frequency: 38.03 kHz (0x006D)
 * @{
 */

/** Power button - toggles AC unit on/off */
static const std::string POWER_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0018 0014 0043 0013 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** Temperature Up button - increases temperature by 1°C */
static const std::string TEMP_UP_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0017 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0018 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** Temperature Down button - decreases temperature by 1°C */
static const std::string TEMP_DOWN_PRONTO = 
    "0000 006D 0022 0000 0158 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** Mode button - cycles through COOL -> DEHUM -> FAN modes */
static const std::string MODE_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 "
    "0014 0043 0013 0018 0014 0018 0014 0043 0013 0043 0013 0043 0013 0042 "
    "0014 0483";

/** Speed/Fan button - toggles between LOW and HIGH fan speed */
static const std::string SPEED_PRONTO = 
    "0000 006D 0022 0000 0158 00B0 0013 0018 0014 0018 0014 0041 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 0014 0042 0014 0018 "
    "0014 0040 0016 0043 0013 0043 0013 003D 0019 0040 0015 0018 0014 003E "
    "0018 0043 0013 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 "
    "0013 0018 0014 0018 0014 0043 0013 0043 0013 0041 0014 0043 0013 0043 "
    "0013 0483";

/** Timer button - controls timer function (not currently used) */
static const std::string TIMER_PRONTO = 
    "0000 006D 0022 0000 0159 00AF 0014 0018 0014 0018 0014 0043 0013 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0043 0013 0043 0013 0018 "
    "0014 0043 0013 0043 0013 0043 0013 0043 0013 0042 0014 0018 0014 0018 "
    "0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0018 0014 0042 "
    "0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 0014 0042 "
    "0014 0483";

/** Repeat frame */
static const std::string REPEAT_PRONTO = "0000 006D 0002 0000 0159 0056 0014 0483";

/** @} */  // End of IR Command Definitions

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
