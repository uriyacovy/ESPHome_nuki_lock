# ESPHome Nuki Lock Component (ESP32) [![Build Component](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml/badge.svg)](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml)

This module brings seamless integration of Nuki Smartlocks into ESPHome, creating a rich Home Assistant lock platform with [24 entities](#entities).

The lock entity updates whenever the lock's state changes - whether through the Nuki app, Home Assistant, or manually. This is achieved via the efficient Nuki BLE advertisement mechanism, ensuring your lock status is always up-to-date.

![some dashboard entites](./docs/nuki_dashboard.png)


## How to Use
To integrate your Nuki Smartlock, add the following code snippet to your ESPHome YAML file:

```yaml
external_components:
  - source: github://uriyacovy/ESPHome_nuki_lock

esp32:
  board: "esp32dev"  # Or whatever other board you're using
  framework:
    type: arduino
    version: 2.0.16
    platform_version: 6.7.0

lock:
  # Required
  - platform: nuki_lock
    name: Nuki Lock
  # Optional: Settings
    pairing_mode_timeout: 300s
    event: "nuki"
  # Optional: Binary Sensors
    is_connected:
      name: "Nuki Connected"
    is_paired:
      name: "Nuki Paired"
    battery_critical:
      name: "Nuki Battery Critical"
    door_sensor:
      name: "Nuki Door Sensor"
  # Optional: Sensors
    battery_level:
      name: "Nuki Battery Level"
  # Optional: Text Sensors
    door_sensor_state:
      name: "Nuki Door Sensor: State"
    last_unlock_user:
      name: "Nuki Last Unlock User"
    last_lock_action:
      name: "Nuki Last Lock Action"
    last_lock_action_trigger:
      name: "Nuki Last Lock Action Trigger"
  # Optional: Switches
    pairing_mode:
      name: "Nuki Pairing Mode"
    auto_unlatch:
      name: "Nuki Auto unlatch"
    button_enabled:
      name: "Nuki Button: Locking operations"
    led_enabled:
      name: "Nuki LED Signal"
  # Optional: Number Inputs
    security_pin:
      name: "Nuki Security Pin"
    led_brightness:
      name: "Nuki LED Brightness"
    timezone_offset:
      name: "Nuki Timezone: Offset"
  # Optional: Switches
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
    single_lock_enabled:
      name: "Nuki Single Lock"
    dst_mode_enabled:
      name: "Nuki Daylight Saving Time"
  # Optional: Select Inputs
    single_buton_press_action:
      name: "Nuki Button: Single Press Action"
    double_buton_press_action:
      name: "Nuki Button: Double Press Action"
    fob_action_1:
      name: "Nuki Fob: Action 1"
    fob_action_2:
      name: "Nuki Fob: Action 2"
    fob_action_3:
      name: "Nuki Fob: Action 3"
    timezone:
      name: "Nuki Timezone"
    advertising_mode:
      name: "Nuki Advertising Mode"
  # Optional: Buttons
    unpair:
      name: "Nuki Unpair Device"
  # Optional: Callbacks
    on_pairing_mode_on_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned on");
    on_pairing_mode_off_action:
      - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned off");
    on_paired_action:
      - lambda: ESP_LOGI("nuki_lock", "Paired sucessfuly");
```

After running ESPHome (`esphome run <yamlfile.yaml>`), follow these steps to pair your Nuki Smartlock:

1. **Activate Pairing Mode**: Enable pairing mode on the ESPHome component. You can do this by toggling the `Pairing Mode` switch entity in Home Assistant or by triggering the `nuki_lock.set_pairing_mode` action in an automation.

2. **Set Nuki to Pairing Mode**: On your Nuki Smart Lock, press and hold the button for 5 seconds until the LED lights up to enter pairing mode.

3. **Complete Pairing**: Once paired, the ESPHome entities will automatically update with the current lock status, and pairing mode will turn off.

Your Nuki Smartlock is now connected and ready to use!


## Supported Services
### Unlatch
To unlatch doors without a handle, call the `open` service in Home Assistant:
```yaml
service: lock.open
data: {}
target:
  entity_id: lock.<NODE_NAME>
```

### Lock and Go
To activate the Lock 'n' Go feature on your Nuki Smart Lock, call the following service in Home Assistant:

```yaml
service: esphome.<NODE_NAME>_lock_n_go
data: {}
```

### Print Keypad Entries
To print the Keypad Entries in the ESPHome Console call the following service in Home Assistant:

```yaml
service: esphome.<NODE_NAME>_print_keypad_entries
data: {}
```

### Add a Keypad Entry
To add a Keypad Entry, call the following service in Home Assistant:

```yaml
service: esphome.<NODE_NAME>_add_keypad_entry
data:
  name: "Name"
  code: 12345678
```

### Remove a Keypad Entry
To remove a Keypad Entry, call the following service in Home Assistant:

```yaml
service: esphome.<NODE_NAME>_delete_keypad_entry
data:
  id: 1
```

### Update a Keypad Entry
To update a Keypad Entry, call the following service in Home Assistant:

```yaml
service: esphome.<NODE_NAME>_update_keypad_entry
data:
  id: 1
  name: "Name"
  code: 12345678
  enabled: True
```

## ESPHome Automations
### Action: Pairing Mode
To toggle the components Pairing Mode, use the following action:
```yaml
on_...:
  - nuki_lock.set_pairing_mode:
      pairing_mode: True
```

### Action: Unpair
To unpair your Nuki Smart Lock, use the following action:
```yaml
on_...:
  - nuki_lock.unpair:
```

### Callbacks
To run specific actions when certain events occur, you can use the following callbacks:
```yaml
on_pairing_mode_on_action:
  - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned on");
on_pairing_mode_off_action:
  - lambda: ESP_LOGI("nuki_lock", "Pairing mode turned off");
on_paired_action:
  - lambda: ESP_LOGI("nuki_lock", "Paired sucessfuly");
```

### Events
By default, this component sends Nuki logs as events to Home Assistant, enabling you to use them in automations. 

- **To Disable Logs**: Set the `event` property in your YAML configuration to `none` if you don't want to receive log events.
  
- **To View Log Events**: Go to **Home Assistant Developer Tools** -> **Events**, and listen for `esphome.nuki` events to monitor log activity.

These log events provide insights into lock operations and help fine-tune automations based on real-time lock data.


Example Event:
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

## Entities

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

**Select Input:**  
- Single Button Press Action
- Double Button Press Action
- Fob Action 1
- Fob Action 2
- Fob Action 3

**Number Input:**  
- LED Brightness
- Security Pin

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

