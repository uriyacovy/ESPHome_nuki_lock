import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import lock, binary_sensor, text_sensor, sensor, switch
from esphome.const import CONF_ID, CONF_BATTERY_LEVEL, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_BATTERY, DEVICE_CLASS_DOOR, UNIT_PERCENT, ENTITY_CATEGORY_CONFIG

AUTO_LOAD = ["binary_sensor", "text_sensor", "sensor", "switch"]

CONF_IS_PAIRED = "is_paired"
CONF_UNPAIR = "unpair"
CONF_BATTERY_CRITICAL = "battery_critical"
CONF_BATTERY_LEVEL = "battery_level"
CONF_DOOR_SENSOR = "door_sensor"
CONF_DOOR_SENSOR_STATE = "door_sensor_state"

nuki_lock_ns = cg.esphome_ns.namespace('nuki_lock')
NukiLock = nuki_lock_ns.class_('NukiLock', lock.Lock, switch.Switch, cg.Component)

CONFIG_SCHEMA = lock.LOCK_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(NukiLock),
    cv.Optional(CONF_IS_PAIRED): binary_sensor.binary_sensor_schema(
                    device_class=DEVICE_CLASS_CONNECTIVITY,
                ),
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
    cv.Optional(CONF_UNPAIR, default=False): cv.boolean,
}).extend(cv.polling_component_schema("500ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await lock.register_lock(var, config)

    if CONF_IS_PAIRED in config:
        sens = await binary_sensor.new_binary_sensor(config[CONF_IS_PAIRED])
        cg.add(var.set_is_paired(sens))

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

    if CONF_UNPAIR in config:
        cg.add(var.set_unpair(config[CONF_UNPAIR]))
    
