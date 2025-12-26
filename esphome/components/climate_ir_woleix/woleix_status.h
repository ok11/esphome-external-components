#pragma once

#include <string>
#include <cstdint>

namespace esphome
{
namespace climate_ir_woleix
{

/**
 * @brief Namespace containing category IDs for different components of the Woleix system.
 */
namespace CategoryId
{
    inline constexpr uint16_t Core = 0;             ///< Core component category
    inline constexpr uint16_t CommandQueue = 1;     ///< Command queue category
    inline constexpr uint16_t StateManager = 2;     ///< State manager category
    inline constexpr uint16_t ProtocolHandler = 3;  ///< Protocol handler category
}

/**
 * @brief Represents a category for status messages.
 */
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

/**
 * @brief Represents a status message in the Woleix system.
 */
class WoleixStatus
{
public:
    /**
     * @brief Enumeration of severity levels for status messages.
     */
    enum class Severity
    {
        WX_SEVERITY_ERROR,    ///< Error severity
        WX_SEVERITY_WARNING,  ///< Warning severity
        WX_SEVERITY_INFO,     ///< Information severity
        WX_SEVERITY_DEBUG     ///< Debug severity
    };

    /**
     * @brief Constructs a WoleixStatus object.
     * @param severity The severity level of the status.
     * @param category The category of the status.
     * @param message The message content of the status.
     */
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

class WoleixStatusReporter;

/**
 * @brief Interface for objects that observe status changes.
 */
class WoleixStatusObserver
{
public:
    /**
     * @brief Method called when a new status is reported.
     * @param reporter The reporter that generated the status.
     * @param status The new status that was reported.
     */
    virtual void observe(const WoleixStatusReporter& reporter, const WoleixStatus& status) = 0;
};

/**
 * @brief Class responsible for reporting status changes to observers.
 */
class WoleixStatusReporter
{
public:
    virtual void register_observer(WoleixStatusObserver* observer)
    {
        observers_.push_back(observer);
    }
    virtual void unregister_observer(WoleixStatusObserver* observer)
    {
        std::erase(observers_, observer);
    }

    virtual void report(const WoleixStatus& status)
    {
        for (auto observer : observers_)
        {
            observer->observe(*this, status);
        }
    } 
protected:
    std::vector<WoleixStatusObserver*> observers_;
};

}
}
