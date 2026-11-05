#pragma once

namespace esphome::si4713 {

  static constexpr uint8_t SI47XX_CMD_POWER_UP = 0x01;
  static constexpr uint8_t SI47XX_CMD_GET_REV = 0x10;
  static constexpr uint8_t SI47XX_CMD_SET_PROPERTY = 0x12;
  static constexpr uint8_t SI47XX_CMD_GET_INT_STATUS = 0x14;
  static constexpr uint8_t SI47XX_CMD_TX_TUNE_FREQ = 0x30;
  static constexpr uint8_t SI47XX_CMD_TX_TUNE_POWER = 0x31;
  static constexpr uint8_t SI47XX_CMD_TX_TUNE_MEASURE = 0x32;
  static constexpr uint8_t SI47XX_CMD_TX_TUNE_STATUS = 0x33;
  static constexpr uint8_t SI47XX_CMD_TX_ASQ_STATUS = 0x34;
  static constexpr uint8_t SI47XX_CMD_TX_RDS_BUFF = 0x35;
  static constexpr uint8_t SI47XX_CMD_TX_RDS_PS = 0x36;

  static constexpr uint8_t SI47XX_STATUS_CTS = 0x80;
  static constexpr uint8_t SI47XX_STATUS_ERR = 0x40;
  static constexpr uint8_t SI47XX_STATUS_RDSINT = 0x04;
  static constexpr uint8_t SI47XX_STATUS_ASQINT = 0x02;
  static constexpr uint8_t SI47XX_STATUS_STCINT = 0x01;
  
  static constexpr uint16_t SI47XX_PROP_TX_COMPONENT_ENABLE = 0x2100;
  static constexpr uint16_t SI47XX_PROP_REFCLK_FREQ = 0x0201;
  static constexpr uint16_t SI47XX_PROP_TX_PREEMPHASIS = 0x2106;
  static constexpr uint16_t SI47XX_PROP_TX_RDS_PS_MIX = 0x2C02;
  static constexpr uint16_t SI47XX_PROP_TX_RDS_PS_MISC = 0x2C03;
  static constexpr uint16_t SI47XX_PROP_TX_RDS_PS_MESSAGE_COUNT = 0x2C05;

  static constexpr uint16_t SI47XX_TX_COMPONENT_ENABLE_RDS = 0x04;
  static constexpr uint16_t SI47XX_TX_COMPONENT_ENABLE_LR = 0x02;
  static constexpr uint16_t SI47XX_TX_COMPONENT_ENABLE_PILOT = 0x01;

  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSMS = 0x0008; // 0 = Speech, 1 = Music
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSTA = 0x0010; // Traffic Announcement
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSTP = 0x0400; // Traffic Program
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_FORCEB = 0x0800; // Force FIFO/Buffer to use PTY+TP from this property
  // Either my chip is faulty or the app note is wrong, but these are ordered backwards as opposed to the actual behavior
  // Following comments are the actual behavior (different than the app note)
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSD0 = 0x1000; // 0 = Static PTY, 1 = Dynamic PTY
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSD1 = 0x2000; // Artificial Head
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSD2 = 0x4000; // Compressed
  static constexpr uint16_t SI47XX_TX_RDS_PS_MISC_RDSD3 = 0x8000; // 0 = Mono, 1 = Stereo

} // namespace esphome::si4713
