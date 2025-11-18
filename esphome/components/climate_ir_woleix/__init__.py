"""Woleix climate component for ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir, sensor
from esphome.const import CONF_ID, CONF_HUMIDITY_SENSOR

# Component metadata
CODEOWNERS = ["@ok11"]
DEPENDENCIES = ["climate_ir"]
AUTO_LOAD = []

# C++ namespace and class reference
climate_ir_woleix_ns = cg.esphome_ns.namespace("climate_ir_woleix")
WoleixClimate = climate_ir_woleix_ns.class_("WoleixClimate", climate_ir.ClimateIR)

# Configuration schema - extends climate_ir's schema with humidity sensor support
CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(WoleixClimate),
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
    }
)

# Code generation function
async def to_code(config):
    """Generate code for Woleix climate component."""
    var = cg.new_Pvariable(config[CONF_ID])
    await climate_ir.register_climate_ir(var, config)

    # Register humidity sensor if provided
    if humidity_sensor_id := config.get(CONF_HUMIDITY_SENSOR):
        humidity_sens = await cg.get_variable(humidity_sensor_id)
        cg.add(var.set_humidity_sensor(humidity_sens))
