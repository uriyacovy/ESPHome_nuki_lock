import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import lock, binary_sensor, text_sensor, sensor, switch, button
from esphome.const import (
    CONF_ID, 
    CONF_BATTERY_LEVEL, 
    DEVICE_CLASS_CONNECTIVITY, 
    DEVICE_CLASS_BATTERY, 
    DEVICE_CLASS_DOOR, 
    UNIT_PERCENT,
    ENTITY_CATEGORY_CONFIG, 
    ENTITY_CATEGORY_DIAGNOSTIC,
    CONF_TRIGGER_ID
)

AUTO_LOAD = ["binary_sensor", "text_sensor", "sensor", "switch", "button"]

CONF_IS_CONNECTED = "is_connected"
CONF_IS_PAIRED = "is_paired"
CONF_SECURITY_PIN = "security_pin"
CONF_UNPAIR_BUTTON = "unpair"
CONF_PAIRING_MODE_SWITCH = "pairing_mode"
CONF_BATTERY_CRITICAL = "battery_critical"
CONF_BATTERY_LEVEL = "battery_level"
CONF_DOOR_SENSOR = "door_sensor"
CONF_DOOR_SENSOR_STATE = "door_sensor_state"

CONF_SET_PAIRING_MODE = "pairing_mode"
CONF_PAIRING_MODE_TIMEOUT = "pairing_mode_timeout"

CONF_ON_PAIRING_MODE_ON = "on_pairing_mode_on_action"
CONF_ON_PAIRING_MODE_OFF = "on_pairing_mode_off_action"
CONF_ON_PAIRED = "on_paired_action"

nuki_lock_ns = cg.esphome_ns.namespace('nuki_lock')
NukiLock = nuki_lock_ns.class_('NukiLockComponent', lock.Lock, switch.Switch, cg.Component)

NukiLockUnpairButton = nuki_lock_ns.class_("NukiLockUnpairButton", button.Button, cg.Component)
NukiLockPairingModeSwitch = nuki_lock_ns.class_("NukiLockPairingModeSwitch", switch.Switch, cg.Component)

NukiLockUnpairAction = nuki_lock_ns.class_(
    "NukiLockUnpairAction", automation.Action
)

NukiLockPairingModeAction = nuki_lock_ns.class_(
    "NukiLockPairingModeAction", automation.Action
)

PairingModeOnTrigger = nuki_lock_ns.class_("PairingModeOnTrigger", automation.Trigger.template())
PairingModeOffTrigger = nuki_lock_ns.class_("PairingModeOffTrigger", automation.Trigger.template())
PairedTrigger = nuki_lock_ns.class_("PairedTrigger", automation.Trigger.template())

CONFIG_SCHEMA = lock.LOCK_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(NukiLock),
    cv.Required(CONF_IS_CONNECTED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_CONNECTIVITY,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Required(CONF_IS_PAIRED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_CONNECTIVITY,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_SECURITY_PIN, default=0): cv.uint16_t,
    cv.Optional(CONF_BATTERY_CRITICAL): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_BATTERY,
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        device_class=DEVICE_CLASS_BATTERY,
        unit_of_measurement=UNIT_PERCENT,
    ),
    cv.Optional(CONF_DOOR_SENSOR): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_DOOR,
    ),
    cv.Optional(CONF_DOOR_SENSOR_STATE): text_sensor.text_sensor_schema(),
    cv.Optional(CONF_UNPAIR_BUTTON): button.button_schema(
        NukiLockUnpairButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:link-off",
    ),
    cv.Optional(CONF_PAIRING_MODE_SWITCH): switch.switch_schema(
        NukiLockPairingModeSwitch,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:bluetooth-connect",
    ),
    cv.Optional(CONF_PAIRING_MODE_TIMEOUT, default="300s"): cv.positive_time_period_seconds,
    cv.Optional(CONF_ON_PAIRING_MODE_ON): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PairingModeOnTrigger),
        }
    ),
    cv.Optional(CONF_ON_PAIRING_MODE_OFF): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PairingModeOffTrigger),
        }
    ),
    cv.Optional(CONF_ON_PAIRED): automation.validate_automation(
        {
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PairedTrigger),
        }
    ),
}).extend(cv.polling_component_schema("500ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await lock.register_lock(var, config)
    if CONF_IS_CONNECTED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_IS_CONNECTED])
        cg.add(var.set_is_connected(sens))
    if CONF_IS_PAIRED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_IS_PAIRED])
        cg.add(var.set_is_paired(sens))

    if CONF_SECURITY_PIN in config:
        cg.add(var.set_security_pin(config[CONF_SECURITY_PIN]))

    if CONF_BATTERY_CRITICAL in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_BATTERY_CRITICAL])
        cg.add(var.set_battery_critical(sens))
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level(sens))
    if CONF_DOOR_SENSOR in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_DOOR_SENSOR])
        cg.add(var.set_door_sensor(sens))
    if CONF_DOOR_SENSOR_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_DOOR_SENSOR_STATE])
        cg.add(var.set_door_sensor_state(sens))

    if CONF_UNPAIR_BUTTON in config:
        btn = await button.new_button(config[CONF_UNPAIR_BUTTON])
        await cg.register_parented(btn, config[CONF_ID])
        cg.add(var.set_unpair_button(btn))

    if CONF_PAIRING_MODE_SWITCH in config:
        sw = await switch.new_switch(config[CONF_PAIRING_MODE_SWITCH])
        await cg.register_parented(sw, config[CONF_ID])
        cg.add(var.set_pairing_mode_switch(sw))

    cg.add(var.set_pairing_mode_timeout(config[CONF_PAIRING_MODE_TIMEOUT]))

    for conf in config.get(CONF_ON_PAIRING_MODE_ON, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_PAIRING_MODE_OFF, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_PAIRED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

@automation.register_action(
    "nuki_lock.unpair",
    NukiLockUnpairAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(NukiLock)
        }
    ),
)
async def nuki_lock_unpair_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)


@automation.register_action(
    "nuki_lock.set_pairing_mode",
    NukiLockPairingModeAction,
    automation.maybe_simple_id(
        {
            cv.GenerateID(): cv.use_id(NukiLock),
            cv.Required(CONF_SET_PAIRING_MODE): cv.templatable(cv.boolean)
        }
    ),
)
async def nuki_lock_set_pairing_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)