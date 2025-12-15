"""Climate platform for Woleix IR climate component."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir, sensor, binary_sensor
from esphome.const import CONF_HUMIDITY_SENSOR

AUTO_LOAD = ["climate_ir"]
CONF_RESET_BUTTON = "reset_button"
CONF_OPTIONS = "options"
CONF_PROTOCOL = "protocol"

climate_ir_woleix_ns = cg.esphome_ns.namespace("climate_ir_woleix")
Protocol = climate_ir_woleix_ns.enum("Protocol", is_class=True)

WoleixClimate = climate_ir_woleix_ns.class_("WoleixClimate", climate_ir.ClimateIR)

PROTOCOLS = {
    "nec": Protocol.NEC,
    "pronto": Protocol.PRONTO
}
CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(WoleixClimate).extend(
    {
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_RESET_BUTTON): cv.use_id(binary_sensor.BinarySensor),
        cv.Optional(CONF_OPTIONS): cv.Schema({
            cv.Optional(CONF_PROTOCOL, default="nec"): cv.enum(PROTOCOLS, lower=True)
        })
    }
)


async def to_code(config):
    """Generate code for Woleix climate component."""
    var = await climate_ir.new_climate_ir(config)

    if CONF_HUMIDITY_SENSOR in config:
        sens = await cg.get_variable(config[CONF_HUMIDITY_SENSOR])
        cg.add(var.set_humidity_sensor(sens))

    if CONF_RESET_BUTTON in config:
        # Resolve the ID to the actual C++ variable
        btn = await cg.get_variable(config[CONF_RESET_BUTTON])
        cg.add(var.set_reset_button(btn))

    if CONF_OPTIONS in config:
        options = config[CONF_OPTIONS]
        cg.add(var.set_protocol(options[CONF_PROTOCOL]))
