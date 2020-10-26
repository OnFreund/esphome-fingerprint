import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import UNIT_EMPTY, ICON_ACCOUNT, ICON_ACCOUNT_CHECK, ICON_DATABASE, \
    ICON_EMPTY, ICON_FINGERPRINT, ICON_SECURITY
from . import CONF_RXXX_ID, RxxxComponent

DEPENDENCIES = ['rxxx']

CONF_FINGERPRINT_COUNT = 'fingerprint_count'
CONF_STATUS = 'status'
CONF_CAPACITY = 'capacity'
CONF_SECURITY_LEVEL = 'security_level'
CONF_LAST_FINGER_ID = 'last_finger_id'
CONF_LAST_CONFIDENCE = 'last_confidence'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_RXXX_ID): cv.use_id(RxxxComponent),
    cv.Optional(CONF_FINGERPRINT_COUNT): sensor.sensor_schema(UNIT_EMPTY, ICON_FINGERPRINT, 0),
    cv.Optional(CONF_STATUS): sensor.sensor_schema(UNIT_EMPTY, ICON_EMPTY, 0),
    cv.Optional(CONF_CAPACITY): sensor.sensor_schema(UNIT_EMPTY, ICON_DATABASE, 0),
    cv.Optional(CONF_SECURITY_LEVEL): sensor.sensor_schema(UNIT_EMPTY, ICON_SECURITY, 0),
    cv.Optional(CONF_LAST_FINGER_ID): sensor.sensor_schema(UNIT_EMPTY, ICON_ACCOUNT, 0),
    cv.Optional(CONF_LAST_CONFIDENCE): sensor.sensor_schema(UNIT_EMPTY, ICON_ACCOUNT_CHECK, 0),
})

def to_code(config):
    hub = yield cg.get_variable(config[CONF_RXXX_ID])

    for key in [CONF_FINGERPRINT_COUNT, CONF_STATUS, CONF_CAPACITY, CONF_SECURITY_LEVEL,
                CONF_LAST_FINGER_ID, CONF_LAST_CONFIDENCE]:
        if key not in config:
            continue
        conf = config[key]
        sens = yield sensor.new_sensor(conf)
        cg.add(getattr(hub, f'set_{key}_sensor')(sens))
