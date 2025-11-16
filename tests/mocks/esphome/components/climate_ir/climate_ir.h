#pragma once

#include <gmock/gmock.h>
#include <vector>
#include <memory>
#include "esphome/core/optional.h"
#include "esphome/components/climate/climate_mode.h"

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
    
    void perform() {
      if (parent_->on_transmit_) {
        parent_->on_transmit_(data_);
      }
    }
    
  private:
    RemoteTransmitter* parent_;
    RemoteTransmitData data_;
  };
  
  TransmitCall transmit() { return TransmitCall(this); }
  
  // For testing: set a callback to capture transmitted data
  void set_transmit_callback(std::function<void(const RemoteTransmitData&)> callback) {
    on_transmit_ = callback;
  }
  
private:
  std::function<void(const RemoteTransmitData&)> on_transmit_;
};

} // namespace remote_base

namespace climate_ir {

using esphome::climate::ClimateMode;
using esphome::climate::ClimateFanMode;
using esphome::climate::ClimateSwingMode;
using esphome::climate::ClimateTraits;

// Mock ClimateIR base class
class ClimateIR {
public:
  ClimateIR(float min_temp, float max_temp)
    : temperature_step_(1.0f),
      min_temperature_(min_temp),
      max_temperature_(max_temp) {}
  
  virtual ~ClimateIR() = default;
  
  // Public state that child classes use
  ClimateMode mode = ClimateMode::CLIMATE_MODE_OFF;
  float target_temperature = 20.0f;
  optional<ClimateFanMode> fan_mode;
  optional<ClimateSwingMode> swing_mode;
  
  // Setter for transmitter (used by tests)
  void set_transmitter(remote_base::RemoteTransmitter* transmitter) {
    transmitter_ = transmitter;
  }
  
  // Call transmit_state from tests
  void call_transmit_state() {
    transmit_state();
  }
  
  // Call traits from tests
  ClimateTraits call_traits() {
    return traits();
  }
  
protected:
  // Override these in derived classes
  virtual void transmit_state() = 0;
  virtual ClimateTraits traits() = 0;
  
  // Protected members accessible to derived classes
  remote_base::RemoteTransmitter* transmitter_ = nullptr;
  float temperature_step_;
  float min_temperature_;
  float max_temperature_;
};

} // namespace climate_ir
} // namespace esphome
