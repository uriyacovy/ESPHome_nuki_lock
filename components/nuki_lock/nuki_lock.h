#pragma once

#include <functional>

#include "esphome/core/component.h"
#include "esphome/components/lock/lock.h"
#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif

#include "nuki_lock_protocol.h"
#include "nuki_ble_constants.h"

#include "nuki_lock_utils.h"

namespace esphome::nuki_lock {

static const uint8_t BLE_CONNECT_TIMEOUT_SEC = 2;
static const uint8_t BLE_CONNECT_RETRIES = 5;

static const uint16_t BLE_DISCONNECT_TIMEOUT = 2000;

static const uint8_t MAX_TOLERATED_UPDATES_ERRORS = 5;

static const uint32_t COOLDOWN_COMMANDS_MILLIS = 1000;
static const uint32_t COOLDOWN_COMMANDS_EXTENDED_MILLIS = 3000;

static const uint8_t MAX_AUTH_DATA_ENTRIES = 10;
static const uint8_t MAX_EVENT_LOG_ENTRIES = 3;

static const uint8_t MAX_NAME_LEN = 32;

struct AuthEntry {
    uint32_t authId;
    char name[MAX_NAME_LEN];
};

struct NukiLockSettings
{
    uint32_t security_pin;
    PinState pin_state;
};

class NukiLockComponent :
    public lock::Lock,
    public Component,
    public SmartlockEventHandler
#ifdef USE_API
    , public api::CustomAPIDevice
#endif
    {
    #ifdef USE_BINARY_SENSOR
    SUB_BINARY_SENSOR(connected)
    SUB_BINARY_SENSOR(paired)
    SUB_BINARY_SENSOR(battery_critical)
    SUB_BINARY_SENSOR(battery_charging)
    SUB_BINARY_SENSOR(keypad_battery_critical)
    SUB_BINARY_SENSOR(door_sensor_battery_critical)
    SUB_BINARY_SENSOR(door_sensor)
    SUB_BINARY_SENSOR(remote_access_connected)
    #endif
    #ifdef USE_SENSOR
    SUB_SENSOR(battery_level)
    SUB_SENSOR(battery_voltage)
    SUB_SENSOR(battery_drain)
    SUB_SENSOR(motor_current)
    SUB_SENSOR(bt_signal)
    SUB_SENSOR(wifi_connection_strength)
    #endif
    #ifdef USE_TEXT_SENSOR
    SUB_TEXT_SENSOR(door_sensor_state)
    SUB_TEXT_SENSOR(last_unlock_user)
    SUB_TEXT_SENSOR(last_lock_action)
    SUB_TEXT_SENSOR(last_lock_action_trigger)
    SUB_TEXT_SENSOR(pin_state)
    SUB_TEXT_SENSOR(wifi_connection_status)
    SUB_TEXT_SENSOR(mqtt_connection_status)
    SUB_TEXT_SENSOR(thread_connection_status)
    #endif
    #ifdef USE_NUMBER
    SUB_NUMBER(led_brightness)
    SUB_NUMBER(timezone_offset)
    SUB_NUMBER(lock_n_go_timeout)
    SUB_NUMBER(auto_lock_timeout)
    SUB_NUMBER(unlatch_duration)
    SUB_NUMBER(unlocked_position_offset)
    SUB_NUMBER(locked_position_offset)
    SUB_NUMBER(single_locked_position_offset)
    SUB_NUMBER(unlocked_to_locked_transition_offset)
    #endif
    #ifdef USE_SELECT
    SUB_SELECT(single_button_press_action)
    SUB_SELECT(double_button_press_action)
    SUB_SELECT(fob_action_1)
    SUB_SELECT(fob_action_2)
    SUB_SELECT(fob_action_3)
    SUB_SELECT(timezone)
    SUB_SELECT(advertising_mode)
    SUB_SELECT(battery_type)
    SUB_SELECT(motor_speed)
    #endif
    #ifdef USE_BUTTON
    SUB_BUTTON(unpair)
    SUB_BUTTON(request_calibration)
    #endif
    #ifdef USE_SWITCH
    SUB_SWITCH(pairing_mode)
    SUB_SWITCH(pairing_enabled)
    SUB_SWITCH(button_enabled)
    SUB_SWITCH(auto_unlatch_enabled)
    SUB_SWITCH(led_enabled)
    SUB_SWITCH(nightmode_enabled)
    SUB_SWITCH(night_mode_auto_lock_enabled)
    SUB_SWITCH(night_mode_auto_unlock_disabled)
    SUB_SWITCH(night_mode_immediate_lock_on_start)
    SUB_SWITCH(auto_lock_enabled)
    SUB_SWITCH(auto_unlock_disabled)
    SUB_SWITCH(immediate_auto_lock_enabled)
    SUB_SWITCH(auto_update_enabled)
    SUB_SWITCH(single_lock_enabled)
    SUB_SWITCH(dst_mode_enabled)
    SUB_SWITCH(auto_battery_type_detection_enabled)
    SUB_SWITCH(slow_speed_during_night_mode_enabled)
    SUB_SWITCH(detached_cylinder_enabled)
    SUB_SWITCH(logging_enabled)
    #endif

    public:
        // device_name identifies this ESP32 to the lock during pairing (shown in the Nuki
        // app's authorization list) and seeds the BLE credential and settings storage keys.
        // It must be unique per instance - callers pass the YAML id, which ESPHome already
        // guarantees is unique - so multiple nuki_lock: blocks on one device don't clobber
        // each other's pairing credentials/PIN settings in flash.
        explicit NukiLockComponent(const std::string &device_name)
            : Lock(), device_name_(device_name), nuki_lock_(device_name, esphome::fnv1_hash(device_name)) {}

        // ESPHome overrides
        void setup() override;
        void loop() override;
        void dump_config() override;
        float get_setup_priority() const override { return setup_priority::HARDWARE; }

        // NukiBLE overrides
        void notify(EventType event_type) override;

        // Configuration setters
        void set_pairing_mode_timeout(uint32_t pairing_mode_timeout) { this->pairing_mode_timeout_ = pairing_mode_timeout; }
        void set_query_interval_config(uint32_t query_interval_config) { this->query_interval_config_ = query_interval_config; }
        void set_query_interval_auth_data(uint32_t query_interval_auth_data) { this->query_interval_auth_data_ = query_interval_auth_data; }
        void set_query_interval_battery_report(uint32_t query_interval_battery_report) { this->query_interval_battery_report_ = query_interval_battery_report; }
        void set_ble_general_timeout(uint32_t ble_general_timeout) { this->ble_general_timeout_ = ble_general_timeout; }
        void set_ble_command_timeout(uint32_t ble_command_timeout) { this->ble_command_timeout_ = ble_command_timeout; }
        // Applies to lock actions (lock/unlock/...) and queued commands (switches, numbers,
        // selects, keypad management) alike - see execute_lock_action_step()/
        // process_pending_nuki_command() in nuki_lock.cpp.
        void set_command_retries(uint8_t command_retries) { this->command_retries_ = command_retries; }
        void set_command_retry_delay(uint32_t command_retry_delay_millis) { this->command_retry_delay_millis_ = command_retry_delay_millis; }
        void set_config_cache_ttl(uint32_t config_cache_ttl) { this->config_cache_ttl_ = config_cache_ttl; }
        void set_event(const char *event) {
            this->event_ = event;
            if(strcmp(event, "esphome.none") != 0) {
                this->send_events_ = true;
            }
        }
        void set_security_pin(uint32_t security_pin);
        void set_pairing_mode(bool enabled);

        // Template configuration setters
        template<typename T> void set_security_pin_config(T security_pin_config) { this->security_pin_config_ = security_pin_config; }
        template<typename T> void set_pairing_as_app(T pairing_as_app) { this->pairing_as_app_ = pairing_as_app; }

        // Callback registration & managers
        template<typename F> void add_pairing_mode_on_callback(F &&callback)
        {
            this->pairing_mode_on_callback_.add(std::forward<F>(callback));
        }

        template<typename F> void add_pairing_mode_off_callback(F &&callback)
        {
            this->pairing_mode_off_callback_.add(std::forward<F>(callback));
        }

        template<typename F> void add_paired_callback(F &&callback)
        {
            this->paired_callback_.add(std::forward<F>(callback));
        }

        template<typename F> void add_event_log_received_callback(F &&callback)
        {
            this->event_log_received_callback_.add(std::forward<F>(callback));
        }

        void unpair();
        void save_settings();
        void request_calibration();
        void setup_lock(bool new_pairing = false);

        bool is_connected() { return this->connected_; }
        bool is_paired() { return this->nuki_lock_.is_paired_with_lock(); }

        void lock_n_go();
        void print_keypad_entries();
        void add_keypad_entry(std::string name, int32_t code);
        void update_keypad_entry(int32_t id, std::string name, int32_t code, bool enabled);
        void delete_keypad_entry(int32_t id);

        // Sets one-shot modifiers applied to the *next* lock/unlock/unlatch action only (then
        // reset to defaults) - see Lock Action's Flags (Force/Auto Unlock) and Name suffix in
        // the Nuki Smart Lock API. force bypasses checks like the "too recent" cooldown;
        // auto_unlock marks the action as an automatic/auto-unlock trigger in the Nuki app's
        // history; name_suffix (max 19 characters) is appended to that history entry.
        void set_lock_action_options(bool force, bool auto_unlock, std::string name_suffix);

        NukiLock* get_nuki_lock() { return &this->nuki_lock_; }
        Config* get_nuki_lock_config() { return &this->nuki_lock_config_; }
        AdvancedConfig* get_nuki_lock_advanced_config() { return &this->nuki_lock_advanced_config_; }

        // Queues a Nuki command (typically a `nuki_lock_.setXxx(...)` call) to run on the
        // single in-flight Nuki BLE command slot. `command` is called repeatedly, once per
        // loop() tick, until it returns something other than CmdResult::Working; a non-
        // Success result is retried (up to command_retries_ times, command_retry_delay_millis_
        // apart) before giving up. `on_done` is then called once with the terminal result.
        // Only one queued command (and at most one other Nuki operation: a lock action,
        // status/config poll, etc.) can be in flight at a time - see nuki_op_active_ in loop().
        void queue_nuki_command(std::function<CmdResult()> command, std::function<void(CmdResult)> on_done);

    protected:
        CallbackManager<void()> pairing_mode_on_callback_;
        CallbackManager<void()> pairing_mode_off_callback_;
        CallbackManager<void()> paired_callback_;
        CallbackManager<void(LogEntry)> event_log_received_callback_;

        void control(const lock::LockCall &call) override;
        void open_latch() override { this->open_latch_ = true; unlock();}

    private:
        // Loop timing: loop() throttles itself to run its main body at most every 500ms
        // instead of blocking, since it is called by ESPHome's scheduler on every
        // Application::loop() iteration.
        uint32_t last_loop_time_{0};

        // Only one Nuki BLE command can be in flight at a time (the underlying NukiBle
        // command state machine has a single nuki_command_state_). nuki_op_active_/
        // nuki_op_step_ guard against starting a *different* operation while one is
        // already mid-flight, which would corrupt that shared state: once a tick starts
        // an operation (a lock action, a status/config/auth-data/event-log poll, or a
        // queued command), the same step function is re-invoked on every following tick
        // until it reaches a terminal CmdResult, regardless of which other flags become
        // true in the meantime.
        bool nuki_op_active_{false};
        std::function<void()> nuki_op_step_;

        std::function<CmdResult()> pending_nuki_command_;
        std::function<void(CmdResult)> pending_nuki_command_done_;
        void process_pending_nuki_command();

        // Core components
        ESPPreferenceObject pref_;
        std::string device_name_;
        NukiLock nuki_lock_;

        // Nuki state & configuration
        KeyTurnerState retrieved_key_turner_state_;
        LockAction lock_action_;
        Config nuki_lock_config_;
        AdvancedConfig nuki_lock_advanced_config_;
        BatteryReport battery_report_;

        // Methods to retrieve or set data. Each performs one step per call and is safe
        // to call repeatedly (once per loop() tick) until it completes - see
        // nuki_op_active_ above.
        void update_status();
        void update_config();
        void update_advanced_config();
        void update_event_logs();
        void update_auth_data();
        void update_battery_report();
        void validate_pin();
        void validate_pin_step();
        void execute_lock_action_step();

        // Set (along with action_attempts_) by control(); execute_lock_action_step()
        // copies lock_action_ into executing_lock_action_ when it starts a fresh attempt
        // so a *new* control() call received while an attempt is still mid-flight doesn't
        // change which action the in-flight BLE command is for.
        LockAction executing_lock_action_;
        bool lock_action_in_flight_{false};
        // Snapshots of force_/auto_unlock_/name_suffix_ taken at the same time as
        // executing_lock_action_, for the same reason - see set_lock_action_options().
        uint8_t executing_lock_action_flags_{0};
        std::string executing_lock_action_name_suffix_;

        // Setup & utility methods
        void setup_intervals(bool setup = true);
        void publish_pin_state();
        void process_auth_data();
        void process_log_entries();

        // PIN management
        PinState pin_state_ = PinState::NotSet;
        uint32_t security_pin_ = 0;
        uint32_t pin_validation_start_time_ = 0;
        uint8_t pin_validation_attempts_ = 0;
        bool pin_validation_pending_{false};
        TemplatableValue<uint32_t> security_pin_config_{};

        // Authorization Entries
        AuthEntry auth_entries_[MAX_AUTH_DATA_ENTRIES];
        size_t auth_entries_count_ = 0;
        uint32_t auth_id_ = 0;
        char auth_name_[33] = {0};
        const char* get_auth_name(uint32_t authId) const;
        uint32_t auth_data_ready_time_ = 0;

        // Keypad management
        bool valid_keypad_id(int32_t id);
        bool valid_keypad_name(std::string name);
        bool valid_keypad_code(int32_t code);
        bool keypad_paired_{false};
        std::vector<uint16_t> keypad_code_ids_;

        // Connection & State flags
        bool connected_{false};
        bool pairing_mode_{false};
        bool send_events_{false};
        TemplatableValue<bool> pairing_as_app_{};

        // Update flags
        bool status_update_{false};
        bool config_update_{false};
        bool advanced_config_update_{false};
        bool auth_data_update_{false};
        bool event_log_update_{false};
        bool battery_report_update_{false};

        // Action flags
        bool open_latch_{false};
        bool lock_n_go_{false};

        // One-shot Lock Action modifiers set via set_lock_action_options(), consumed and reset
        // by execute_lock_action_step() the moment it snapshots a new attempt (see
        // executing_lock_action_flags_/executing_lock_action_name_suffix_ below).
        bool force_{false};
        bool auto_unlock_{false};
        std::string name_suffix_;

        // Error tracking & counters
        uint8_t action_attempts_ = 0;
        // How many retries remain for the currently-pending queue_nuki_command() command -
        // separate from action_attempts_, which tracks lock action retries instead.
        uint8_t pending_command_retries_left_ = 0;
        uint32_t status_update_consecutive_errors_ = 0;

        // Timing & Intervals
        uint32_t last_command_executed_time_ = 0;
        uint32_t command_cooldown_millis = 0;
        uint32_t query_interval_auth_data_ = 0;
        uint32_t query_interval_config_ = 0;
        uint32_t query_interval_battery_report_ = 0;
        uint32_t ble_general_timeout_ = 0;
        uint32_t ble_command_timeout_ = 0;
        uint32_t pairing_mode_timeout_ = 0;
        uint8_t command_retries_ = 5;
        uint32_t command_retry_delay_millis_ = 1000;
        uint32_t config_cache_ttl_ = 60;

        // Event Logs
        uint32_t last_rolling_log_id = 0;
        uint32_t event_log_ready_time_ = 0;
        const char* event_;
};

// Entities
#ifdef USE_BUTTON
class NukiLockUnpairButton : public button::Button, public Parented<NukiLockComponent> {
    public:
        NukiLockUnpairButton() = default;
    protected:
        void press_action() override;
};

class NukiLockRequestCalibrationButton : public button::Button, public Parented<NukiLockComponent> {
    public:
        NukiLockRequestCalibrationButton() = default;
    protected:
        void press_action() override;
};
#endif

#ifdef USE_SELECT
class NukiLockSingleButtonPressActionSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockSingleButtonPressActionSelect() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockDoubleButtonPressActionSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockDoubleButtonPressActionSelect() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockFobAction1Select : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockFobAction1Select() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockFobAction2Select : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockFobAction2Select() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockFobAction3Select : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockFobAction3Select() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockTimeZoneSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockTimeZoneSelect() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockAdvertisingModeSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockAdvertisingModeSelect() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockBatteryTypeSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockBatteryTypeSelect() = default;
    protected:
        void control(const std::string &value) override;
};

