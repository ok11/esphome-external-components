#pragma once

#include <string>
#include <cstdint>

namespace esphome
{
namespace climate_ir_woleix
{

namespace CategoryId
{
    inline constexpr uint16_t Core = 0;
    inline constexpr uint16_t CommandQueue = 1;
    inline constexpr uint16_t StateMachine = 2;
    inline constexpr uint16_t ProtocolHandler = 3;
}

struct Category
{
    uint32_t value;
    const char* name;
    
    constexpr uint16_t module_id() const { return value >> 16; }
    constexpr uint16_t local_id() const { return value & 0xFFFF; }
    
    static constexpr Category make(uint16_t module, uint16_t local, const char* name)
    {
        return Category{(uint32_t(module) << 16) | local, name};
    }
    
    constexpr bool operator==(Category other) const { return value == other.value; }
};

class WoleixStatus
{
public:
    enum class Severity
    {
        WX_SEVERITY_ERROR,
        WX_SEVERITY_WARNING,
        WX_SEVERITY_INFO,
        WX_SEVERITY_DEBUG
    };

    WoleixStatus(Severity severity, Category category, std::string message)
        : severity_(severity), category_(category), message_(message)
    {}

    const Severity get_severity() const { return severity_; }
    Category get_category() const { return category_; }
    const std::string get_message() const { return message_; }

    bool operator==(const WoleixStatus& other) const
    {
        return severity_ == other.severity_ && category_ == other.category_;
    }
protected:
    Severity severity_;
    Category category_;
    std::string message_;
};

class WoleixStatusObserver;

class WoleixStatusReporter
{
    virtual void register_observer(WoleixStatusObserver* observer) = 0;
    virtual void unregister_observer(WoleixStatusObserver* observer) = 0;
};

class WoleixStatusObserver
{
public:
    virtual void observe(const WoleixStatusReporter& reporter, const WoleixStatus& status) = 0;
};
}
}