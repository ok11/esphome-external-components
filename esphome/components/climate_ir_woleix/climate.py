"""Climate platform for Woleix IR climate component."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir, sensor
from esphome.const import CONF_HUMIDITY_SENSOR

AUTO_LOAD = ["climate_ir"]
CONF_RESET_BUTTON = "reset_button"
CONF_OPTIONS = "options"
CONF_PROTOCOL = "protocol"

climate_ir_woleix_ns = cg.esphome_ns.namespace("climate_ir_woleix")
Protocol = climate_ir_woleix_ns.enum("Protocol", is_class=True)

WoleixClimate = climate_ir_woleix_ns.class_("WoleixClimate", climate_ir.ClimateIR)

CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(WoleixClimate).extend(
    {
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor)
    }
)


async def to_code(config):
    """Generate code for Woleix climate component."""
    var = await climate_ir.new_climate_ir(config)

    if CONF_HUMIDITY_SENSOR in config:
        sens = await cg.get_variable(config[CONF_HUMIDITY_SENSOR])
        cg.add(var.set_humidity_sensor(sens))

