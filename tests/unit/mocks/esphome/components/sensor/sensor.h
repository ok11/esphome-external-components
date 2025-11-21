#pragma once

#include <functional>
#include <vector>

namespace esphome {
namespace sensor {

// Mock Sensor class for testing
class Sensor {
public:
    float state{0.0f};
    
    template<typename F>
    void add_on_state_callback(F &&callback) {
        callbacks_.push_back(std::forward<F>(callback));
    }
    
    // Method to trigger all registered callbacks with a new state value
    void publish_state(float new_state) {
        state = new_state;
        for (auto& callback : callbacks_) {
            callback(new_state);
        }
    }

private:
    std::vector<std::function<void(float)>> callbacks_;
};

}  // namespace sensor
}
