import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import CONF_RXXX_ID, RxxxComponent

DEPENDENCIES = ['rxxx']

CONF_ENROLLING = "enrolling"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_RXXX_ID): cv.use_id(RxxxComponent),
    cv.Optional(CONF_ENROLLING): binary_sensor.BINARY_SENSOR_SCHEMA,
})


def to_code(config):
    hub = yield cg.get_variable(config[CONF_RXXX_ID])

    if CONF_ENROLLING in config:
        conf = config[CONF_ENROLLING]
        sens = yield binary_sensor.new_binary_sensor(conf)
        cg.add(hub.set_enrolling_binary_sensor(sens))
