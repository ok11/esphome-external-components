#pragma once

#include <gmock/gmock.h>
#include <vector>

namespace esphome {
namespace remote_base {

// Mock RemoteTransmitData
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

// Mock RemoteTransmitter
class RemoteTransmitter {
public:
  class TransmitCall {
  public:
    explicit TransmitCall(RemoteTransmitter* parent) : parent_(parent) {}
    
    RemoteTransmitData* get_data() { return &data_; }
    
    void perform();
    
  private:
    RemoteTransmitter* parent_;
    RemoteTransmitData data_;
  };
  
  virtual ~RemoteTransmitter() = default;
  
  TransmitCall transmit() { return TransmitCall(this); }
  
  // Virtual method for GMock
  virtual void transmit_raw(const RemoteTransmitData& data) {
    if (on_transmit_) {
      on_transmit_(data);
    }
  }
  
  // For testing: set a callback to capture transmitted data
  void set_transmit_callback(std::function<void(const RemoteTransmitData&)> callback) {
    on_transmit_ = callback;
  }
  
private:
  std::function<void(const RemoteTransmitData&)> on_transmit_;
};

// TransmitCall::perform implementation (needs to be after RemoteTransmitter definition)
inline void RemoteTransmitter::TransmitCall::perform() {
  parent_->transmit_raw(data_);
}


} // namespace remote_base
} // namespace esphome