import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import lock, binary_sensor, text_sensor, sensor, switch, button, number, select
from esphome.components.number import NUMBER_MODES
from esphome.const import (
    CONF_ID, 
    CONF_BATTERY_LEVEL, 
    DEVICE_CLASS_CONNECTIVITY, 
    DEVICE_CLASS_BATTERY, 
    DEVICE_CLASS_DOOR, 
    DEVICE_CLASS_SWITCH,
    UNIT_PERCENT,
    ENTITY_CATEGORY_CONFIG, 
    ENTITY_CATEGORY_DIAGNOSTIC,
    CONF_TRIGGER_ID,
    CONF_MODE,
)

AUTO_LOAD = ["binary_sensor", "text_sensor", "sensor", "switch", "button", "number", "select"]

CONF_IS_CONNECTED = "is_connected"
CONF_IS_PAIRED = "is_paired"
CONF_BATTERY_CRITICAL = "battery_critical"
CONF_DOOR_SENSOR = "door_sensor"

CONF_BATTERY_LEVEL = "battery_level"

CONF_DOOR_SENSOR_STATE = "door_sensor_state"
CONF_LAST_UNLOCK_USER_TEXT_SENSOR = "last_unlock_user"

CONF_UNPAIR_BUTTON = "unpair"

CONF_PAIRING_MODE_SWITCH = "pairing_mode"
CONF_AUTO_UNLATCH_SWITCH = "auto_unlatch"
CONF_BUTTON_ENABLED_SWITCH = "button_enabled"
CONF_LED_ENABLED_SWITCH = "led_enabled"
CONF_NIGHT_MODE_ENABLED_SWITCH = "night_mode_enabled"
CONF_NIGHT_MODE_AUTO_LOCK_ENABLED_SWITCH = "night_mode_auto_lock_enabled"
CONF_NIGHT_MODE_AUTO_UNLOCK_DISABLED_SWITCH = "night_mode_auto_unlock_disabled"
CONF_NIGHT_MODE_IMMEDIATE_LOCK_ON_START_ENABLED_SWITCH = "night_mode_immediate_lock_on_start_enabled"
CONF_AUTO_LOCK_ENABLED_SWITCH = "auto_lock_enabled"
CONF_AUTO_UNLOCK_DISABLED_SWITCH = "auto_unlock_disabled"
CONF_IMMEDIATE_AUTO_LOCK_ENABLED_SWITCH = "immediate_auto_lock_enabled"
CONF_AUTO_UPDATE_ENABLED_SWITCH = "auto_update_enabled"

CONF_SINGLE_BUTTON_PRESS_ACTION_SELECT = "single_buton_press_action"
CONF_DOUBLE_BUTTON_PRESS_ACTION_SELECT = "double_buton_press_action"
CONF_FOB_ACTION_1_SELECT = "fob_action_1"
CONF_FOB_ACTION_2_SELECT = "fob_action_2"
CONF_FOB_ACTION_3_SELECT = "fob_action_3"

CONF_LED_BRIGHTNESS_NUMBER = "led_brightness"
CONF_SECURITY_PIN_NUMBER = "security_pin"

CONF_BUTTON_PRESS_ACTION_SELECT_OPTIONS = [
    "No Action",
    "Intelligent",
    "Unlock",
    "Lock",
    "Unlatch",
    "Lock n Go",
    "Show Status",
]

CONF_FOB_ACTION_SELECT_OPTIONS = [
    "No Action",
    "Unlock",
    "Lock",
    "Lock n Go",
    "Intelligent",
]

CONF_PAIRING_MODE_TIMEOUT = "pairing_mode_timeout"
CONF_EVENT = "event"

CONF_SET_PAIRING_MODE = "pairing_mode"

CONF_ON_PAIRING_MODE_ON = "on_pairing_mode_on_action"
CONF_ON_PAIRING_MODE_OFF = "on_pairing_mode_off_action"
CONF_ON_PAIRED = "on_paired_action"

