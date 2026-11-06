from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, number, switch
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_MAX_VALUE,
    CONF_MIN_VALUE,
    CONF_MODE,
    CONF_NAME,
    CONF_RESET_PIN,
    CONF_RESTORE_VALUE,
    CONF_STEP,
    DEVICE_CLASS_FREQUENCY,
)
from esphome.cpp_generator import MockObj
from esphome.types import ConfigType

AUTO_LOAD = ["number", "switch"]
CODEOWNERS = ["@patagonaa"]
DEPENDENCIES = ["i2c"]
DOMAIN = "si4713"
MULTI_CONF = True

CONF_OUTPUT_FREQUENCY_NUMBER = "output_frequency_number"
CONF_OUTPUT_POWER_NUMBER = "output_power_number"
CONF_OUTPUT_ENABLE_SWITCH = "output_enable_switch"
CONF_PREEMPHASIS_MODE = "preemphasis_mode"
CONF_REFERENCE_CLOCK_FREQUENCY = "reference_clock_frequency"

CONF_RDS = "rds"
CONF_STATION_NAME = "station_name"
CONF_PROGRAM_TYPE = "program_type"
CONF_RADIO_TEXT = "radio_text"

UNIT_DBUV = "dBµV"
UNIT_MHZ = "MHz"

PREEMPHASIS_MODES = {
    "75US": 0,  # USA
    "50US": 1,  # Europe, Australia, Japan
    "OFF": 2,
}

si4713_ns = cg.esphome_ns.namespace("si4713")

Si4713Component = si4713_ns.class_(
    "Si4713Component", cg.Component
)

Si4713FrequencyNumber = si4713_ns.class_(
    "Si4713FrequencyNumber", number.Number, cg.Component
)

Si4713PowerNumber = si4713_ns.class_(
    "Si4713PowerNumber", number.Number, cg.Component
)

Si4713OutputEnableSwitch = si4713_ns.class_(
    "Si4713OutputEnableSwitch", switch.Switch, cg.Component
)


def validate_freq_min_max(config):
    if config[CONF_MAX_VALUE] <= config[CONF_MIN_VALUE]:
        raise cv.Invalid(f"{CONF_MAX_VALUE} must be greater than {CONF_MIN_VALUE}")
    if config[CONF_MIN_VALUE] < 76:
        raise cv.Invalid(f"{CONF_MIN_VALUE} must be greater than or equal to 76 MHz")
    if config[CONF_MAX_VALUE] > 108:
        raise cv.Invalid(f"{CONF_MAX_VALUE} must be less than or equal to 108 MHz")
    return config


