#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <stdexcept>

#include "woleix_constants.h"
#include "woleix_command.h"
#include "woleix_status.h"

#include "mock_queue.h"

using namespace esphome::climate_ir_woleix;
using ::testing::_;
using ::testing::AtLeast;

// Producer notification tests
class MockWoleixCommandQueueProducer : public WoleixCommandQueueProducer
{
public:
    MOCK_METHOD(void, stop_enqueing, (), (override));
    MOCK_METHOD(void, hold_enqueing, (), (override));
    MOCK_METHOD(void, resume_enqueing, (), (override));
};

// Consumer notification tests
class MockWoleixCommandQueueConsumer : public WoleixCommandQueueConsumer
{
public:
    MOCK_METHOD(void, start_processing, (), (override));
};


class WoleixCommandQueueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mock_queue = new MockWoleixCommandQueue();  // Max capacity of 16
        mock_consumer = new MockWoleixCommandQueueConsumer();
        mock_producer = new MockWoleixCommandQueueProducer();

        mock_queue->register_consumer(mock_consumer);
        mock_queue->register_producer(mock_producer);
    }

    void TearDown() override
    {
        mock_queue->unregister_consumer(mock_consumer);
        mock_queue->unregister_producer(mock_producer);

        delete mock_consumer;
        delete mock_producer;
        delete mock_queue;
    }

    MockWoleixCommandQueueConsumer* mock_consumer;
    MockWoleixCommandQueueProducer* mock_producer;
    MockWoleixCommandQueue* mock_queue;
};

// Constructor test
TEST_F(WoleixCommandQueueTest, ConstructorSetsMaxCapacity)
{
    EXPECT_EQ(mock_queue->max_capacity(), 16);
}

// Enqueue operation tests
TEST_F(WoleixCommandQueueTest, EnqueueSingleCommand)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    mock_queue->enqueue(cmd);
    EXPECT_EQ(mock_queue->length(), 1);
    EXPECT_FALSE(mock_queue->is_empty());
}

TEST_F(WoleixCommandQueueTest, EnqueueMultipleCommands)
{
    for (int i = 0; i < 5; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        mock_queue->enqueue(cmd);
    }
    EXPECT_EQ(mock_queue->length(), 5);
}

// Dequeue operation tests
TEST_F(WoleixCommandQueueTest, DequeueFromNonEmptyQueue)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    mock_queue->enqueue(cmd);
    
    mock_queue->dequeue();
    EXPECT_TRUE(mock_queue->is_empty());
}

TEST_F(WoleixCommandQueueTest, DequeueFromEmptyQueue)
{
    ASSERT_TRUE(mock_queue->is_empty());
    EXPECT_THROW
    (
        mock_queue->dequeue(),
        std::out_of_range
    );
}

TEST_F(WoleixCommandQueueTest, GetCommandFromEmptyQueue)
{
    ASSERT_TRUE(mock_queue->is_empty());
    EXPECT_THROW
    (
        mock_queue->next(),
        std::out_of_range
    );
}

// Reset operation test
TEST_F(WoleixCommandQueueTest, ResetClearsQueue)
{
    for (int i = 0; i < 5; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        mock_queue->enqueue(cmd);
    }
    ASSERT_EQ(mock_queue->length(), 5);
    mock_queue->reset();
    EXPECT_TRUE(mock_queue->is_empty());
    EXPECT_EQ(mock_queue->length(), 0);
}

// Get_command operation tests
TEST_F(WoleixCommandQueueTest, GetCommandAtValidIndex)
{
    WoleixCommand cmd1(WoleixCommand::Type::POWER, 0xFB04);
    WoleixCommand cmd2(WoleixCommand::Type::TEMP_UP, 0xFB04);
    mock_queue->enqueue(cmd1);
    mock_queue->enqueue(cmd2);

    EXPECT_EQ(mock_queue->get_command(0).get_type(), WoleixCommand::Type::POWER);
    EXPECT_EQ(mock_queue->get_command(1).get_type(), WoleixCommand::Type::TEMP_UP);
}

TEST_F(WoleixCommandQueueTest, GetCommandAtInvalidIndex)
{
    WoleixCommand cmd(WoleixCommand::Type::POWER, 0xFB04);
    mock_queue->enqueue(cmd);

    EXPECT_THROW(mock_queue->get_command(1), std::out_of_range);
}

TEST_F(WoleixCommandQueueTest, ProducersNotifiedWhenQueueAtHighWatermark)
{
    EXPECT_CALL(*mock_producer, hold_enqueing()).Times(2);

    // Enqueue 14 commands (78% of max_capacity)
    for (int i = 0; i < 14; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        mock_queue->enqueue(cmd);
    }

    // Enqueue one more command to trigger the notification
    WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
    mock_queue->enqueue(cmd);

    // Verify that the listener was notified
    testing::Mock::VerifyAndClearExpectations(mock_producer);

}

TEST_F(WoleixCommandQueueTest, ProducersNotifiedWhenQueueAtLowWatermark)
{
    // Enqueue 3 commands
    for (int i = 0; i < 3; ++i)
    {
        WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);
        mock_queue->enqueue(cmd);
    }

    EXPECT_CALL(*mock_producer, resume_enqueing()).Times(3);

    // Dequeue 3 commands
    for (int i = 0; i < 3; ++i)
    {
        mock_queue->dequeue();
    }
}

TEST_F(WoleixCommandQueueTest, ConsumersNotifiedOnEnquedCommand)
{
    WoleixCommand cmd(WoleixCommand::Type::TEMP_UP, 0xFB04);

    EXPECT_CALL(*mock_consumer, start_processing()).Times(1);

    mock_queue->enqueue(cmd);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