nuki_lock_ns = cg.esphome_ns.namespace('nuki_lock')
NukiLock = nuki_lock_ns.class_('NukiLockComponent', lock.Lock, switch.Switch, cg.Component)

NukiLockUnpairButton = nuki_lock_ns.class_("NukiLockUnpairButton", button.Button, cg.Component)
NukiLockPairingModeSwitch = nuki_lock_ns.class_("NukiLockPairingModeSwitch", switch.Switch, cg.Component)
NukiLockAutoUnlatchEnabledSwitch = nuki_lock_ns.class_("NukiLockAutoUnlatchEnabledSwitch", switch.Switch, cg.Component)
NukiLockButtonEnabledSwitch = nuki_lock_ns.class_("NukiLockButtonEnabledSwitch", switch.Switch, cg.Component)
NukiLockLedEnabledSwitch = nuki_lock_ns.class_("NukiLockLedEnabledSwitch", switch.Switch, cg.Component)
NukiLockNightModeEnabledSwitch = nuki_lock_ns.class_("NukiLockNightModeEnabledSwitch", switch.Switch, cg.Component)
NukiLockNightModeAutoLockEnabledSwitch = nuki_lock_ns.class_("NukiLockNightModeAutoLockEnabledSwitch", switch.Switch, cg.Component)
NukiLockNightModeAutoUnlockDisabledSwitch = nuki_lock_ns.class_("NukiLockNightModeAutoUnlockDisabledSwitch", switch.Switch, cg.Component)
NukiLockNightModeImmediateLockOnStartEnabledSwitch = nuki_lock_ns.class_("NukiLockNightModeImmediateLockOnStartEnabledSwitch", switch.Switch, cg.Component)
NukiLockAutoLockEnabledSwitch = nuki_lock_ns.class_("NukiLockAutoLockEnabledSwitch", switch.Switch, cg.Component)
NukiLockAutoUnlockDisabledSwitch = nuki_lock_ns.class_("NukiLockAutoUnlockDisabledSwitch", switch.Switch, cg.Component)
NukiLockImmediateAutoLockEnabledSwitch = nuki_lock_ns.class_("NukiLockImmediateAutoLockEnabledSwitch", switch.Switch, cg.Component)
NukiLockAutoUpdateEnabledSwitch = nuki_lock_ns.class_("NukiLockAutoUpdateEnabledSwitch", switch.Switch, cg.Component)
NukiLockLedBrightnessNumber = nuki_lock_ns.class_("NukiLockLedBrightnessNumber", number.Number, cg.Component)
NukiLockSecurityPinNumber = nuki_lock_ns.class_("NukiLockSecurityPinNumber", number.Number, cg.Component)
NukiLockSingleButtonPressActionSelect = nuki_lock_ns.class_("NukiLockSingleButtonPressActionSelect", select.Select, cg.Component)
NukiLockDoubleButtonPressActionSelect = nuki_lock_ns.class_("NukiLockDoubleButtonPressActionSelect", select.Select, cg.Component)
NukiLockFobAction1Select = nuki_lock_ns.class_("NukiLockFobAction1Select", select.Select, cg.Component)
NukiLockFobAction2Select = nuki_lock_ns.class_("NukiLockFobAction2Select", select.Select, cg.Component)
NukiLockFobAction3Select = nuki_lock_ns.class_("NukiLockFobAction3Select", select.Select, cg.Component)

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
        icon="mdi:link"
    ),
    cv.Required(CONF_IS_PAIRED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_CONNECTIVITY,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:link"
    ),
    cv.Optional(CONF_BATTERY_CRITICAL): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_BATTERY,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:battery-alert-variant-outline",
    ),
    cv.Optional(CONF_DOOR_SENSOR): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_DOOR,
        icon="mdi:door-open",
    ),

    cv.Optional(CONF_DOOR_SENSOR_STATE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:door-open",
    ),
    cv.Optional(CONF_LAST_UNLOCK_USER_TEXT_SENSOR):  text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:account-clock"
    ),

    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        device_class=DEVICE_CLASS_BATTERY,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        unit_of_measurement=UNIT_PERCENT,
        icon="mdi:battery-50",
    ),

    cv.Optional(CONF_UNPAIR_BUTTON): button.button_schema(
        NukiLockUnpairButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:link-off",
    ),

    cv.Optional(CONF_PAIRING_MODE_SWITCH): switch.switch_schema(
        NukiLockPairingModeSwitch,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:bluetooth"
    ),
    cv.Optional(CONF_BUTTON_ENABLED_SWITCH): switch.switch_schema(
        NukiLockButtonEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:radiobox-marked"
    ),
    cv.Optional(CONF_AUTO_UNLATCH_SWITCH): switch.switch_schema(
        NukiLockAutoUnlatchEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:auto-upload"
    ),
    cv.Optional(CONF_LED_ENABLED_SWITCH): switch.switch_schema(
        NukiLockLedEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:led-on"
    ),
    cv.Optional(CONF_NIGHT_MODE_ENABLED_SWITCH): switch.switch_schema(
        NukiLockNightModeEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:shield-moon",
    ),
    cv.Optional(CONF_NIGHT_MODE_AUTO_LOCK_ENABLED_SWITCH): switch.switch_schema(
        NukiLockNightModeAutoLockEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-clock"
    ),
    cv.Optional(CONF_NIGHT_MODE_AUTO_UNLOCK_DISABLED_SWITCH): switch.switch_schema(
        NukiLockNightModeAutoUnlockDisabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-remove"
    ),
    cv.Optional(CONF_NIGHT_MODE_IMMEDIATE_LOCK_ON_START_ENABLED_SWITCH): switch.switch_schema(
        NukiLockNightModeImmediateLockOnStartEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-alert"
    ),
    cv.Optional(CONF_AUTO_LOCK_ENABLED_SWITCH): switch.switch_schema(
        NukiLockAutoLockEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-clock"
    ),
    cv.Optional(CONF_AUTO_UNLOCK_DISABLED_SWITCH): switch.switch_schema(
        NukiLockAutoUnlockDisabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-remove"
    ),
    cv.Optional(CONF_IMMEDIATE_AUTO_LOCK_ENABLED_SWITCH): switch.switch_schema(
        NukiLockImmediateAutoLockEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:lock-alert"
        
    ),
    cv.Optional(CONF_AUTO_UPDATE_ENABLED_SWITCH): switch.switch_schema(
        NukiLockAutoUpdateEnabledSwitch,
        device_class=DEVICE_CLASS_SWITCH,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:auto-download",
    ),

    cv.Optional(CONF_LED_BRIGHTNESS_NUMBER): number.number_schema(
        NukiLockLedBrightnessNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:brightness-6",
    ),

    cv.Optional(CONF_SECURITY_PIN_NUMBER): number.number_schema(
        NukiLockSecurityPinNumber,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:shield-key",
    ).extend({ cv.Optional(CONF_MODE, default="BOX"): cv.enum(NUMBER_MODES, upper=True), }),

    cv.Optional(CONF_SINGLE_BUTTON_PRESS_ACTION_SELECT): select.select_schema(
        NukiLockSingleButtonPressActionSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:gesture-tap",
    ),
    cv.Optional(CONF_DOUBLE_BUTTON_PRESS_ACTION_SELECT): select.select_schema(
        NukiLockDoubleButtonPressActionSelect,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:gesture-tap",
    ),
    cv.Optional(CONF_FOB_ACTION_1_SELECT): select.select_schema(
        NukiLockFobAction1Select,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:gesture-tap",
    ),
    cv.Optional(CONF_FOB_ACTION_2_SELECT): select.select_schema(
        NukiLockFobAction2Select,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:gesture-tap",
    ),
    cv.Optional(CONF_FOB_ACTION_3_SELECT): select.select_schema(
        NukiLockFobAction3Select,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:gesture-tap",
    ),

    cv.Optional(CONF_PAIRING_MODE_TIMEOUT, default="300s"): cv.positive_time_period_seconds,
    cv.Optional(CONF_EVENT, default="nuki"): cv.string,

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

    # Component Settings
    if CONF_PAIRING_MODE_TIMEOUT in config:
        cg.add(var.set_pairing_mode_timeout(config[CONF_PAIRING_MODE_TIMEOUT]))

    if CONF_EVENT in config:
        cg.add(var.set_event("esphome." + config[CONF_EVENT]))

    # Binary Sensor
    if is_connected := config.get(CONF_IS_CONNECTED):
        sens = await binary_sensor.new_binary_sensor(is_connected)
        cg.add(var.set_is_connected_binary_sensor(sens))

    if is_paired := config.get(CONF_IS_PAIRED):
        sens = await binary_sensor.new_binary_sensor(is_paired)
        cg.add(var.set_is_paired_binary_sensor(sens))

    if battery_critical := config.get(CONF_BATTERY_CRITICAL):
        sens = await binary_sensor.new_binary_sensor(battery_critical)
        cg.add(var.set_battery_critical_binary_sensor(sens))

    if door_sensor := config.get(CONF_DOOR_SENSOR):
        sens = await binary_sensor.new_binary_sensor(door_sensor)
        cg.add(var.set_door_sensor_binary_sensor(sens))

    # Sensor
    if battery_level := config.get(CONF_BATTERY_LEVEL):
        sens = await sensor.new_sensor(battery_level)
        cg.add(var.set_battery_level_sensor(sens))

    # Text Sensor
    if door_sensor_state := config.get(CONF_DOOR_SENSOR_STATE):
        sens = await text_sensor.new_text_sensor(door_sensor_state)
        cg.add(var.set_door_sensor_state_text_sensor(sens))

    if last_unlock_user := config.get(CONF_LAST_UNLOCK_USER_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(last_unlock_user)
        cg.add(var.set_last_unlock_user_text_sensor(sens))

    # Button
    if unpair := config.get(CONF_UNPAIR_BUTTON):
        b = await button.new_button(unpair)
        await cg.register_parented(b, config[CONF_ID])
        cg.add(var.set_unpair_button(b))

    # Number
    if led_brightness := config.get(CONF_LED_BRIGHTNESS_NUMBER):
        n = await number.new_number(
            led_brightness, min_value=0, max_value=5, step=1
        )
        await cg.register_parented(n, config[CONF_ID])
        cg.add(var.set_led_brightness_number(n))

    if security_pin := config.get(CONF_SECURITY_PIN_NUMBER):
        n = await number.new_number(
            security_pin, min_value=0, max_value=65535, step=1
        )
        await cg.register_parented(n, config[CONF_ID])
        cg.add(var.set_security_pin_number(n))

    # Switch
    if pairing_mode := config.get(CONF_PAIRING_MODE_SWITCH):
        s = await switch.new_switch(pairing_mode)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_pairing_mode_switch(s))

    if button_enabled := config.get(CONF_BUTTON_ENABLED_SWITCH):
        s = await switch.new_switch(button_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_button_enabled_switch(s))

    if auto_unlatch := config.get(CONF_AUTO_UNLATCH_SWITCH):
        s = await switch.new_switch(auto_unlatch)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_auto_unlatch_enabled_switch(s))

    if led_enabled := config.get(CONF_LED_ENABLED_SWITCH):
        s = await switch.new_switch(led_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_led_enabled_switch(s))

    if nightmode_enabled := config.get(CONF_NIGHT_MODE_ENABLED_SWITCH):
        s = await switch.new_switch(nightmode_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_nightmode_enabled_switch(s))

    if night_mode_auto_lock_enabled := config.get(CONF_NIGHT_MODE_AUTO_LOCK_ENABLED_SWITCH):
        s = await switch.new_switch(night_mode_auto_lock_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_night_mode_auto_lock_enabled_switch(s))

    if night_mode_auto_unlock_disabled := config.get(CONF_NIGHT_MODE_AUTO_UNLOCK_DISABLED_SWITCH):
        s = await switch.new_switch(night_mode_auto_unlock_disabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_night_mode_auto_unlock_disabled_switch(s))

    if night_mode_immediate_lock_on_start := config.get(CONF_NIGHT_MODE_IMMEDIATE_LOCK_ON_START_ENABLED_SWITCH):
        s = await switch.new_switch(night_mode_immediate_lock_on_start)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_night_mode_immediate_lock_on_start_switch(s))

    if auto_lock_enabled := config.get(CONF_AUTO_LOCK_ENABLED_SWITCH):
        s = await switch.new_switch(auto_lock_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_auto_lock_enabled_switch(s))

    if auto_unlock_disabled := config.get(CONF_AUTO_UNLOCK_DISABLED_SWITCH):
        s = await switch.new_switch(auto_unlock_disabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_auto_unlock_disabled_switch(s))

    if immediate_auto_lock_enabled := config.get(CONF_IMMEDIATE_AUTO_LOCK_ENABLED_SWITCH):
        s = await switch.new_switch(immediate_auto_lock_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_immediate_auto_lock_enabled_switch(s))

    if auto_update_enabled := config.get(CONF_AUTO_UPDATE_ENABLED_SWITCH):
        s = await switch.new_switch(auto_update_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_auto_update_enabled_switch(s))

    # Select
    if single_button_press_action := config.get(CONF_SINGLE_BUTTON_PRESS_ACTION_SELECT):
        sel = await select.new_select(
            single_button_press_action,
            options=[CONF_BUTTON_PRESS_ACTION_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_single_button_press_action_select(sel))

    if double_button_press_action := config.get(CONF_DOUBLE_BUTTON_PRESS_ACTION_SELECT):
        sel = await select.new_select(
            double_button_press_action,
            options=[CONF_BUTTON_PRESS_ACTION_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_double_button_press_action_select(sel))

    if fob_action_1 := config.get(CONF_FOB_ACTION_1_SELECT):
        sel = await select.new_select(
            fob_action_1,
            options=[CONF_FOB_ACTION_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_fob_action_1_select(sel))

    if fob_action_2 := config.get(CONF_FOB_ACTION_2_SELECT):
        sel = await select.new_select(
            fob_action_2,
            options=[CONF_FOB_ACTION_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_fob_action_2_select(sel))

    if fob_action_3 := config.get(CONF_FOB_ACTION_3_SELECT):
        sel = await select.new_select(
            fob_action_3,
            options=[CONF_FOB_ACTION_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_fob_action_3_select(sel))


    # Callback
    for conf in config.get(CONF_ON_PAIRING_MODE_ON, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_PAIRING_MODE_OFF, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_PAIRED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    # Libraries
    cg.add_library("Preferences", None)
    cg.add_library("h2zero/NimBLE-Arduino", "1.4.0")
    cg.add_library("Crc16", None)
    cg.add_library(
        None,
        None,
        "https://github.com/I-Connect/NukiBleEsp32#93e7da927171c8973b7ef857c7fa644c174ed47d",
    )


# Actions
NukiLockUnpairAction = nuki_lock_ns.class_(
    "NukiLockUnpairAction", automation.Action
)

NUKI_LOCK_UNPAIR_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(NukiLock)
    }
)

@automation.register_action(
    "nuki_lock.unpair", NukiLockUnpairAction, NUKI_LOCK_UNPAIR_SCHEMA
)

async def nuki_lock_unpair_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)



NUKI_LOCK_SET_PAIRING_MODE_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(NukiLock),
        cv.Required(CONF_SET_PAIRING_MODE): cv.templatable(cv.boolean)
    }
)

@automation.register_action(
    "nuki_lock.set_pairing_mode", NukiLockPairingModeAction, NUKI_LOCK_SET_PAIRING_MODE_SCHEMA
)

async def nuki_lock_set_pairing_mode_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    pairing_mode_template_ = await cg.templatable(config[CONF_SET_PAIRING_MODE], args, cg.bool_)
    cg.add(var.set_pairing_mode(pairing_mode_template_))
    return var