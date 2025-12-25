#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "woleix_status.h"

using namespace esphome::climate_ir_woleix;

// Test Category Creation and Comparison
TEST(WoleixStatusTest, CategoryCreationAndComparison)
{
    auto category1 = Category::make(1, 2, "TestCategory1");
    auto category2 = Category::make(1, 2, "TestCategory2");
    auto category3 = Category::make(1, 3, "TestCategory3");

    EXPECT_EQ(category1, category2);
    EXPECT_NE(category1, category3);
    EXPECT_EQ(category1.module_id(), 1);
    EXPECT_EQ(category1.local_id(), 2);

    // Test edge cases
    auto max_category = Category::make(65535, 65535, "MaxCategory");
    EXPECT_EQ(max_category.module_id(), 65535);
    EXPECT_EQ(max_category.local_id(), 65535);
}

// Test WoleixStatus Construction and Accessors
TEST(WoleixStatusTest, ConstructionAndAccessors)
{
    auto category = Category::make(1, 2, "TestCategory");
    WoleixStatus status(WoleixStatus::Severity::WX_SEVERITY_ERROR, category, "Test message");

    EXPECT_EQ(status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_ERROR);
    EXPECT_EQ(status.get_category(), category);
    EXPECT_EQ(status.get_message(), "Test message");
}

// Test WoleixStatus Equality
TEST(WoleixStatusTest, Equality)
{
    auto category1 = Category::make(1, 2, "TestCategory1");
    auto category2 = Category::make(1, 3, "TestCategory2");

    WoleixStatus status1(WoleixStatus::Severity::WX_SEVERITY_ERROR, category1, "Message 1");
    WoleixStatus status2(WoleixStatus::Severity::WX_SEVERITY_ERROR, category1, "Message 2");
    WoleixStatus status3(WoleixStatus::Severity::WX_SEVERITY_WARNING, category1, "Message 1");
    WoleixStatus status4(WoleixStatus::Severity::WX_SEVERITY_ERROR, category2, "Message 1");

    EXPECT_EQ(status1, status2);  // Same severity and category, different message
    EXPECT_NE(status1, status3);  // Different severity
    EXPECT_NE(status1, status4);  // Different category
}

// Define new category IDs
namespace NewCategoryId
{
    inline constexpr uint16_t NewModule = 100;
    inline constexpr uint16_t NewLocal1 = 1;
    inline constexpr uint16_t NewLocal2 = 2;
}

// Test Category Extensibility
TEST(WoleixStatusTest, CategoryExtensibility)
{

    auto new_category1 = Category::make(NewCategoryId::NewModule, NewCategoryId::NewLocal1, "NewCategory1");
    auto new_category2 = Category::make(NewCategoryId::NewModule, NewCategoryId::NewLocal2, "NewCategory2");

    WoleixStatus status1(WoleixStatus::Severity::WX_SEVERITY_INFO, new_category1, "New category test 1");
    WoleixStatus status2(WoleixStatus::Severity::WX_SEVERITY_DEBUG, new_category2, "New category test 2");

    EXPECT_NE(status1, status2);
    EXPECT_EQ(status1.get_category().module_id(), NewCategoryId::NewModule);
    EXPECT_EQ(status1.get_category().local_id(), NewCategoryId::NewLocal1);
    EXPECT_EQ(status2.get_category().module_id(), NewCategoryId::NewModule);
    EXPECT_EQ(status2.get_category().local_id(), NewCategoryId::NewLocal2);
}

// Test Severity Enum
TEST(WoleixStatusTest, SeverityEnum)
{
    auto category = Category::make(1, 1, "TestCategory");

    WoleixStatus error_status(WoleixStatus::Severity::WX_SEVERITY_ERROR, category, "Error");
    WoleixStatus warning_status(WoleixStatus::Severity::WX_SEVERITY_WARNING, category, "Warning");
    WoleixStatus info_status(WoleixStatus::Severity::WX_SEVERITY_INFO, category, "Info");
    WoleixStatus debug_status(WoleixStatus::Severity::WX_SEVERITY_DEBUG, category, "Debug");

    EXPECT_EQ(error_status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_ERROR);
    EXPECT_EQ(warning_status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_WARNING);
    EXPECT_EQ(info_status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_INFO);
    EXPECT_EQ(debug_status.get_severity(), WoleixStatus::Severity::WX_SEVERITY_DEBUG);

    EXPECT_NE(error_status, warning_status);
    EXPECT_NE(warning_status, info_status);
    EXPECT_NE(info_status, debug_status);
}

// Mock classes for WoleixListener and WoleixReporter
class MockWoleixStatusObserver : public WoleixStatusObserver
{
public:
    MOCK_METHOD(void, observe, (const WoleixStatusReporter&, const WoleixStatus&), (override));
};

class MockWoleixStatusReporter : public WoleixStatusReporter
{
public:
    MOCK_METHOD(void, register_observer, (WoleixStatusObserver*), (override));
    MOCK_METHOD(void, unregister_observer, (WoleixStatusObserver*), (override));
};

// Test WoleixStatusObserver and WoleixStatusReporter
TEST(WoleixStatusTest, StatusObserverAndStatusReporter)
{
    MockWoleixStatusReporter reporter;
    MockWoleixStatusObserver observer1, observer2;

    EXPECT_CALL(reporter, register_observer(&observer1));
    EXPECT_CALL(reporter, register_observer(&observer2));
    EXPECT_CALL(reporter, unregister_observer(&observer1));

    reporter.register_observer(&observer1);
    reporter.register_observer(&observer2);
    reporter.unregister_observer(&observer1);

    auto category = Category::make(1, 1, "TestCategory");
    WoleixStatus status(WoleixStatus::Severity::WX_SEVERITY_INFO, category, "Test notification");

    EXPECT_CALL(observer2, observe(testing::Ref(reporter), testing::Eq(status)));

    observer2.observe(reporter, status);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
