import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PASSWORD, CONF_TRIGGER_ID, CONF_UART_ID

CODEOWNERS = ['@OnFreund', '@loongyh']
DEPENDENCIES = ['uart']
AUTO_LOAD = ['binary_sensor', 'sensor']
MULTI_CONF = True

CONF_SENSING_PIN = "sensing_pin"
CONF_ON_FINGER_SCANNED = "on_finger_scanned"
CONF_ON_ENROLLMENT_SCAN = "on_enrollment_scan"
CONF_ON_ENROLLMENT_DONE = "on_enrollment_done"
CONF_FINGER_ID = "finger_id"
CONF_NUM_SCANS = "num_scans"
CONF_RXXX_ID = 'rxxx_id'

rxxx_ns = cg.esphome_ns.namespace('rxxx')
RxxxComponent = rxxx_ns.class_('RxxxComponent', cg.PollingComponent, uart.UARTDevice)
ScannedTrigger = rxxx_ns.class_('FingerScannedTrigger',
  automation.Trigger.template(
    cg.bool_,
    cg.uint8,
    cg.uint16,
    cg.uint16))

EnrollmentScanTrigger = rxxx_ns.class_('EnrollmentScanTrigger',
  automation.Trigger.template(
    cg.uint8,
    cg.uint16))

EnrollmentTrigger = rxxx_ns.class_('EnrollmentTrigger',
  automation.Trigger.template(
    cg.bool_,
    cg.uint8,
    cg.uint16))

EnrollmentAction = rxxx_ns.class_('FingerprintEnrollAction', automation.Action)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RxxxComponent),
    cv.Required(CONF_SENSING_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_PASSWORD): cv.uint32_t,
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
    if CONF_PASSWORD in config:
        password = config[CONF_PASSWORD]
        cg.add(var.set_password(password))
    uart_device = yield uart.register_uart_device(var, config)
    cg.add(var.set_uart(uart_device))

    sensing_pin = yield cg.gpio_pin_expression(config[CONF_SENSING_PIN])
    cg.add(var.set_sensing_pin(sensing_pin))

    for conf in config.get(CONF_ON_FINGER_SCANNED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.bool_, 'success'),
          (cg.uint8, 'result'),
          (cg.uint16, 'finger_id'),
          (cg.uint16, 'confidence')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_SCAN, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.uint8, 'scan_number'),
          (cg.uint16, 'finger_id')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_DONE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        cg.add(var.register_trigger(trigger))
        yield automation.build_automation(trigger, [(cg.bool_, 'success'),
          (cg.uint8, 'result'),
          (cg.uint16, 'finger_id')], conf)

    # https://platformio.org/lib/show/382/Adafruit%20Fingerprint%20Sensor%20Library
    cg.add_library('https://github.com/adafruit/Adafruit-Fingerprint-Sensor-Library.git', None)

FINGER_ENROLL_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
    cv.Required(CONF_FINGER_ID): cv.templatable(cv.uint16_t),
    cv.Required(CONF_NUM_SCANS): cv.templatable(cv.uint8_t),
})


@automation.register_action('rxxx.enroll', EnrollmentAction, FINGER_ENROLL_SCHEMA)
def rxxx_enroll(config, action_id, template_arg, args):
    paren = yield cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = yield cg.templatable(config[CONF_FINGER_ID], args, cg.uint16)
    cg.add(var.set_finger_id(template_))
    template_ = yield cg.templatable(config[CONF_NUM_SCANS], args, cg.uint8)
    cg.add(var.set_num_scans(template_))
    yield var
