namespace esphome {

#include <string>
#include <functional>
#include <cstdint>

class Component
{
public:
    void set_timeout(const std::string &name, uint32_t timeout, std::function<void()> &&f);  // NOLINT
    bool cancel_timeout(const std::string &name);  // NOLINT
};
} 