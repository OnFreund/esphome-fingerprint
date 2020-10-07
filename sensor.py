import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, UNIT_EMPTY, ICON_COUNTER, ICON_EMPTY
from . import CONF_FINGERPRINT_READER_ID, fingerprint_ns, FingerprintComponent

DEPENDENCIES = ['fingerprint_reader']

FingerprintEnrollingBinarySensor = fingerprint_ns.class_('FingerprintEnrollingBinarySensor', binary_sensor.BinarySensor)

CONF_SENSING_PIN = "sensing_pin"
CONF_FINGERPRINT_COUNT = "fingerprint_count"
CONF_STATUS = "status"
CONF_CAPACITY = "capacity"
CONF_SECURITY_LEVEL = "security_level"
CONF_LAST_FINGER_ID = "last_finger_id"
CONF_LAST_CONFIDENCE = "last_confidence"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_FINGERPRINT_READER_ID): cv.use_id(FingerprintComponent),
    cv.Optional(CONF_FINGERPRINT_COUNT):
        sensor.sensor_schema(UNIT_EMPTY, ICON_COUNTER, 0),
    cv.Optional(CONF_STATUS):
        sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 0),
    cv.Optional(CONF_CAPACITY):
        sensor.sensor_schema(UNIT_EMPTY, ICON_COUNTER, 0),
    cv.Optional(CONF_SECURITY_LEVEL):
        sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 0),
    cv.Optional(CONF_LAST_FINGER_ID):
        sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 0),
    cv.Optional(CONF_LAST_CONFIDENCE):
        sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 0),
})

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    if CONF_FINGERPRINT_COUNT in config:
        conf = config[CONF_FINGERPRINT_COUNT]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))

    if CONF_STATUS in config:
        conf = config[CONF_STATUS]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))

    if CONF_CAPACITY in config:
        conf = config[CONF_CAPACITY]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))

    if CONF_SECURITY_LEVEL in config:
        conf = config[CONF_SECURITY_LEVEL]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))

    if CONF_LAST_FINGER_ID in config:
        conf = config[CONF_LAST_FINGER_ID]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))

    if CONF_LAST_CONFIDENCE in config:
        conf = config[CONF_LAST_CONFIDENCE]
        sens = yield sensor.new_sensor(conf)
        cg.add(var.set_fingerprint_count_sensor(sens))
