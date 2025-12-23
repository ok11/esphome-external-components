#pragma once

#include <string>
#include <cstdint>

namespace esphome {
namespace climate_ir_woleix {

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

    enum class Category
    {
        WX_CAT_QUEUE_FULL,
        WX_CAT_QUEUE_EMPTY,
        WX_CAT_TRANSMISSION_FAILED,
        WX_CAT_OTHERS
    };

    WoleixStatus(Severity severity, Category category, std::string message)
        : severity_(severity), category_(category), message_(message)
    {}

    const Severity get_severity() const { return severity_; }
    const Category get_category() const { return category_; }
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

class WoleixListener;

class WoleixReporter
{
    virtual void register_listener(std::shared_ptr<WoleixListener> listener) = 0;
    virtual void unregister_listener(std::shared_ptr<WoleixListener> listener) = 0;
};

class WoleixListener
{
public:
    virtual void notify(const WoleixReporter& reporter, const WoleixStatus& status) = 0;
};
}
}