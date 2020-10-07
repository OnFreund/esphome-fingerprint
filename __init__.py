import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PASSWORD, CONF_TRIGGER_ID, CONF_UART_ID

CODEOWNERS = ['@OnFreund']
DEPENDENCIES = ['uart']
AUTO_LOAD = ['binary_sensor', 'sensor']
MULTI_CONF = True

CONF_SENSING_PIN = "sensing_pin"
CONF_ON_FINGER_SCANNED = "on_finger_scanned"
CONF_ON_ENROLLMENT_SCAN = "on_enrollment_scan"
CONF_ON_ENROLLMENT_DONE = "on_enrollment_done"
CONF_FINGER_ID = "finger_id"
CONF_NUM_SCANS = "num_scans"
CONF_FINGERPRINT_READER_ID = 'fingerprint_reader_id'

fingerprint_ns = cg.esphome_ns.namespace('fingerprint_reader')
FingerprintComponent = fingerprint_ns.class_('FingerprintReader', cg.PollingComponent)
ScannedTrigger = fingerprint_ns.class_('FingerScannedTrigger',
  automation.Trigger.template(
    cg.bool,
    cg.int,
    cg.int,
    cg.int))

EnrollmentScanTrigger = fingerprint_ns.class_('EnrollmentScanTrigger',
  automation.Trigger.template(
    cg.int,
    cg.int))

EnrollmentTrigger = fingerprint_ns.class_('EnrollmentTrigger',
  automation.Trigger.template(
    cg.bool,
    cg.int,
    cg.int))

EnrollmentAction = fingerprint_ns.class_('FingerprintEnrollAction', automation.Action)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(FingerprintComponent),
    cv.Required(CONF_SENSING_PIN): cv.int,
    cv.Optional(CONF_PASSWORD): cv.int,
    cv.Optional(CONF_ON_FINGER_SCANNED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ScannedTrigger),
    }),
    cv.Optional(CONF_ON_ENROLLMENT_SCAN): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnrollmentScanTrigger),
    }),
    cv.Optional(CONF_ON_ENROLLMENT_DONE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnrollmentTrigger),
    }),
}).extend(cv.polling_component_schema('500ms')).extend(uart.UART_DEVICE_SCHEMA)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    # yield uart.register_uart_device(var, config)
    if CONF_PASSWORD in config:
        password = config[CONF_PASSWORD]
        cg.add(var.set_password(password))

    var pin = yield cg.get_variable(config[CONF_SENSING_PIN])
    cg.add(var.set_sensing_pin(pin))
    var uart = yield cg.get_variable(config[CONF_UART_ID])
    cg.add(var.set_uart(uart))

    for conf in config.get(CONF_ON_FINGER_SCANNED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.bool, 'success')
          (cg.int, 'result'),
          (cg.int, 'finger_id'),
          (cg.int, 'confidence')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_SCAN, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.int, 'scan_number'),
          (cg.int, 'finger_id')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_DONE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.bool, 'success')
          (cg.int, 'result'),
          (cg.int, 'finger_id')], conf)


FINGER_ENROLL_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(FingerprintComponent),
    cv.Required(CONF_FINGER_ID): cv.templatable(cv.int),
    cv.Required(CONF_NUM_SCANS): cv.templatable(cv.int),
})


@automation.register_action('fingerprint_reader.enroll', EnrollmentAction, FINGER_ENROLL_SCHEMA)
def fingerprint_reader_enroll(config, action_id, template_arg, args):
    paren = yield cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = yield cg.templatable(config[CONF_FINGER_ID], args, cg.int)
    cg.add(var.set_finger_id(template_))
    template_ = yield cg.templatable(config[CONF_NUM_SCANS], args, cg.int)
    cg.add(var.set_num_scans(template_))
    yield var
