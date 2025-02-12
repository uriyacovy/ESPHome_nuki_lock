import logging
from esphome.core import CORE
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components.esp32 import add_idf_component, add_idf_sdkconfig_option
from esphome.components import lock, binary_sensor, text_sensor, sensor, switch, button, number, select
from esphome.const import (
    CONF_ID, 
    DEVICE_CLASS_CONNECTIVITY, 
    DEVICE_CLASS_BATTERY, 
    DEVICE_CLASS_DOOR, 
    DEVICE_CLASS_SWITCH,
    DEVICE_CLASS_SIGNAL_STRENGTH,
    UNIT_PERCENT,
    UNIT_DECIBEL_MILLIWATT,
    ENTITY_CATEGORY_CONFIG, 
    ENTITY_CATEGORY_DIAGNOSTIC,
    CONF_TRIGGER_ID,
)
import esphome.final_validate as fv

LOGGER = logging.getLogger(__name__)

AUTO_LOAD = ["binary_sensor", "text_sensor", "sensor", "switch", "button", "number", "select"]

CONF_IS_CONNECTED_BINARY_SENSOR = "is_connected"
CONF_IS_PAIRED_BINARY_SENSOR = "is_paired"
CONF_BATTERY_CRITICAL_BINARY_SENSOR = "battery_critical"
CONF_DOOR_SENSOR_BINARY_SENSOR = "door_sensor"

CONF_BATTERY_LEVEL_SENSOR = "battery_level"
CONF_BT_SIGNAL_SENSOR = "bt_signal_strength"

CONF_DOOR_SENSOR_STATE_TEXT_SENSOR = "door_sensor_state"
CONF_LAST_UNLOCK_USER_TEXT_SENSOR = "last_unlock_user"
CONF_LAST_LOCK_ACTION_TEXT_SENSOR = "last_lock_action"
CONF_LAST_LOCK_ACTION_TRIGGER_TEXT_SENSOR = "last_lock_action_trigger"
CONF_PIN_STATE_TEXT_SENSOR = "pin_status"

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
CONF_SINGLE_LOCK_ENABLED_SWITCH = "single_lock_enabled"
CONF_DST_MODE_ENABLED_SWITCH = "dst_mode_enabled"
CONF_AUTO_BATTERY_TYPE_DETECTION_ENABLED_SWITCH = "auto_battery_type_detection_enabled"
CONF_SLOW_SPEED_DURING_NIGHT_MODE_ENABLED_SWITCH = "slow_speed_during_night_mode"

CONF_SINGLE_BUTTON_PRESS_ACTION_SELECT = "single_buton_press_action"
CONF_DOUBLE_BUTTON_PRESS_ACTION_SELECT = "double_buton_press_action"
CONF_FOB_ACTION_1_SELECT = "fob_action_1"
CONF_FOB_ACTION_2_SELECT = "fob_action_2"
CONF_FOB_ACTION_3_SELECT = "fob_action_3"
CONF_TIMEZONE_SELECT = "timezone"
CONF_ADVERTISING_MODE_SELECT = "advertising_mode"
CONF_BATTERY_TYPE_SELECT = "battery_type"
CONF_MOTOR_SPEED_SELECT = "motor_speed"

CONF_LED_BRIGHTNESS_NUMBER = "led_brightness"
CONF_TIMEZONE_OFFSET_NUMBER = "timezone_offset"
CONF_LOCK_N_GO_TIMEOUT_NUMBER = "lock_n_go_timeout"

CONF_BUTTON_PRESS_ACTION_SELECT_OPTIONS = [
    "No Action",
    "Intelligent",
    "Unlock",
    "Lock",
    "Unlatch",
    "Lock n Go",
    "Show Status"
]

CONF_FOB_ACTION_SELECT_OPTIONS = [
    "No Action",
    "Unlock",
    "Lock",
    "Lock n Go",
    "Intelligent"
]

CONF_MOTOR_SPEED_SELECT_OPTIONS = [
    "Standard",
    "Insane",
    "Gentle"
]

