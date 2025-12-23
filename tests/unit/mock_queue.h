#pragma once

#include "woleix_command.h"

using esphome::climate_ir_woleix::WoleixCommand;
using esphome::climate_ir_woleix::WoleixCommandQueue;

class MockWoleixCommandQueue: public WoleixCommandQueue
{
public:
    MockWoleixCommandQueue() : WoleixCommandQueue(16) {}
    // Helper to count specific commands in the queue and sum their repeat counts
    int count_command(WoleixCommand::Type type)
    {
        return std::accumulate(queue_->begin(), queue_->end(), 0,
            [this, type](int sum, const WoleixCommand& cmd)
            {
                return sum + (cmd.get_type() == type ? cmd.get_repeat_count() : 0);
            }
        );
    }

};
