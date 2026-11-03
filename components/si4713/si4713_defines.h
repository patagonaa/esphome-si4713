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
  
  static constexpr uint16_t SI47XX_PROP_REFCLK_FREQ = 0x0201;
  static constexpr uint16_t SI47XX_PROP_TX_PREEMPHASIS = 0x2106;

} // namespace esphome::si4713
