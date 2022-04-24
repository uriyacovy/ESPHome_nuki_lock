# Nuki Lock for ESPHome (ESP32)
This module builds an ESPHome lock platform for Nuki Smartlock (nuki_lock) that creates 4 new entities in Home Assistant:
- Lock 
- Binary Sensor: Critical Battery 
- Sensor: Battery Level
- Binary Sensor: Is Paired 

The lock entity is updated whenever the look changes state (via Nuki App, HA, or manually) using Nuki BT advertisement mechanism.

## How to use
Add the following to the ESPHome yaml file:
external_components:
```
external_components:
  - source: github://uriyacovy/ESPHome_nuki_lock

lock:
  - platform: nuki_lock
    name: Nuki Lock
    is_paired: 
      name: "Nuki Paired"
    battery_critical:
      name: "Nuki Battery Critical"
    battery_level:
      name: "Nuki Battery Level"
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
The module depends on the work done by https://github.com/I-Connect, https://github.com/I-Connect/NukiBleEsp32

This library requires also https://github.com/nkolban/ESP32_BLE_Arduino/

## Tested Hardware
- ESP32 wroom
- Nuki smart lock v3

