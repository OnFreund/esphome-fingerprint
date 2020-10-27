import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome import pins
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PASSWORD, CONF_SPEED, CONF_STATE, CONF_TRIGGER_ID, \
    CONF_UART_ID

CODEOWNERS = ['@OnFreund', '@loongyh']
DEPENDENCIES = ['uart']
AUTO_LOAD = ['binary_sensor', 'sensor']
MULTI_CONF = True

CONF_SENSING_PIN = 'sensing_pin'
CONF_ON_FINGER_SCAN_MATCHED = 'on_finger_scan_matched'
CONF_ON_FINGER_SCAN_UNMATCHED = 'on_finger_scan_unmatched'
CONF_ON_ENROLLMENT_SCAN = 'on_enrollment_scan'
CONF_ON_ENROLLMENT_DONE = 'on_enrollment_done'
CONF_ON_ENROLLMENT_FAILED = 'on_enrollment_failed'
CONF_FINGER_ID = 'finger_id'
CONF_NUM_SCANS = 'num_scans'
CONF_RXXX_ID = 'rxxx_id'
CONF_COLOR = 'color'
CONF_COUNT = 'count'

rxxx_ns = cg.esphome_ns.namespace('rxxx')
RxxxComponent = rxxx_ns.class_('RxxxComponent', cg.PollingComponent, uart.UARTDevice)

FingerScanMatchedTrigger = rxxx_ns.class_('FingerScanMatchedTrigger',
  automation.Trigger.template(
    cg.uint16,
    cg.uint16))

FingerScanUnmatchedTrigger = rxxx_ns.class_('FingerScanUnmatchedTrigger',
    automation.Trigger.template())

EnrollmentScanTrigger = rxxx_ns.class_('EnrollmentScanTrigger',
  automation.Trigger.template(
    cg.uint8,
    cg.uint16))

EnrollmentDoneTrigger = rxxx_ns.class_('EnrollmentDoneTrigger',
    automation.Trigger.template(cg.uint16))

EnrollmentFailedTrigger = rxxx_ns.class_('EnrollmentFailedTrigger',
    automation.Trigger.template(cg.uint16))

EnrollmentAction = rxxx_ns.class_('EnrollmentAction', automation.Action)
CancelEnrollmentAction = rxxx_ns.class_('CancelEnrollmentAction', automation.Action)
DeleteAction = rxxx_ns.class_('DeleteAction', automation.Action)
DeleteAllAction = rxxx_ns.class_('DeleteAllAction', automation.Action)
AuraLEDControlAction = rxxx_ns.class_('AuraLEDControlAction', automation.Action)

AuraLEDMode = rxxx_ns.enum('AuraLEDMode')
AURA_LED_STATES = {
    'BREATHING': AuraLEDMode.BREATHING,
    'FLASHING': AuraLEDMode.FLASHING,
    'ALWAYS_ON': AuraLEDMode.ALWAYS_ON,
    'ALWAYS_OFF': AuraLEDMode.ALWAYS_OFF,
    'GRADUAL_ON': AuraLEDMode.GRADUAL_ON,
    'GRADUAL_OFF': AuraLEDMode.GRADUAL_OFF,
}
validate_aura_led_states = cv.enum(AURA_LED_STATES, upper=True)
AURA_LED_COLORS = {
    'RED': AuraLEDMode.RED,
    'BLUE': AuraLEDMode.BLUE,
    'PURPLE': AuraLEDMode.PURPLE,
}
validate_aura_led_colors = cv.enum(AURA_LED_COLORS, upper=True)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(RxxxComponent),
    cv.Optional(CONF_SENSING_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_PASSWORD): cv.uint32_t,
    cv.Optional(CONF_ON_FINGER_SCAN_MATCHED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FingerScanMatchedTrigger),
    }),
    cv.Optional(CONF_ON_FINGER_SCAN_UNMATCHED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(FingerScanUnmatchedTrigger),
    }),
    cv.Optional(CONF_ON_ENROLLMENT_SCAN): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnrollmentScanTrigger),
    }),
    cv.Optional(CONF_ON_ENROLLMENT_DONE): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnrollmentDoneTrigger),
    }),
    cv.Optional(CONF_ON_ENROLLMENT_FAILED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EnrollmentFailedTrigger),
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

    if CONF_SENSING_PIN in config:
        sensing_pin = yield cg.gpio_pin_expression(config[CONF_SENSING_PIN])
        cg.add(var.set_sensing_pin(sensing_pin))

    for conf in config.get(CONF_ON_FINGER_SCAN_MATCHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.uint16, 'finger_id'),
          (cg.uint16, 'confidence')], conf)

    for conf in config.get(CONF_ON_FINGER_SCAN_UNMATCHED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_SCAN, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.uint8, 'scan_number'),
          (cg.uint16, 'finger_id')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_DONE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.uint16, 'finger_id')], conf)

    for conf in config.get(CONF_ON_ENROLLMENT_FAILED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.uint16, 'finger_id')], conf)

    # https://platformio.org/lib/show/382/Adafruit%20Fingerprint%20Sensor%20Library
    cg.add_library('382', '2.0.4')


@automation.register_action('rxxx.enroll', EnrollmentAction, cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
    cv.Required(CONF_FINGER_ID): cv.templatable(cv.uint16_t),
    cv.Optional(CONF_NUM_SCANS): cv.templatable(cv.uint8_t),
}))
def rxxx_enroll_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    yield cg.register_parented(var, config[CONF_ID])
    
    template_ = yield cg.templatable(config[CONF_FINGER_ID], args, cg.uint16)
    cg.add(var.set_finger_id(template_))
    if CONF_NUM_SCANS in config:
        template_ = yield cg.templatable(config[CONF_NUM_SCANS], args, cg.uint8)
        cg.add(var.set_num_scans(template_))
    yield var


@automation.register_action('rxxx.aura_led_control', AuraLEDControlAction, cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
    cv.Required(CONF_STATE): cv.templatable(validate_aura_led_states),
    cv.Required(CONF_SPEED): cv.templatable(cv.uint8_t),
    cv.Required(CONF_COLOR): cv.templatable(validate_aura_led_colors),
    cv.Required(CONF_COUNT): cv.templatable(cv.uint8_t),
}))
def rxxx_aura_led_control_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    yield cg.register_parented(var, config[CONF_ID])
    
    for key in [CONF_STATE, CONF_SPEED, CONF_COLOR, CONF_COUNT]:
        template_ = yield cg.templatable(config[key], args, cg.uint8)
        cg.add(getattr(var, f'set_{key}')(template_))
    yield var


@automation.register_action('rxxx.cancel_enroll', CancelEnrollmentAction, cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
}))
def rxxx_cancel_enroll_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    yield cg.register_parented(var, config[CONF_ID])
    yield var

@automation.register_action('rxxx.delete', DeleteAction, cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
    cv.Required(CONF_FINGER_ID): cv.templatable(cv.uint16_t),
}))
def rxxx_delete_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    yield cg.register_parented(var, config[CONF_ID])
    
    template_ = yield cg.templatable(config[CONF_FINGER_ID], args, cg.uint16)
    cg.add(var.set_finger_id(template_))
    yield var

@automation.register_action('rxxx.delete_all', DeleteAllAction, cv.Schema({
    cv.GenerateID(): cv.use_id(RxxxComponent),
}))
def rxxx_delete_all_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    yield cg.register_parented(var, config[CONF_ID])
    yield var