class NukiLockMotorSpeedSelect : public select::Select, public Parented<NukiLockComponent> {
    public:
        NukiLockMotorSpeedSelect() = default;
    protected:
        void control(const std::string &value) override;
};
#endif

#ifdef USE_SWITCH
class NukiLockPairingModeSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockPairingModeSwitch() = default;
    protected:
        void write_state(bool state) override;
};

class NukiLockPairingEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockPairingEnabledSwitch() = default;
    protected:
        void write_state(bool state) override;
};

class NukiLockAutoUnlatchEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoUnlatchEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockButtonEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockButtonEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockLedEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockLedEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockNightModeEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockNightModeEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockNightModeAutoLockEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockNightModeAutoLockEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockNightModeAutoUnlockDisabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockNightModeAutoUnlockDisabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockNightModeImmediateLockOnStartEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockNightModeImmediateLockOnStartEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockAutoLockEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoLockEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockAutoUnlockDisabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoUnlockDisabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockImmediateAutoLockEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockImmediateAutoLockEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockAutoUpdateEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoUpdateEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockSingleLockEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockSingleLockEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockDstModeEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockDstModeEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockAutoBatteryTypeDetectionEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoBatteryTypeDetectionEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockSlowSpeedDuringNightModeEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockSlowSpeedDuringNightModeEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockDetachedCylinderEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockDetachedCylinderEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};

class NukiLockLoggingEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockLoggingEnabledSwitch() = default;

    protected:
        void write_state(bool state) override;
};
#endif

#ifdef USE_NUMBER
class NukiLockLedBrightnessNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockLedBrightnessNumber() = default;

    protected:
        void control(float value) override;
};
class NukiLockTimeZoneOffsetNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockTimeZoneOffsetNumber() = default;

    protected:
        void control(float value) override;
};
class NukiLockLockNGoTimeoutNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockLockNGoTimeoutNumber() = default;

    protected:
        void control(float value) override;
};
class NukiLockAutoLockTimeoutNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockAutoLockTimeoutNumber() = default;

    protected:
        void control(float value) override;
};

class NukiLockUnlatchDurationNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockUnlatchDurationNumber() = default;

    protected:
        void control(float value) override;
};

class NukiLockUnlockedPositionOffsetDegreesNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockUnlockedPositionOffsetDegreesNumber() = default;

    protected:
        void control(float value) override;
};

class NukiLockLockedPositionOffsetDegreesNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockLockedPositionOffsetDegreesNumber() = default;

    protected:
        void control(float value) override;
};

class NukiLockSingleLockedPositionOffsetDegreesNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockSingleLockedPositionOffsetDegreesNumber() = default;

    protected:
        void control(float value) override;
};

class NukiLockUnlockedToLockedTransitionOffsetDegreesNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockUnlockedToLockedTransitionOffsetDegreesNumber() = default;

    protected:
        void control(float value) override;
};
#endif

}  // namespace esphome::nuki_lock