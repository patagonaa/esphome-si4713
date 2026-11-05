#pragma once

#include "esphome/core/component.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/number/number.h"

#include <optional>

namespace esphome::si4713 {

class Si4713Component;

class Si4713FrequencyNumber final : public number::Number, public Component, public Parented<Si4713Component> {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

 protected:
  void control(float value) override;
  bool restore_value_{true};

  ESPPreferenceObject pref_;
};

class Si4713PowerNumber final : public number::Number, public Component, public Parented<Si4713Component> {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }
  void set_restore_value(bool restore_value) { this->restore_value_ = restore_value; }

 protected:
  void control(float value) override;
  bool restore_value_{true};

  ESPPreferenceObject pref_;
};

class Si4713Component final : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_preemphasis_mode(uint8_t preemphasis_mode) {
    this->preemphasis_mode_ = preemphasis_mode;
  };
  void set_reference_clock_frequency(uint16_t freq) {
    this->reference_clock_frequency_ = freq;
  };
  void set_reset_pin(GPIOPin *reset_pin) { this->reset_pin_ = reset_pin; }

  void set_tx_frequency(uint16_t freq);
  void set_tx_power(uint8_t power_dbuv);
  bool tune();

 protected:
  void reset_device_();
  bool write_and_wait_(const uint8_t* data, size_t data_length, uint8_t* response, size_t response_length);
  bool power_up_();
  bool set_property_(uint16_t property, uint16_t value);
  bool get_rev_(uint8_t *part_number);
  bool get_int_status_(uint8_t *status);
  bool tune_freq_(uint16_t freq);
  bool tune_pwr_(uint8_t power_dbuv);
  bool measure_noise_(uint16_t freq, uint8_t *noise_dbuv);
  bool set_rds_radio_text_(const char *radio_text);
  bool set_rds_station_name_(const char *station_name);
  bool enable_rds_(uint8_t pty);

  uint8_t preemphasis_mode_;
  uint16_t reference_clock_frequency_;

  bool tune_in_progress_{false};

  uint16_t tx_frequency_{0};
  uint8_t tx_power_{0};

  GPIOPin *reset_pin_{nullptr};
};

} // namespace esphome::si4713