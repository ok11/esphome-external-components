#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <stdexcept>

#include "../../esphome/components/climate_ir_woleix/woleix_command.h"
#include "../../esphome/components/climate_ir_woleix/woleix_constants.h"
#include "../../esphome/components/climate_ir_woleix/woleix_management.h"

using namespace esphome::climate_ir_woleix;
using ::testing::_;
using ::testing::AtLeast;

class WoleixCommandQueueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        queue = std::make_unique<WoleixCommandQueue>(10);  // Max capacity of 10
    }

    void TearDown() override
    {
        queue.reset();
    }

    std::unique_ptr<WoleixCommandQueue> queue;
};

// Constructor test
TEST_F(WoleixCommandQueueTest, ConstructorSetsMaxCapacity)
{
    EXPECT_EQ(queue->max_capacity(), 10);
}

// Enqueue operation tests
TEST_F(WoleixCommandQueueTest, EnqueueSingleCommand)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    queue->enqueue(cmd);
    EXPECT_EQ(queue->length(), 1);
    EXPECT_FALSE(queue->is_empty());
}

TEST_F(WoleixCommandQueueTest, EnqueueMultipleCommands)
{
    for (int i = 0; i < 5; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        queue->enqueue(cmd);
    }
    EXPECT_EQ(queue->length(), 5);
}

// Dequeue operation tests
TEST_F(WoleixCommandQueueTest, DequeueFromNonEmptyQueue)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    queue->enqueue(cmd);
    
    const WoleixCommand& dequeued = queue->dequeue();
    EXPECT_EQ(dequeued.get_type(), WoleixCommand::Type::POWER);
    EXPECT_TRUE(queue->is_empty());
}

TEST_F(WoleixCommandQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_TRUE(queue->is_empty());
    EXPECT_THROW
    (
        {
            WoleixCommand cmd = queue->dequeue();
            (void)cmd;  // Suppress unused variable warning
        },
        std::out_of_range
    );
}

// Reset operation test
TEST_F(WoleixCommandQueueTest, ResetClearsQueue)
{
    for (int i = 0; i < 5; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        queue->enqueue(cmd);
    }
    ASSERT_EQ(queue->length(), 5);
    queue->reset();
    EXPECT_TRUE(queue->is_empty());
    EXPECT_EQ(queue->length(), 0);
    EXPECT_THROW
    (
        {
            WoleixCommand cmd = queue->dequeue();
            (void)cmd;  // Suppress unused variable warning
        },
        std::out_of_range
    );
}

// Get_command operation tests
TEST_F(WoleixCommandQueueTest, GetCommandAtValidIndex)
{
    WoleixCommand cmd1(WoleixCommand::Type::POWER, 0xFB04);
    WoleixCommand cmd2(WoleixCommand::Type::TEMP_UP, 0xFB04);
    queue->enqueue(cmd1);
    queue->enqueue(cmd2);

    EXPECT_EQ(queue->get_command(0).get_type(), WoleixCommand::Type::POWER);
    EXPECT_EQ(queue->get_command(1).get_type(), WoleixCommand::Type::TEMP_UP);
}

TEST_F(WoleixCommandQueueTest, GetCommandAtInvalidIndex)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    queue->enqueue(cmd);

    EXPECT_THROW(queue->get_command(1), std::out_of_range);
}

// Listener notification tests
class MockWoleixListener : public WoleixCommandQueueListener
{
public:
    MOCK_METHOD(void, notify, (const WoleixReporter& reporter, const WoleixStatus& status), (override));
    MOCK_METHOD(void, hold, (), (override));
    MOCK_METHOD(void, resume, (), (override));
};

TEST_F(WoleixCommandQueueTest, ListenersNotifiedWhenQueueBecomesFull)
{
    auto listener = std::make_shared<MockWoleixListener>();
    queue->register_listener(listener);

    EXPECT_CALL(*listener, notify(_, WX_STATUS_QUEUE_FULL)).Times(1);

    // Enqueue 9 commands (90% of max_capacity)
    for (int i = 0; i < 9; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        queue->enqueue(cmd);
    }

    // Enqueue one more command to trigger the notification
    WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
    queue->enqueue(cmd);

    // Verify that the listener was notified
    testing::Mock::VerifyAndClearExpectations(listener.get());
}

TEST_F(WoleixCommandQueueTest, ListenersNotifiedWhenQueueBecomesEmpty)
{
    auto listener = std::make_shared<MockWoleixListener>();
    queue->register_listener(listener);

    // Enqueue 3 commands
    for (int i = 0; i < 3; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        queue->enqueue(cmd);
    }

    EXPECT_CALL(*listener, notify(_, WX_STATUS_QUEUE_EMPTY)).Times(1);

    // Dequeue 3 commands
    for (int i = 0; i < 3; ++i)
    {
        queue->dequeue();
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
