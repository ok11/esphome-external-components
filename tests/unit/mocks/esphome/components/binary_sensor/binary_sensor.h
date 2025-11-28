#include <vector>
#include <functional>

namespace esphome {
namespace binary_sensor {

// Mock BinarySensor class
class BinarySensor {
public:
    bool state{false};
    
    template<typename F>
    void add_on_state_callback(F &&callback) {
        callbacks_.push_back(std::forward<F>(callback));
    }
    
    // Method to trigger all registered callbacks with a new state value
    void publish_state(bool new_state) {
        state = new_state;
        for (auto& callback : callbacks_) {
            callback(new_state);
        }
    }

private:
    std::vector<std::function<void(bool)>> callbacks_;
};

}  // namespace binary_sensor
}  // namespace esphome

