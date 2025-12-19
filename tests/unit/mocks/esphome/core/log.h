#pragma once

#include <string>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

// Mock ESP logging macros
#define ESP_LOGD(tag, format, ...) printf("[D][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) printf("[I][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) printf("[W][%s] " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...) printf("[E][%s] " format "\n", tag, ##__VA_ARGS__)

// Mock delay function (in global namespace, not esphome namespace)
inline void delay(uint32_t ms)
{
    // In tests, we don't actually delay
}

namespace esphome {
    // Empty namespace for esphome core
} // namespace esphome
