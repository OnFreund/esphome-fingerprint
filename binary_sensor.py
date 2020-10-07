import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from . import CONF_FINGERPRINT_READER_ID, fingerprint_ns, FingerprintComponent

DEPENDENCIES = ['fingerprint_reader']

CONF_ENROLLING = "enrolling"

CONFIG_SCHEMA = cv.Schema.extend({
    cv.GenerateID(CONF_FINGERPRINT_READER_ID): cv.use_id(FingerprintComponent),
    cv.Optional(CONF_ENROLLING):
        binary_sensor.BINARY_SENSOR_SCHEMA,
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_ENROLLING in config:
        conf = config[CONF_ENROLLING]
        sens = yield binary_sensor.new_sensor(conf)
        cg.add(var.set_enrolling_binary_sensor(sens))