CONF_TIMEZONE_SELECT_OPTIONS = [
    "Africa/Cairo",
    "Africa/Lagos",
    "Africa/Maputo",
    "Africa/Nairobi",
    "America/Anchorage",
    "America/Argentina/Buenos_Aires",
    "America/Chicago",
    "America/Denver",
    "America/Halifax",
    "America/Los_Angeles",
    "America/Manaus",
    "America/Mexico_City",
    "America/New_York",
    "America/Phoenix",
    "America/Regina",
    "America/Santiago",
    "America/Sao_Paulo",
    "America/St_Johns",
    "Asia/Bangkok",
    "Asia/Dubai",
    "Asia/Hong_Kong",
    "Asia/Jerusalem",
    "Asia/Karachi",
    "Asia/Kathmandu",
    "Asia/Kolkata",
    "Asia/Riyadh",
    "Asia/Seoul",
    "Asia/Shanghai",
    "Asia/Tehran",
    "Asia/Tokyo",
    "Asia/Yangon",
    "Australia/Adelaide",
    "Australia/Brisbane",
    "Australia/Darwin",
    "Australia/Hobart",
    "Australia/Perth",
    "Australia/Sydney",
    "Europe/Berlin",
    "Europe/Helsinki",
    "Europe/Istanbul",
    "Europe/London",
    "Europe/Moscow",
    "Pacific/Auckland",
    "Pacific/Guam",
    "Pacific/Honolulu",
    "Pacific/Pago_Pago",
    "None"
]

CONF_ADVERTISING_MODE_SELECT_OPTIONS = [
    "Automatic",
    "Normal",
    "Slow", 
    "Slowest"
]

CONF_BATTERY_TYPE_SELECT_OPTIONS = [
    "Alkali",
    "Accumulators",
    "Lithium"
]

CONF_PAIRING_MODE_TIMEOUT = "pairing_mode_timeout"
CONF_PAIRING_AS_APP = "pairing_as_app"
CONF_SECURITY_PIN = "security_pin"
CONF_ALT_CONNECT_MODE = "alternative_connect_mode"
CONF_QUERY_INTERVAL_CONFIG = "query_interval_config"
CONF_QUERY_INTERVAL_AUTH_DATA = "query_interval_auth_data"
CONF_EVENT = "event"

CONF_SET_PAIRING_MODE = "pairing_mode"
CONF_SET_SECURITY_PIN = "security_pin"

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
NukiLockSingleLockEnabledSwitch = nuki_lock_ns.class_("NukiLockSingleLockEnabledSwitch", switch.Switch, cg.Component)
NukiLockDstModeEnabledSwitch = nuki_lock_ns.class_("NukiLockDstModeEnabledSwitch", switch.Switch, cg.Component)
NukiLockAutoBatteryTypeDetectionEnabledSwitch = nuki_lock_ns.class_("NukiLockAutoBatteryTypeDetectionEnabledSwitch", switch.Switch, cg.Component)
#NukiLockSlowSpeedDuringNightModeEnabledSwitch = nuki_lock_ns.class_("NukiLockSlowSpeedDuringNightModeEnabledSwitch", switch.Switch, cg.Component)

NukiLockLedBrightnessNumber = nuki_lock_ns.class_("NukiLockLedBrightnessNumber", number.Number, cg.Component)
NukiLockTimeZoneOffsetNumber = nuki_lock_ns.class_("NukiLockTimeZoneOffsetNumber", number.Number, cg.Component)
NukiLockLockNGoTimeoutNumber = nuki_lock_ns.class_("NukiLockLockNGoTimeoutNumber", number.Number, cg.Component)

NukiLockSingleButtonPressActionSelect = nuki_lock_ns.class_("NukiLockSingleButtonPressActionSelect", select.Select, cg.Component)
NukiLockDoubleButtonPressActionSelect = nuki_lock_ns.class_("NukiLockDoubleButtonPressActionSelect", select.Select, cg.Component)
NukiLockFobAction1Select = nuki_lock_ns.class_("NukiLockFobAction1Select", select.Select, cg.Component)
NukiLockFobAction2Select = nuki_lock_ns.class_("NukiLockFobAction2Select", select.Select, cg.Component)
NukiLockFobAction3Select = nuki_lock_ns.class_("NukiLockFobAction3Select", select.Select, cg.Component)
NukiLockTimeZoneSelect = nuki_lock_ns.class_("NukiLockTimeZoneSelect", select.Select, cg.Component)
NukiLockAdvertisingModeSelect = nuki_lock_ns.class_("NukiLockAdvertisingModeSelect", select.Select, cg.Component)
NukiLockBatteryTypeSelect = nuki_lock_ns.class_("NukiLockBatteryTypeSelect", select.Select, cg.Component)
#NukiLockMotorSpeedSelect = nuki_lock_ns.class_("NukiLockMotorSpeedSelect", select.Select, cg.Component)

