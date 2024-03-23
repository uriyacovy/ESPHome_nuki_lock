# Nuki Lock for ESPHome (ESP32)
[![Build Component](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml/badge.svg)](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml)

This module builds an ESPHome lock platform for Nuki Smartlock (nuki_lock) that creates 9 new entities in Home Assistant:
- Lock
- Binary Sensor: Is Paired
- Binary Sensor: Is Connected
- Binary Sensor: Critical Battery
- Sensor: Battery Level
- Binary Sensor: Door Sensor
- Text Sensor: Door Sensor State
- Switch: Pairing Mode
- Button: Unpair

The lock entity is updated whenever the look changes state (via Nuki App, HA, or manually) using Nuki BT advertisement mechanism.

![dashboard](./docs/nuki_dashboard.png)

## How to use
Add the following to the ESPHome yaml file:

```yaml
esphome:
  libraries:
  - Preferences
  - https://github.com/h2zero/NimBLE-Arduino#1.4.0
  - Crc16
  - https://github.com/uriyacovy/NukiBleEsp32

external_components:
  - source: github://AzonInc/ESPHome_nuki_lock

esp32:
  board: "esp32dev"  # Or whatever other board you're using
  framework:
    type: arduino
    version: 2.0.16
    platform_version: 6.7.0

lock:
  # Required:
  - platform: nuki_lock
    name: Nuki Lock
    is_connected:
      name: "Nuki Connected"
    is_paired:
      name: "Nuki Paired"

  # Optional:
    battery_critical:
      name: "Nuki Battery Critical"
    battery_level:
      name: "Nuki Battery Level"
    door_sensor:
      name: "Nuki Door Sensor"
    door_sensor_state:
      name: "Nuki Door Sensor State"
    unpair:
      name: "Nuki Unpair"
    pairing_mode:
      name: "Nuki Pairing Mode"

  # Optional: Callbacks
    on_pairing_mode_on_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned on");
    on_pairing_mode_off_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned off");
    on_paired_action:
      - lambda: ESP_LOGI("nuki_lock", "Paired sucessfuly");
```

After running ESPHome (esphome run <yamlfile.yaml>), the module will actively try to pair to Nuki.
To set Nuki for paring mode, press the button for 5 seconds until the led turns on.
Once Nuki is paired, the new ESPHome entities will get the updated state.

## Supported Services ##
### Unlatch ###
To unlatch doors without a handle, call open service from Home Assistant:
```yaml
service: lock.open
data: {}
target:
  entity_id: lock.<NODE_NAME>
```

### Lock and Go
To run lock and go, call this service from Home Assistant:
```yaml
service: esphome.<NODE_NAME>_lock_n_go
data: {}
```

## Supported Automation Actions ##
### Pairing Mode ###
You can use automations to turn on/off the pairing mode:
```yaml
on_...:
  - nuki_lock.set_pairing_mode:
```

### Unpair
You can usea utomations to unpair your Nuki:
```yaml
on_...:
  - nuki_lock.unpair:
```

## Dependencies
The module is a Fork of [ESPHome_nuki_lock](https://github.com/uriyacovy/ESPHome_nuki_lock) and depends on the work done by [I-Connect](https://github.com/I-Connect) with [NukiBleEsp32](https://github.com/I-Connect/NukiBleEsp32).


## Tested Hardware
- ESP32 wroom
- Nuki smart lock v3
- Nuki smart lock v2
- Nuki door sensor

