# Nuki Lock for ESPHome (ESP32)
This module builds an ESPHome lock platform for Nuki Smartlock (nuki_lock) that creates 6 new entities in Home Assistant:
- Lock 
- Binary Sensor: Is Paired
- Binary Sensor: Critical Battery 
- Sensor: Battery Level
- Binary Sensor: Door Sensor
- Text Sensor: Door Sensor State

The lock entity is updated whenever the look changes state (via Nuki App, HA, or manually) using Nuki BT advertisement mechanism.

![screenshot](https://user-images.githubusercontent.com/1754967/183266065-d1a6e9fe-d7f7-4295-9c0d-4bf9235bf4cd.png)

## How to use
Add the following to the ESPHome yaml file:

```
esphome:

  libraries:
  - Preferences
  - https://github.com/nkolban/ESP32_BLE_Arduino/
  - https://github.com/uriyacovy/NukiBleEsp32

external_components:
  - source: github://uriyacovy/ESPHome_nuki_lock

lock:
  # Required:
  - platform: nuki_lock
    name: Nuki Lock
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
```

After running ESPHome (esphome run <yamlfile.yaml>), the module will actively try to pair to Nuki.
To set Nuki for paring mode, press the button for 5 seconds until the led turns on.
Once Nuki is paired, the new ESPHome entities will get the updated state.

## Unparing Nuki
To unpair Nuki, add the following to ESPHome yaml file below `platform: nuki_lock` section and run ESPHome again:
```
    unpair: true
```

## Dependencies
The module depends on the work done by [I-Connect](https://github.com/I-Connect), https://github.com/I-Connect/NukiBleEsp32

This library requires also https://github.com/nkolban/ESP32_BLE_Arduino/ and Arduino library Preferences.

## Tested Hardware
- ESP32 wroom
- Nuki smart lock v3
- Nuki smart lock v2
- Nuki door sensor

