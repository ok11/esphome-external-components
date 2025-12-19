#pragma once

#include <string>

namespace esphome {
namespace remote_base {

struct ProntoData
{
    std::string data;
    int delta;

    bool operator==(const ProntoData &rhs) const
    {
      return data == rhs.data && delta == rhs.delta;
    }
};

class ProntoProtocol
{
public:
    using ProtocolData = ProntoData;
};

}  // namespace remote_base
}  // namespace esphome

