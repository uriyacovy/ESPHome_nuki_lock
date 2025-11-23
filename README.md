# ESPHome Nuki Lock Component (ESP32) [![Build Component](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml/badge.svg)](https://github.com/uriyacovy/ESPHome_nuki_lock/actions/workflows/build.yaml)

This component brings seamless integration of Nuki Smartlocks into ESPHome, creating a rich Home Assistant lock platform with [a bunch of entities](#entities).

The lock entity updates whenever the lock's state changes - whether through the Nuki app, Home Assistant, or manually. This is achieved via the efficient Nuki BLE advertisement mechanism, ensuring your lock status is always up-to-date.

![some dashboard entites](./docs/nuki_dashboard.png)


## How to Use
To integrate your Nuki Smartlock, add one of the following code snippets to your ESPHome YAML file.

> [!WARNING]  
> This component relies on NimBLE, which is incompatible with the ESPHome BLE stack.
> Please remove all Bluetooth components (esp32_ble, esp32_improv, ...) from your configuration to use this component.

> [!WARNING]  
> You need at least **ESPHome 2025.11.0** to use this component.

> [!TIP]  
> If your ESP32 is equipped with PSRAM, you can add the `psram` component to enable the use of PSRAM for the NimBLE Stack, enhancing the reliability of this component.

### Example configuration YAML
<details>
  <summary>ESP-IDF (recommended)</summary>

```yaml
esphome:
  name: esphome-nuki-lock
  friendly_name: ESPHome Nuki Lock

esp32:
  variant: "esp32"  # Or whatever other board you're using
  framework:
    type: esp-idf

wifi:
  ssid: "SSID"
  password: "PASSWORD"

# In case you want to use the Home Assistant services
# you need to enable custom_services
# In case you want to send nuki event logs to Home Assistant
# you need to enable homeassistant_services
api:
  custom_services: true
  homeassistant_services: true

external_components:
  - source: github://uriyacovy/ESPHome_nuki_lock

lock:
  # Required
  - platform: nuki_lock
    name: Nuki Lock
  # Optional: Settings
    pairing_mode_timeout: 300s
    event: "none" # Change if you want to use event logs
    security_pin: 1234
  # Optional: Advanced Settings
    pairing_as_app: false
    query_interval_config: 3600s
    query_interval_auth_data: 7200s
    ble_general_timeout: 3s
    ble_command_timeout: 3s

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
    bt_signal_strength:
      name: "Bluetooth Signal Strength"
  # Optional: Text Sensors
    door_sensor_state:
      name: "Nuki Door Sensor: State"
    last_unlock_user:
      name: "Nuki Last Unlock User"
    last_lock_action:
      name: "Nuki Last Lock Action"
    last_lock_action_trigger:
      name: "Nuki Last Lock Action Trigger"
    pin_status:
      name: "Nuki Security Pin Status"
  # Optional: Number Inputs
    led_brightness:
      name: "Nuki LED Brightness"
    timezone_offset:
      name: "Nuki Timezone: Offset"
    lock_n_go_timeout:
      name: "Nuki LockNGo Timeout"
  # Optional: Switches
    pairing_mode:
      name: "Nuki Pairing Mode"
    auto_unlatch:
      name: "Nuki Auto unlatch"
    button_enabled:
      name: "Nuki Button: Locking operations"
    led_enabled:
      name: "Nuki LED Signal"
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
    auto_battery_type_detection_enabled:
      name: "Nuki Automatic Battery Type Detection"
    slow_speed_during_night_mode_enabled:
      name: "Nuki Slow Speed During Night Mode"
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
    battery_type:
      name: "Nuki Battery Type"
    motor_speed:
      name: "Nuki Motor Speed"
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
    on_event_log_action:
      - lambda: |-
          ESP_LOGI("nuki_lock", "Event Log (NukiLock::LogEntry) received index: %i, authId: %i", x.index, x.authId);
```
</details>

<details>
  <summary>Arduino Framework (IDF component)</summary>

```yaml
esphome:
  name: esphome-nuki-lock
  friendly_name: ESPHome Nuki Lock

esp32:
  variant: "esp32"  # Or whatever other board you're using
  framework:
    type: arduino

wifi:
  ssid: "SSID"
  password: "PASSWORD"

# In case you want to use the Home Assistant services
# you need to enable custom_services
# In case you want to send nuki event logs to Home Assistant
# you need to enable homeassistant_services
api:
  custom_services: true
  homeassistant_services: true

external_components:
  - source: github://uriyacovy/ESPHome_nuki_lock

lock:
  # Required
  - platform: nuki_lock
    name: Nuki Lock
  # Optional: Settings
    pairing_mode_timeout: 300s
    event: "none" # Change if you want to use event logs
    security_pin: 1234
  # Optional: Advanced Settings
    pairing_as_app: false
    query_interval_config: 3600s
    query_interval_auth_data: 7200s
    ble_general_timeout: 3s
    ble_command_timeout: 3s

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
    bt_signal_strength:
      name: "Bluetooth Signal Strength"
  # Optional: Text Sensors
    door_sensor_state:
      name: "Nuki Door Sensor: State"
    last_unlock_user:
      name: "Nuki Last Unlock User"
    last_lock_action:
      name: "Nuki Last Lock Action"
    last_lock_action_trigger:
      name: "Nuki Last Lock Action Trigger"
    pin_status:
      name: "Nuki Security Pin Status"
  # Optional: Number Inputs
    led_brightness:
      name: "Nuki LED Brightness"
    timezone_offset:
      name: "Nuki Timezone: Offset"
    lock_n_go_timeout:
      name: "Nuki LockNGo Timeout"
  # Optional: Switches
    pairing_mode:
      name: "Nuki Pairing Mode"
    auto_unlatch:
      name: "Nuki Auto unlatch"
    button_enabled:
      name: "Nuki Button: Locking operations"
    led_enabled:
      name: "Nuki LED Signal"
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
    auto_battery_type_detection_enabled:
      name: "Nuki Automatic Battery Type Detection"
    slow_speed_during_night_mode_enabled:
      name: "Nuki Slow Speed During Night Mode"
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
    battery_type:
      name: "Nuki Battery Type"
    motor_speed:
      name: "Nuki Motor Speed"
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
    on_event_log_action:
      - lambda: |-
          ESP_LOGI("nuki_lock", "Event Log (NukiLock::LogEntry) received index: %i, authId: %i", x.index, x.authId);
```
</details>

After running ESPHome (`esphome run <yamlfile.yaml>`), follow these steps to pair your Nuki Smartlock:

1. **Activate Pairing Mode**: Enable pairing mode on the ESPHome component. You can do this by toggling the `Pairing Mode` switch entity in Home Assistant or by triggering the `nuki_lock.set_pairing_mode` action in an automation.

2. **Set Nuki to Pairing Mode**: On your Nuki Smart Lock, press and hold the button for 5 seconds until the LED lights up to enter pairing mode.

3. **Complete Pairing**: Once paired, the ESPHome entities will automatically update with the current lock status, and pairing mode will turn off.

Your Nuki Smartlock is now connected and ready to use!

## Settings

The following settings allow you to customize the behavior of the Nuki Lock component, optimizing its performance and reliability. You can configure these in your ESPHome YAML file:

- **`security_pin`**: The Nuki security PIN required for performing specific operations (Event Logs, Auth Data, Keypad, ...).
- **`pairing_mode_timeout`**: Specifies how long (in seconds) the pairing mode remains active. Default: `300s`.
- **`event`**: Defines the event name used by the Nuki Lock component. Default: `none`.
- **`pairing_as_app`**: Determines if pairing should be done as an app. This is not recommended for most setups. Default: `false`.
- **`query_interval_config`**: Sets the interval (in seconds) for querying the configuration. Default: `3600s`.
- **`query_interval_auth_data`**: Sets the interval (in seconds) for querying authentication data. Default: `7200s`.


## Supported Services

> [!IMPORTANT]  
> In order to use the services, you have to enable them in your `api` configuration.
> Set `custom_services` to `true` as in the example below:
> ```
> api:
>   custom_services: true
> ```

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

### Action: Security Pin

> [!CAUTION]  
> Overriding the security PIN will save it to flash!  
> To revert back to the PIN defined in your YAML configuration, you must set the override PIN to `0`.

To override the security pin without recompiling, use the following action:
```yaml
on_...:
  - nuki_lock.set_security_pin:
      security_pin: 1234
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
on_event_log_action:
  - lambda: |-
      ESP_LOGI("nuki_lock", "Event Log (NukiLock::LogEntry) received index: %i, authId: %i", x.index, x.authId);
```

### Events
This component can send Nuki logs as events to Home Assistant, enabling you to use them in automations.
<b>This is disabled by default.</b>

> [!IMPORTANT]  
> In order to use the event logs, you have to enable them in your `api` configuration.
> Set `homeassistant_services` to `true` as in the example below:
> ```
> api:
>   homeassistant_services: true
> ```

> [!NOTE]
> To receive events, **you must set your security PIN**.
> Without it, it's not possible to access any event logs from your lock.

- **To Enable Logs**: Set the `event` property in your YAML configuration to something else than `none` if you want to receive log events.

- **To View Log Events**: Go to **Home Assistant Developer Tools** -> **Events**, and listen for `esphome.nuki` events to monitor log activity. You can also use the `on_event_log_action` callback to access the LogEntry which is available as `x`.

These log events provide insights into lock operations and help fine-tune automations based on lock data.
Keep in mind that the logs **are not displayed in real-time** and may take up to a minute to arrive.

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

> [!NOTE]  
> Most settings entities **require the security PIN** to make changes.  
> Without the PIN, modifying these settings is not possible.  
> Additionally, the `Last Unlock User` feature will only function if events are enabled!  

**Lock:**  
- Lock

**Binary Sensor:**  
- Is Paired
- Is Connected
- Critical Battery 
- Door Sensor

**Sensor:**
- Battery Level
- Bluetooth Signal Strength

**Text Sensor:**  
- Door Sensor State
- Last Unlock User
- Last Lock Action
- Last Lock Action Trigger
- Pin Status

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
- Single Lock
- Daylight Saving Time
- Automatic Updates
- Automatic Battery Type Detection (Smart Lock Gen 1-4)
- Slow Speed During Night Mode (Smart Lock Ultra)

**Select Input:**  
- Single Button Press Action
- Double Button Press Action
- Fob Action 1
- Fob Action 2
- Fob Action 3
- Timezone
- Advertising Mode
- Battery Type (Smart Lock Gen 1-4)
- Motor Speed (Smart Lock Ultra)

**Number Input:**  
- LED Brightness
- Timezone Offset

**Button:**  
- Unpair Device

## Dependencies
The component depends on the work done by [I-Connect](https://github.com/I-Connect) with [NukiBleEsp32](https://github.com/I-Connect/NukiBleEsp32).
We use [this fork](https://github.com/AzonInc/NukiBleEsp32/tree/idf) to maintain ESP-IDF compatibility.

## Tested Hardware
- ESP32-WROOM
- ESP32-S3-WROOM
- Nuki smart lock v4
- Nuki smart lock v3
- Nuki smart lock v2
- Nuki door sensor