NukiLockUnpairAction = nuki_lock_ns.class_(
    "NukiLockUnpairAction", automation.Action
)

NukiLockPairingModeAction = nuki_lock_ns.class_(
    "NukiLockPairingModeAction", automation.Action
)

NukiLockSecurityPinAction = nuki_lock_ns.class_(
    "NukiLockSecurityPinAction", automation.Action
)

PairingModeOnTrigger = nuki_lock_ns.class_("PairingModeOnTrigger", automation.Trigger.template())
PairingModeOffTrigger = nuki_lock_ns.class_("PairingModeOffTrigger", automation.Trigger.template())
PairedTrigger = nuki_lock_ns.class_("PairedTrigger", automation.Trigger.template())

def _validate(config):
    return config

CONFIG_SCHEMA = cv.All(
    lock.LOCK_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(NukiLock),
            cv.Optional(CONF_IS_CONNECTED_BINARY_SENSOR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:link"
            ),
            cv.Optional(CONF_IS_PAIRED_BINARY_SENSOR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_CONNECTIVITY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:link"
            ),
            cv.Optional(CONF_BATTERY_CRITICAL_BINARY_SENSOR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:battery-alert-variant-outline",
            ),
            cv.Optional(CONF_DOOR_SENSOR_BINARY_SENSOR): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_DOOR,
                icon="mdi:door-open",
            ),
            cv.Optional(CONF_DOOR_SENSOR_STATE_TEXT_SENSOR): text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:door-open",
            ),
            cv.Optional(CONF_LAST_UNLOCK_USER_TEXT_SENSOR):  text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:account-clock"
            ),
            cv.Optional(CONF_LAST_LOCK_ACTION_TEXT_SENSOR):  text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:account-clock"
            ),
            cv.Optional(CONF_PIN_STATE_TEXT_SENSOR):  text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:shield-key"
            ),
            cv.Optional(CONF_LAST_LOCK_ACTION_TRIGGER_TEXT_SENSOR):  text_sensor.text_sensor_schema(
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                icon="mdi:account-clock"
            ),
            cv.Optional(CONF_BATTERY_LEVEL_SENSOR): sensor.sensor_schema(
                device_class=DEVICE_CLASS_BATTERY,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                unit_of_measurement=UNIT_PERCENT,
                icon="mdi:battery-50",
            ),
            cv.Optional(CONF_BT_SIGNAL_SENSOR): sensor.sensor_schema(
                device_class=DEVICE_CLASS_SIGNAL_STRENGTH,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
                unit_of_measurement=UNIT_DECIBEL_MILLIWATT,
                icon="mdi:bluetooth-audio"
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
            cv.Optional(CONF_SINGLE_LOCK_ENABLED_SWITCH): switch.switch_schema(
                NukiLockSingleLockEnabledSwitch,
                device_class=DEVICE_CLASS_SWITCH,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:lock-plus",
            ),
            cv.Optional(CONF_DST_MODE_ENABLED_SWITCH): switch.switch_schema(
                NukiLockDstModeEnabledSwitch,
                device_class=DEVICE_CLASS_SWITCH,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:sun-clock",
            ),
            cv.Optional(CONF_AUTO_BATTERY_TYPE_DETECTION_ENABLED_SWITCH): switch.switch_schema(
                NukiLockAutoBatteryTypeDetectionEnabledSwitch,
                device_class=DEVICE_CLASS_SWITCH,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:battery-check",
            ),
            #cv.Optional(CONF_SLOW_SPEED_DURING_NIGHT_MODE_ENABLED_SWITCH): switch.switch_schema(
            #    NukiLockSlowSpeedDuringNightModeEnabledSwitch,
            #    device_class=DEVICE_CLASS_SWITCH,
            #    entity_category=ENTITY_CATEGORY_CONFIG,
            #    icon="mdi:speedometer-slow",
            #),
            cv.Optional(CONF_LED_BRIGHTNESS_NUMBER): number.number_schema(
                NukiLockLedBrightnessNumber,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:brightness-6",
            ),
            cv.Optional(CONF_TIMEZONE_OFFSET_NUMBER): number.number_schema(
                NukiLockTimeZoneOffsetNumber,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:clock-end",
            ),
            cv.Optional(CONF_LOCK_N_GO_TIMEOUT_NUMBER): number.number_schema(
                NukiLockLockNGoTimeoutNumber,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:clock-end",
            ),
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
            cv.Optional(CONF_TIMEZONE_SELECT): select.select_schema(
                NukiLockTimeZoneSelect,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:map-clock",
            ),
            cv.Optional(CONF_ADVERTISING_MODE_SELECT): select.select_schema(
                NukiLockAdvertisingModeSelect,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:timer-cog",
            ),
            cv.Optional(CONF_BATTERY_TYPE_SELECT): select.select_schema(
                NukiLockBatteryTypeSelect,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:battery",
            ),
            #cv.Optional(CONF_MOTOR_SPEED_SELECT): select.select_schema(
            #    NukiLockMotorSpeedSelect,
            #    entity_category=ENTITY_CATEGORY_CONFIG,
            #    icon="mdi:speedometer-medium",
            #),
            cv.Optional(CONF_ALT_CONNECT_MODE, default="true"): cv.boolean,
            cv.Optional(CONF_PAIRING_AS_APP, default="false"): cv.boolean,
            cv.Optional(CONF_PAIRING_MODE_TIMEOUT, default="300s"): cv.positive_time_period_seconds,
            cv.Optional(CONF_EVENT, default="nuki"): cv.string,
            cv.Optional(CONF_SECURITY_PIN): cv.uint16_t,
            cv.Optional(CONF_QUERY_INTERVAL_CONFIG, default="3600s"): cv.positive_time_period_seconds,
            cv.Optional(CONF_QUERY_INTERVAL_AUTH_DATA, default="3600s"): cv.positive_time_period_seconds,
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
        }
    )
    .extend(cv.polling_component_schema("500ms")),
    _validate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await lock.register_lock(var, config)

    # Component Settings
    if CONF_PAIRING_MODE_TIMEOUT in config:
        cg.add(var.set_pairing_mode_timeout(config[CONF_PAIRING_MODE_TIMEOUT]))

    if CONF_EVENT in config:
        cg.add(var.set_event("esphome." + config[CONF_EVENT]))
        
    if CONF_SECURITY_PIN in config:
        cg.add(var.set_security_pin(config[CONF_SECURITY_PIN]))
        
    if CONF_PAIRING_AS_APP in config:
        cg.add(var.set_pairing_as_app(config[CONF_PAIRING_AS_APP]))
        
        if config[CONF_ALT_CONNECT_MODE]:
            cg.add_define("NUKI_ALT_CONNECT")

    if CONF_ALT_CONNECT_MODE in config:
        cg.add(var.set_alt_connect_mode(config[CONF_ALT_CONNECT_MODE]))

    if CONF_QUERY_INTERVAL_CONFIG in config:
        cg.add(var.set_query_interval_config(config[CONF_QUERY_INTERVAL_CONFIG]))

    if CONF_QUERY_INTERVAL_AUTH_DATA in config:
        cg.add(var.set_query_interval_auth_data(config[CONF_QUERY_INTERVAL_AUTH_DATA]))

    # Binary Sensor
    if is_connected := config.get(CONF_IS_CONNECTED_BINARY_SENSOR):
        sens = await binary_sensor.new_binary_sensor(is_connected)
        cg.add(var.set_is_connected_binary_sensor(sens))

    if is_paired := config.get(CONF_IS_PAIRED_BINARY_SENSOR):
        sens = await binary_sensor.new_binary_sensor(is_paired)
        cg.add(var.set_is_paired_binary_sensor(sens))

    if battery_critical := config.get(CONF_BATTERY_CRITICAL_BINARY_SENSOR):
        sens = await binary_sensor.new_binary_sensor(battery_critical)
        cg.add(var.set_battery_critical_binary_sensor(sens))

    if door_sensor := config.get(CONF_DOOR_SENSOR_BINARY_SENSOR):
        sens = await binary_sensor.new_binary_sensor(door_sensor)
        cg.add(var.set_door_sensor_binary_sensor(sens))

    # Sensor
    if battery_level := config.get(CONF_BATTERY_LEVEL_SENSOR):
        sens = await sensor.new_sensor(battery_level)
        cg.add(var.set_battery_level_sensor(sens))

    if bt_signal := config.get(CONF_BT_SIGNAL_SENSOR):
        sens = await sensor.new_sensor(bt_signal)
        cg.add(var.set_bt_signal_sensor(sens))

    # Text Sensor
    if door_sensor_state := config.get(CONF_DOOR_SENSOR_STATE_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(door_sensor_state)
        cg.add(var.set_door_sensor_state_text_sensor(sens))

    if last_unlock_user := config.get(CONF_LAST_UNLOCK_USER_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(last_unlock_user)
        cg.add(var.set_last_unlock_user_text_sensor(sens))

    if last_lock_action_trigger := config.get(CONF_LAST_LOCK_ACTION_TRIGGER_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(last_lock_action_trigger)
        cg.add(var.set_last_lock_action_trigger_text_sensor(sens))

    if last_lock_action := config.get(CONF_LAST_LOCK_ACTION_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(last_lock_action)
        cg.add(var.set_last_lock_action_text_sensor(sens))

    if pin_state := config.get(CONF_PIN_STATE_TEXT_SENSOR):
        sens = await text_sensor.new_text_sensor(pin_state)
        cg.add(var.set_pin_state_text_sensor(sens))

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

    if timezone_offset := config.get(CONF_TIMEZONE_OFFSET_NUMBER):
        n = await number.new_number(
            timezone_offset, min_value=-60, max_value=60, step=1
        )
        await cg.register_parented(n, config[CONF_ID])
        cg.add(var.set_timezone_offset_number(n))

    if lock_n_go_timeout := config.get(CONF_LOCK_N_GO_TIMEOUT_NUMBER):
        n = await number.new_number(
            lock_n_go_timeout, min_value=5, max_value=60, step=1
        )
        await cg.register_parented(n, config[CONF_ID])
        cg.add(var.set_lock_n_go_timeout_number(n))

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

    if single_lock_enabled := config.get(CONF_SINGLE_LOCK_ENABLED_SWITCH):
        s = await switch.new_switch(single_lock_enabled)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_single_lock_enabled_switch(s))

    if dst_mode := config.get(CONF_DST_MODE_ENABLED_SWITCH):
        s = await switch.new_switch(dst_mode)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_dst_mode_enabled_switch(s))

    if auto_battery_type_detection := config.get(CONF_AUTO_BATTERY_TYPE_DETECTION_ENABLED_SWITCH):
        s = await switch.new_switch(auto_battery_type_detection)
        await cg.register_parented(s, config[CONF_ID])
        cg.add(var.set_auto_battery_type_detection_enabled_switch(s))

    #if slow_speed_during_night_mode := config.get(CONF_SLOW_SPEED_DURING_NIGHT_MODE_ENABLED_SWITCH):
    #    s = await switch.new_switch(slow_speed_during_night_mode)
    #    await cg.register_parented(s, config[CONF_ID])
    #    cg.add(var.set_slow_speed_during_night_mode_enabled_switch(s))

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

    if timezone := config.get(CONF_TIMEZONE_SELECT):
        sel = await select.new_select(
            timezone,
            options=[CONF_TIMEZONE_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_timezone_select(sel))

    if advertising_mode := config.get(CONF_ADVERTISING_MODE_SELECT):
        sel = await select.new_select(
            advertising_mode,
            options=[CONF_ADVERTISING_MODE_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_advertising_mode_select(sel))

    if battery_type := config.get(CONF_BATTERY_TYPE_SELECT):
        sel = await select.new_select(
            battery_type,
            options=[CONF_BATTERY_TYPE_SELECT_OPTIONS],
        )
        await cg.register_parented(sel, config[CONF_ID])
        cg.add(var.set_battery_type_select(sel))

    #if motor_speed := config.get(CONF_MOTOR_SPEED_SELECT):
    #    sel = await select.new_select(
    #        motor_speed,
    #        options=[CONF_MOTOR_SPEED_SELECT_OPTIONS],
    #    )
    #    await cg.register_parented(sel, config[CONF_ID])
    #    cg.add(var.set_motor_speed_select(sel))


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
    if CORE.using_esp_idf:
        add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
        add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ENABLED", True)
        add_idf_sdkconfig_option("CONFIG_BT_BLUEDROID_ENABLED", False)

        add_idf_sdkconfig_option("CONFIG_BTDM_BLE_SCAN_DUPL", True)
        add_idf_sdkconfig_option("CONFIG_NIMBLE_CPP_LOG_LEVEL", 0)
        add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_LOG_LEVEL", 0)
        add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_LOG_LEVEL_NONE", True)

        add_idf_component(
            name="NukiBleEsp32",
            repo="https://github.com/AzonInc/NukiBleEsp32.git",
            ref="idf",
        )
    else:
        cg.add_build_flag("-DCONFIG_BTDM_BLE_SCAN_DUPL=y")
        cg.add_build_flag("-DCONFIG_NIMBLE_CPP_LOG_LEVEL=0")
        cg.add_build_flag("-DCONFIG_BT_NIMBLE_LOG_LEVEL=0")
        cg.add_build_flag("-DCONFIG_BT_NIMBLE_LOG_LEVEL_NONE=y")

        cg.add_library("Preferences", None)
        cg.add_library("h2zero/NimBLE-Arduino", "1.4.2")
        cg.add_library("Crc16", None)
        cg.add_library(
            None,
            None,
            "https://github.com/I-Connect/NukiBleEsp32#940d809",
        )


    # Defines
    cg.add_define("NUKI_MUTEX_RECURSIVE")
    cg.add_define("NUKI_NO_WDT_RESET")

    # Remove Build flags
    cg.add_platformio_option(
        "build_unflags",
        [
            f"-DCONFIG_BTDM_BLE_SCAN_DUPL",
            f"-DCONFIG_BT_NIMBLE_LOG_LEVEL",
            f"-DCONFIG_NIMBLE_CPP_LOG_LEVEL",
            f"-Werror=all",
            f"-Wall",
        ],
    )

    # Build flags
    cg.add_build_flag("-Wno-unused-result")
    cg.add_build_flag("-Wno-ignored-qualifiers")
    cg.add_build_flag("-Wno-missing-field-initializers")
    cg.add_build_flag("-Wno-maybe-uninitialized")


def _final_validate(config):
    full_config = fv.full_config.get()

    incompatible_components = [
        "esp32_ble", 
        "esp32_improv", 
        "esp32_ble_beacon", 
        "esp32_ble_client", 
        "esp32_ble_tracker", 
        "esp32_ble_server"
    ]

    if CORE.is_esp32:
        # Check if any of the incompatible components are in the configuration
        if any(component in full_config for component in incompatible_components):
            raise cv.Invalid(f"The `nuki_lock` component relies on NimBLE, which is incompatible with the ESPHome BLE stack.\nTo use `nuki_lock`, please remove all Bluetooth components (esp32_ble, esp32_improv, ...) from your configuration.")
        
        # Check for PSRAM support
        if "psram" in full_config:
            if CORE.using_esp_idf:
                add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL", True)
                add_idf_sdkconfig_option("CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL", 50768)
                add_idf_sdkconfig_option("CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST", True)
                add_idf_sdkconfig_option("CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY", True)
            else:
                cg.add_build_flag(f"-DCONFIG_BT_NIMBLE_MEM_ALLOC_MODE_EXTERNAL=1")
                cg.add_build_flag(f"-DCONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=50768")
                cg.add_build_flag(f"-DCONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST=1")
                cg.add_build_flag(f"-DCONFIG_BT_BLE_DYNAMIC_ENV_MEMORY=1")
        else:
            LOGGER.info("Consider enabling PSRAM support if it's available for the NimBLE Stack.")

        # Check for API encryption
        if "api" in full_config:
            if "encryption" in full_config["api"]:
                LOGGER.warning("You may need to disable API encryption to successfully pair with the Nuki Smart Lock, as it consumes quite a bit of memory.")
    
    return config

FINAL_VALIDATE_SCHEMA = _final_validate


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




NUKI_LOCK_SET_SECURITY_PIN_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(NukiLock),
        cv.Required(CONF_SET_SECURITY_PIN): cv.templatable(cv.uint16_t)
    }
)

@automation.register_action(
    "nuki_lock.set_security_pin", NukiLockSecurityPinAction, NUKI_LOCK_SET_SECURITY_PIN_SCHEMA
)

async def nuki_lock_set_security_pin_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    security_pin_template_ = await cg.templatable(config[CONF_SET_SECURITY_PIN], args, cg.uint16)
    cg.add(var.set_security_pin(security_pin_template_))
    return var