def validate_power_min_max(config):
    if config[CONF_MAX_VALUE] <= config[CONF_MIN_VALUE]:
        raise cv.Invalid(f"{CONF_MAX_VALUE} must be greater than {CONF_MIN_VALUE}")
    if config[CONF_MIN_VALUE] < 88:
        raise cv.Invalid(f"{CONF_MIN_VALUE} must be greater than or equal to 88 dBµV")
    if config[CONF_MAX_VALUE] > 120:
        raise cv.Invalid(f"{CONF_MAX_VALUE} must be less than or equal to 120 dBµV")
    return config


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Si4713Component),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_PREEMPHASIS_MODE): cv.enum(PREEMPHASIS_MODES, upper=True),
            cv.Optional(CONF_REFERENCE_CLOCK_FREQUENCY, default="32.768kHz"): cv.All(
                    cv.frequency, cv.Range(min=31130, max=34406)
                ),
            cv.Required(CONF_OUTPUT_FREQUENCY_NUMBER): cv.maybe_simple_value(
                number.number_schema(
                    Si4713FrequencyNumber,
                    device_class=DEVICE_CLASS_FREQUENCY,
                    unit_of_measurement=UNIT_MHZ,
                )
                .extend(
                    {
                        cv.Optional(CONF_MAX_VALUE, default=108.0): cv.positive_float,
                        cv.Optional(CONF_MIN_VALUE, default=87.5): cv.positive_float,
                        cv.Optional(CONF_RESTORE_VALUE, default=True): cv.boolean,
                        cv.Optional(CONF_STEP, default=0.1): cv.float_range(min=0.05),
                        cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True),
                    }
                )
                .extend(cv.COMPONENT_SCHEMA),
                validate_freq_min_max,
                key=CONF_NAME,
            ),
            cv.Required(CONF_OUTPUT_POWER_NUMBER): cv.maybe_simple_value(
                number.number_schema(
                    Si4713PowerNumber,
                    unit_of_measurement=UNIT_DBUV
                )
                .extend(
                    {
                        cv.Optional(CONF_MAX_VALUE, default=115): cv.positive_int,
                        cv.Optional(CONF_MIN_VALUE, default=88): cv.positive_int,
                        cv.Optional(CONF_RESTORE_VALUE, default=True): cv.boolean,
                        cv.Optional(CONF_STEP, default=1): cv.int_range(min=1),
                    }
                )
                .extend(cv.COMPONENT_SCHEMA),
                validate_power_min_max,
                key=CONF_NAME,
            ),
            cv.Required(CONF_OUTPUT_ENABLE_SWITCH): cv.maybe_simple_value(
                switch.switch_schema(Si4713OutputEnableSwitch, default_restore_mode="RESTORE_DEFAULT_OFF")
                .extend(cv.COMPONENT_SCHEMA),
                key=CONF_NAME,
            ),
            cv.Optional(CONF_RDS): cv.Schema(
                {
                    cv.Optional(CONF_PROGRAM_TYPE, default=0): cv.int_range(0, 31),
                    cv.Required(CONF_STATION_NAME): cv.All(cv.string, cv.Length(min=1, max=8)),
                    cv.Optional(CONF_RADIO_TEXT): cv.All(cv.string, cv.Length(min=1, max=64)),
                }
            )
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x63))
)

FINAL_VALIDATE_SCHEMA = i2c.final_validate_device_schema(DOMAIN, max_frequency="400khz")

async def setup_rds(rds_config: ConfigType, hub: MockObj) -> None:
    cg.add(hub.set_rds_pty(rds_config[CONF_PROGRAM_TYPE]))
    cg.add(hub.set_rds_station_name(rds_config[CONF_STATION_NAME]))
    if CONF_RADIO_TEXT in rds_config:
        cg.add(hub.set_rds_text(rds_config[CONF_RADIO_TEXT]))
    cg.add(hub.set_rds_enabled(True))


async def setup_number(config: ConfigType, key: str) -> None:
    num_var = await number.new_number(
        config[key],
        min_value=config[key][CONF_MIN_VALUE],
        max_value=config[key][CONF_MAX_VALUE],
        step=config[key][CONF_STEP],
        )
    cg.add(
        num_var.set_restore_value(
            config[key][CONF_RESTORE_VALUE]
        )
    )
    await cg.register_parented(num_var, config[CONF_ID])
    await cg.register_component(num_var, config[key])


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])

    pin = await cg.gpio_pin_expression(config.get(CONF_RESET_PIN))
    cg.add(var.set_reset_pin(pin))

    cg.add(var.set_preemphasis_mode(config[CONF_PREEMPHASIS_MODE]))
    cg.add(var.set_reference_clock_frequency(config[CONF_REFERENCE_CLOCK_FREQUENCY]))

    if CONF_RDS in config:
        await setup_rds(config[CONF_RDS], var)

    await setup_number(config, CONF_OUTPUT_FREQUENCY_NUMBER)
    await setup_number(config, CONF_OUTPUT_POWER_NUMBER)

    sw_output_enable_var = await switch.new_switch(config[CONF_OUTPUT_ENABLE_SWITCH])
    await cg.register_parented(sw_output_enable_var, config[CONF_ID])
    await cg.register_component(sw_output_enable_var, config[CONF_OUTPUT_ENABLE_SWITCH])

    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
