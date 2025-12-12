#pragma once

#include <vector>

#include "pronto_protocol.h"
#include "nec_protocol.h"

namespace esphome {
namespace remote_base {

class RemoteTransmitData {
public:
  void set_carrier_frequency(uint32_t freq) { carrier_frequency_ = freq; }
  void mark(uint32_t us) { data_.push_back(us); }
  void space(uint32_t us) { data_.push_back(us); }
  
  uint32_t get_carrier_frequency() const { return carrier_frequency_; }
  const std::vector<uint32_t>& get_data() const { return data_; }
  
private:
  uint32_t carrier_frequency_ = 0;
  std::vector<uint32_t> data_;
};


// Mock RemoteTransmitterBase
class RemoteTransmitterBase
{
public:
  virtual ~RemoteTransmitterBase() = default;
  
  template<typename Protocol>
  void transmit(const typename Protocol::ProtocolData &data, uint32_t send_times, uint32_t send_wait);
  virtual void send_(const ProntoProtocol::ProtocolData& data, uint32_t send_times, uint32_t send_wait)
  {
    // Mock implementation
  }
  virtual void send_(const NECProtocol::ProtocolData& data, uint32_t send_times, uint32_t send_wait)
  {
    // Mock implementation
  }
};


template<>
inline void RemoteTransmitterBase::transmit<ProntoProtocol>(const ProntoProtocol::ProtocolData &data, uint32_t send_times, uint32_t send_wait)
{
  send_(data, send_times, send_wait);
}

template<>
inline void RemoteTransmitterBase::transmit<NECProtocol>(const NECProtocol::ProtocolData &data, uint32_t send_times, uint32_t send_wait)
{
  send_(data, send_times, send_wait);
}

// Mock class that can be used with GMock
class RemoteTransmittable
{
 public:
  RemoteTransmittable() {}
  RemoteTransmittable(RemoteTransmitterBase *transmitter) : transmitter_(transmitter) {}
  void set_transmitter(RemoteTransmitterBase *transmitter) { this->transmitter_ = transmitter; }

 protected:
  template<typename Protocol>
  void transmit_(const typename Protocol::ProtocolData &data, uint32_t send_times = 1, uint32_t send_wait = 0)
  {
    this->transmitter_->transmit<Protocol>(data, send_times, send_wait);
  }
  RemoteTransmitterBase *transmitter_;
};


} // namespace remote_base
} // namespace esphome
