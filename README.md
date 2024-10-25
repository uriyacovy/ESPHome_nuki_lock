# ESPHome Nuki Lock Component (ESP32) [![Build Component](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml/badge.svg)](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml)

This module builds an ESPHome lock platform for Nuki Smartlocks (nuki_lock) that creates [24 entities](#entites) in Home Assistant.

The lock entity is updated whenever the look changes state (via Nuki App, HA, or manually) using the Nuki BLE advertisement mechanism.

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
  - source: github://uriyacovy/ESPHome_nuki_lock

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
    door_sensor:
      name: "Nuki Door Sensor"

    battery_level:
      name: "Nuki Battery Level"

    door_sensor_state:
      name: "Nuki Door Sensor: State"
    last_unlock_user:
      name: "Nuki Last Unlock User"

    pairing_mode:
      name: "Nuki Pairing Mode"
    auto_unlatch:
      name: "Nuki Auto unlatch"
    button_enabled:
      name: "Nuki Button: Locking operations"
    led_enabled:
      name: "Nuki LED Signal"

    led_brightness:
      name: "Nuki LED Brightness"

    night_mode_enabled:
      name: "Nuki Night Mode"
    night_mode_auto_lock_enabled:
      name: "Nuki Night Mode: Auto Lock"
    night_mode_auto_unlock_disabled:
      name: "Nuki Night Mode: Reject Auto Unlock"
    night_mode_immediate_lock_on_start_enabled:
      name: "Nuki Night Mode: Lock at Start Time"
    auto_lock_enabled:
      name: "Nuki Auto Lock"
    auto_unlock_disabled:
      name: "Nuki Auto Unlock: Disable"
    immediate_auto_lock_enabled:
      name: "Nuki Auto Lock: Immediately"
    auto_update_enabled:
      name: "Nuki Automatic Updates"
      
    single_buton_press_action:
      name: "Nuki Single Button Press Action"
    double_buton_press_action:
      name: "Nuki Double Button Press Action"
    fob_action_1:
      name: "Nuki Fob Action 1"
    fob_action_2:
      name: "Nuki Fob Action 2"
    fob_action_3:
      name: "Nuki Fob Action 3"

    unpair:
      name: "Nuki Unpair Device"

  # Optional: Settings
    security_pin: 1234
    pairing_mode_timeout: 300s
    event: "nuki"

  # Optional: Callbacks
    on_pairing_mode_on_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned on");
    on_pairing_mode_off_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned off");
    on_paired_action:
      - lambda: ESP_LOGI("nuki_lock", "Paired sucessfuly");
```

After running ESPHome (esphome run <yamlfile.yaml>), you have to activate the pairing mode of the ESPHome Component to pair your Nuki.
You can use the `Pairing Mode` Switch Entity or use the `nuki_lock.set_pairing_mode` Automation Action to do so.
To set Nuki for paring mode, press the Button on your Smart Lock for 5 seconds until the led turns on.
Once Nuki is paired, the new ESPHome entities will get the updated state and pairing mode is turned off.

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

## Automation
### Action: Pairing Mode
You can use this action to turn on/off the pairing mode: 
```yaml
on_...:
  - nuki_lock.set_pairing_mode:
      pairing_mode: True
```

### Action: Unpair
You can use this action to unpair your Nuki Smartlock: 
```yaml
on_...:
  - nuki_lock.unpair:
```

### Callbacks
You can use this callbacks to run specific actions: 
```yaml
on_pairing_mode_on_action:
  - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned on");
on_pairing_mode_off_action:
  - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned off");
on_paired_action:
  - lambda: ESP_LOGI("nuki_lock", "Paired sucessfuly");
```

### Events
By default this component sends auth eventlog events to Home Assistant.
You can use them in automations. If you want to disable events, set the `event` property in your yaml to `none`.
```yaml
event_type: esphome.nuki
data:
  device_id: 373c62d6788cf81d322763235513310e
  action: Unlatch
  authorizationId: "3902896895"
  authorizationName: Nuki ESPHome
  completionStatus: success
  index: "92"
  timeDay: "25"
  timeHour: "0"
  timeMinute: "46"
  timeMonth: "10"
  timeSecond: "11"
  timeYear: "2024"
  trigger: system
  type: LockAction
origin: LOCAL
time_fired: "2024-10-25T00:46:33.398284+00:00"
context:
  id: 01JB0J7AXPMS5DWHG188Y6XFCP
  parent_id: null
  user_id: null
```

## Entites

**Lock:**  
- Lock

**Binary Sensor:**  
- Is Paired
- Is Connected
- Critical Battery 
- Door Sensor

**Sensor:**
- Battery Level

**Text Sensor:**  
- Door Sensor State
- Last Unlock User

**Switch:**  
- Pairing Mode
- Button Enabled
- LED Enabled
- Night Mode
- Night Mode: Auto Lock
- Night Mode: Reject Auto Unlock
- Night Mode: Lock at Start Time
- Auto Lock
- Auto Unlock: Disable
- Auto Lock: Immediately
- Automatic Updates

**Select:**  
- Single Button Press Action
- Double Button Press Action

**Number:**  
- LED Brightness

**Button:**  
- Unpair Device

## Dependencies
The module depends on the work done by [I-Connect](https://github.com/I-Connect) with [NukiBleEsp32](https://github.com/I-Connect/NukiBleEsp32).

## Tested Hardware
- ESP32-WROOM
- ESP32-S3-WROOM
- Nuki smart lock v4
- Nuki smart lock v3
- Nuki smart lock v2
- Nuki door sensor

