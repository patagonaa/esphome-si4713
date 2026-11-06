#include "si4713.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include "si4713_defines.h"

namespace esphome::si4713 {

ESPHOME_LOG_TAG(TAG, "si4713");

void Si4713FrequencyNumber::setup() {
  ESP_LOGD(TAG, "FreqNumber setup");
  float value;
  if (this->restore_value_) {
    this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
    if (this->pref_.load(&value)) {
      this->control(value);
    }
  }
}

void Si4713FrequencyNumber::control(float value) {
  this->parent_->set_tx_frequency((uint16_t)(value * 100.0f));
  this->parent_->tune();
  this->publish_state(value);

  if (this->restore_value_)
    this->pref_.save(&value);
}

void Si4713PowerNumber::setup() {
  float value;
  if (this->restore_value_) {
    this->pref_ = global_preferences->make_preference<float>(this->get_object_id_hash());
    if (this->pref_.load(&value)) {
      this->control(value);
    }
  }
}

void Si4713PowerNumber::control(float value) {
  this->parent_->set_tx_power((uint8_t)value);
  this->parent_->tune();

  this->publish_state(value);

  if (this->restore_value_)
    this->pref_.save(&value);
}

void Si4713OutputEnableSwitch::setup() {
  auto initial_state = this->get_initial_state_with_restore_mode();
  if (initial_state.has_value()) {
    this->write_state(initial_state.value());
  }
}

void Si4713OutputEnableSwitch::write_state(bool state) {
  this->parent_->set_tx_output_enable(state);
  this->parent_->tune();

  this->publish_state(state);
}

void Si4713Component::setup() {
  this->reset_pin_->setup();
  this->reset_device_();

  if (!this->power_up_()) {
    this->mark_failed(LOG_STR("failed to power up"));
    return;
  }

  if (!this->set_property_(SI47XX_PROP_TX_PREEMPHASIS, this->preemphasis_mode_)){
    this->mark_failed(LOG_STR("failed to set property"));
    return;
  }

  if (!this->set_property_(SI47XX_PROP_REFCLK_FREQ, this->reference_clock_frequency_)){
    this->mark_failed(LOG_STR("failed to set property"));
    return;
  }

  ESP_LOGD(TAG, "Set properties");

  uint8_t part_number;
  if (!this->get_rev_(&part_number)) {
    this->mark_failed(LOG_STR("failed to get part number"));
    return;
  }
  ESP_LOGD(TAG, "Part number: %" PRIu8, part_number);
  if (part_number != 13) {
    this->mark_failed(LOG_STR("wrong part number returned"));
    return;
  }

  if (this->rds_enabled_) {
    this->set_rds_station_name_(this->rds_station_name_.c_str());
    this->set_rds_radio_text_(this->rds_text_.c_str());
    this->enable_rds_(this->rds_pty_);
  }
}

void Si4713Component::loop() {
  if (this->tune_in_progress_) {
    uint8_t status;
    if (!this->get_int_status_(&status)) {
      this->mark_failed(LOG_STR("failed to get status!"));
      return;
    }
    constexpr uint8_t status_ready_flags = SI47XX_STATUS_CTS | SI47XX_STATUS_STCINT;
    if ((status & status_ready_flags) == status_ready_flags) {
      ESP_LOGI(TAG, "tune complete!");
      this->tune_in_progress_ = false;
    }
  }
}

void Si4713Component::dump_config() {
  ESP_LOGCONFIG(TAG,
                "SI4713:\n"
                "  Preemphasis Mode: 0x%x",
                this->preemphasis_mode_);
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
}

void Si4713Component::set_tx_frequency(uint16_t freq) {
  this->tx_frequency_ = freq;
}

void Si4713Component::set_tx_power(uint8_t power_dbuv) {
  this->tx_power_ = power_dbuv;
}

void Si4713Component::set_tx_output_enable(bool enable) {
  this->tx_output_enable_ = enable;
}

bool Si4713Component::tune() {
  if (this->tx_output_enable_) {
    if (this->tx_frequency_ == 0 || this->tx_power_ == 0) {
      return false;
    }
    if (!this->tune_freq_(this->tx_frequency_))
      return false;
    if (!this->tune_pwr_(this->tx_power_))
      return false;
  } else {
    // HACK: after boot, if we _only_ call "tune_pwr_(0)", the STC interrupt gets reset but never set,
    // causing an endless loop since tune_in_progress_ is true.
    // after output has been on once, this isn't an issue anymore, but it doesn't hurt to always do both.
    if (!this->tune_freq_(this->tx_frequency_))
      return false;
    if (!this->tune_pwr_(0))
      return false;
  }
  return true;
}

void Si4713Component::reset_device_() {
  this->reset_pin_->digital_write(false);
  delay(1);
  this->reset_pin_->digital_write(true);
  delay(1);
}

bool Si4713Component::write_and_wait_(const uint8_t *data, size_t data_length, uint8_t* response, size_t response_length) {
  if (this->bus_->write_readv(this->address_, data, data_length, nullptr, 0) != i2c::ErrorCode::NO_ERROR) {
    return false;
  }

  // address in + data out should be 21 clock cycles, 2400 * (21/400kHz) = 126ms (power up may take up to 110ms)
  constexpr size_t timeout_tries = 2400;
  for (size_t i = 0; i < timeout_tries; i++) {
    uint8_t status = 0;
    if (this->bus_->write_readv(this->address_, nullptr, 0, &status, 1) != i2c::ErrorCode::NO_ERROR) {
      return false;
    }

    if (status & SI47XX_STATUS_ERR) {
      return false;
    }

    if (status & SI47XX_STATUS_CTS) {
      if (response_length > 0) {
        if (this->bus_->write_readv(this->address_, nullptr, 0, response, response_length) != i2c::ErrorCode::NO_ERROR) {
          return false;
        }
      }
      
      return true;
    }
  }

  // timeout
  return false;
}

bool Si4713Component::power_up_() {
  // App Note AN332, page 12

  ESP_LOGD(TAG, "Trying to power up");
  const uint8_t data[] = {
    SI47XX_CMD_POWER_UP,
    0x12, // No CTS interrupt, no GPIO2, no patch, enable crystal oscillator, function transmit
    0x50  // use analog audio inputs
  };
  if (!this->write_and_wait_(data, 3, nullptr, 0)) {
    return false;
  }
  ESP_LOGD(TAG, "Powerup successful");
  return true;
}

bool Si4713Component::set_property_(uint16_t property, uint16_t value) {
  const uint8_t data[] = {
    SI47XX_CMD_SET_PROPERTY, 0, 
    (uint8_t)(property >> 8), (uint8_t)(property & 0xFF),
    (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)
  };
  return this->write_and_wait_(data, 6, nullptr, 0);
}

bool Si4713Component::get_rev_(uint8_t* part_number) {
  uint8_t read_buf[9];

  uint8_t cmd = SI47XX_CMD_GET_REV;
  this->write_and_wait_(&cmd, 1, read_buf, 9);

  *part_number = read_buf[1];
  return true;
}

bool Si4713Component::get_int_status_(uint8_t* status) {
  if (this->read_register(SI47XX_CMD_GET_INT_STATUS, status, 1) != i2c::ErrorCode::ERROR_OK) {
    return false;
  }
  return true;
}

bool Si4713Component::tune_freq_(uint16_t freq) {
  this->tune_in_progress_ = true;

  const uint8_t freq_data[] = {
    SI47XX_CMD_TX_TUNE_FREQ, 0,
    (uint8_t)(freq >> 8), (uint8_t)(freq & 0xFF)
  };
  if(!this->write_and_wait_(freq_data, 4, nullptr, 0)) {
    return false;
  }
  ESP_LOGI(TAG, "Tuning to %.2f MHz", freq / 100.0f);
  return true;
}

bool Si4713Component::tune_pwr_(uint8_t power_dbuv) {
  this->tune_in_progress_ = true;

  constexpr uint8_t ant_cap = 0;
  const uint8_t pwr_data[] = {
    SI47XX_CMD_TX_TUNE_POWER,
    0, 0,
    power_dbuv,
    ant_cap
  };
  if (!this->write_and_wait_(pwr_data, 5, nullptr, 0)) {
    return false;
  }

  ESP_LOGI(TAG, "Setting TX power to %" PRIu8 " dBuV", power_dbuv);

  return true;
}

bool Si4713Component::measure_noise_(uint16_t freq, uint8_t *noise_dbuv) {
  // Start measurment on frequency
  ESP_LOGD(TAG, "Starting noise measurment");
  constexpr uint8_t ant_cap = 0;
  const uint8_t measure_data[] = {
    SI47XX_CMD_TX_TUNE_MEASURE, 0, 
    (uint8_t)(freq >> 8), (uint8_t)(freq & 0xFF),
    ant_cap
  };
  if (!this->write_and_wait_(measure_data, 5, nullptr, 0)) {
    return false;
  }

  // wait until ready
  constexpr uint8_t status_ready_flags = SI47XX_STATUS_CTS | SI47XX_STATUS_STCINT;
  for (size_t i = 0; i < 100; i++) { // ~100ms timeout
    uint8_t status;
    if (!this->get_int_status_(&status)) {
      return false;
    }
    if ((status & status_ready_flags) == status_ready_flags) {
      break;
    }
    delay(1);
  }

  ESP_LOGD(TAG, "Noise measurment finished!");

  // get tune status
  const uint8_t tune_status_req[] = {SI47XX_CMD_TX_TUNE_STATUS, 0x01};
  uint8_t tune_status_res[8];
  if (this->bus_->write_readv(this->address_, tune_status_req, 2, tune_status_res, 8) != i2c::ErrorCode::ERROR_OK) {
    return false;
  }

  *noise_dbuv = tune_status_res[7];
  return true;
}

bool Si4713Component::set_rds_radio_text_(const char *radio_text) {
  uint8_t request[8];
  request[0] = SI47XX_CMD_TX_RDS_BUFF;

  uint8_t group = 0x20; // Group 2A

  size_t text_len = strlen(radio_text);
  size_t text_buf_len = text_len == 64 ? text_len : (text_len + 1); // terminating "\r"
  size_t pages = (text_buf_len % 4) == 0 ? (text_buf_len / 4) : ((text_buf_len / 4) + 1);

  // we could check the TX_RDS_BUFF response for available space, but 53 is the guaranteed minimum length which should be enough.
  if (pages > 53) {
    ESP_LOGW(TAG, "Radio text is too long!");
    return false;
  }

  for (size_t page = 0; page < pages; page++) {
    request[1] = page == 0 ? 0x06 : 0x04; // load buffer, clear buffer on first page
    request[2] = group;
    request[3] = page;

    for (size_t j = 0; j < 4; j++) {
      size_t idx = page * 4 + j;
      request[4 + j] = (idx >= text_len) ? '\r' : radio_text[idx];
    }

    if (!this->write_and_wait_(request, 8, nullptr, 0))
      return false;
  }

  return true;
}

bool Si4713Component::set_rds_station_name_(const char *station_name) {
  uint8_t request[6];
  request[0] = SI47XX_CMD_TX_RDS_PS;

  size_t station_len = strlen(station_name);
  size_t messages = (station_len % 8) == 0 ? (station_len / 8) : ((station_len / 8) + 1);
  size_t pages = messages * 2;

  for (size_t page = 0; page < pages; page++) {
    request[1] = page;
    for (size_t j = 0; j < 4; j++) {
      size_t idx = page * 4 + j;
      request[2 + j] = (idx >= station_len) ? ' ' : station_name[idx];
    }

    if (!this->write_and_wait_(request, 6, nullptr, 0))
      return false;
  }

  if (!this->set_property_(SI47XX_PROP_TX_RDS_PS_MESSAGE_COUNT, messages))
    return false;

  return true;
}

bool Si4713Component::enable_rds_(uint8_t pty) {
  if (!this->set_property_(SI47XX_PROP_TX_RDS_PS_MISC, SI47XX_TX_RDS_PS_MISC_RDSMS | SI47XX_TX_RDS_PS_MISC_RDSD3 | SI47XX_TX_RDS_PS_MISC_FORCEB | ((pty & 0x1F) << 5)))
    return false;

  if (!this->set_property_(SI47XX_PROP_TX_COMPONENT_ENABLE, SI47XX_TX_COMPONENT_ENABLE_LR | SI47XX_TX_COMPONENT_ENABLE_PILOT | SI47XX_TX_COMPONENT_ENABLE_RDS))
    return false;

  ESP_LOGI(TAG, "RDS set up!");

  return true;
}

} // namespace esphome::si4713
