# esphome-si4713
ESPHome component for Silicon Labs SI4713 FM Transmitter with RDS

## Example usage:

```yaml

external_components:
  - source: github://patagonaa/esphome-si4713@main
    components: [si4713]

i2c:
  sda: GPIO13
  scl: GPIO12
  id: si4713_i2c
  frequency: 400kHz
  scan: false

si4713:
  id: si4713_transmitter
  i2c_id: si4713_i2c
  reset_pin: GPIO11
  preemphasis_mode: 50US # 50us for Europe, Australia, Japan; 75us for US
  reference_clock_frequency: 32.768kHz # tune this if transmit frequency isn't 100% accurate
  output_frequency_number: "Output Frequency"
  output_power_number: "Output Power"
  output_enable_switch: "Output Enable"
  # optional:
  rds:
    station_name: "BeeMovie" # 8 chars max
    program_type: 7 # "Culture"
    radio_text: "According to all known laws of aviation\nthere is no way a bee sh" # 64 chars max
  input_level_sensor:
    name: "Input Level"
    filters:
      - throttle_average: 1s
```