#pragma once

namespace esphome {
namespace sensor {

// Mock Sensor class for testing
class Sensor {
public:
    float state{0.0f};
    
    template<typename F>
    void add_on_state_callback(F &&callback) {
        // Mock implementation - does nothing in tests
    }
};

}  // namespace sensor
}  // namespace esphome
