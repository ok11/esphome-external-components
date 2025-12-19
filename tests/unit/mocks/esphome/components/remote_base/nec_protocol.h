#pragma once

#include <cinttypes>

namespace esphome {
namespace remote_base {

struct NECData
{
    uint16_t address;
    uint16_t command;
    uint16_t command_repeats;

    bool operator==(const NECData &rhs) const 
    {
      return address == rhs.address && command == rhs.command;
    }
};

class NECProtocol
{
public:
    using ProtocolData = NECData;
};
}  // namespace remote_base
}  // namespace esphome
