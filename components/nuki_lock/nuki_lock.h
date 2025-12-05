#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esphome/core/component.h"
#include "esphome/components/lock/lock.h"
#include "esphome/core/preferences.h"

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

#include "NukiLock.h"
#include "NukiConstants.h"
#include "BleScanner.h"

#include "utils.h"

namespace esphome::nuki_lock {

static const char *TAG = "nuki_lock.lock";

static const uint8_t BLE_CONNECT_TIMEOUT_SEC = 2;
static const uint8_t BLE_CONNECT_RETRIES = 5;

static const uint16_t BLE_DISCONNECT_TIMEOUT = 2000;

static const uint8_t MAX_ACTION_ATTEMPTS = 5;
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
    public Nuki::SmartlockEventHandler
#ifdef USE_API
    , public api::CustomAPIDevice
#endif
    {
    #ifdef USE_BINARY_SENSOR
    SUB_BINARY_SENSOR(connected)
    SUB_BINARY_SENSOR(paired)
    SUB_BINARY_SENSOR(battery_critical)
    SUB_BINARY_SENSOR(door_sensor)
    #endif
    #ifdef USE_SENSOR
    SUB_SENSOR(battery_level)
    SUB_SENSOR(bt_signal)
    #endif
    #ifdef USE_TEXT_SENSOR
    SUB_TEXT_SENSOR(door_sensor_state)
    SUB_TEXT_SENSOR(last_unlock_user)
    SUB_TEXT_SENSOR(last_lock_action)
    SUB_TEXT_SENSOR(last_lock_action_trigger)
    SUB_TEXT_SENSOR(pin_state)
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
    #endif

    public:
        const uint32_t deviceId_ = 2020002;
        const std::string deviceName_ = "Nuki ESPHome";

        explicit NukiLockComponent() : Lock(), nuki_lock_(deviceName_, deviceId_) {}

        // ESPHome overrides
        void setup() override;
        void dump_config() override;
        float get_setup_priority() const override { return setup_priority::HARDWARE; }

        // NukiBLE overrides
        void notify(Nuki::EventType event_type) override;

        // Configuration setters
        void set_pairing_mode_timeout(uint32_t pairing_mode_timeout) { this->pairing_mode_timeout_ = pairing_mode_timeout; }
        void set_query_interval_config(uint32_t query_interval_config) { this->query_interval_config_ = query_interval_config; }
        void set_query_interval_auth_data(uint32_t query_interval_auth_data) { this->query_interval_auth_data_ = query_interval_auth_data; }
        void set_ble_general_timeout(uint32_t ble_general_timeout) { this->ble_general_timeout_ = ble_general_timeout; }
        void set_ble_command_timeout(uint32_t ble_command_timeout) { this->ble_command_timeout_ = ble_command_timeout; }
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
        void add_pairing_mode_on_callback(std::function<void()> &&callback);
        void add_pairing_mode_off_callback(std::function<void()> &&callback);
        void add_paired_callback(std::function<void()> &&callback);
        void add_event_log_received_callback(std::function<void(NukiLock::LogEntry)> &&callback);
        CallbackManager<void()> pairing_mode_on_callback_{};
        CallbackManager<void()> pairing_mode_off_callback_{};
        CallbackManager<void()> paired_callback_{};
        CallbackManager<void(NukiLock::LogEntry)> event_log_received_callback_{};


        void unpair();
        void save_settings();
        void request_calibration();
        void setup_lock(bool new_pairing = false);

        bool is_connected() { return this->connected_; }
        bool is_paired() { return this->nuki_lock_.isPairedWithLock(); }

        void lock_n_go();
        void print_keypad_entries();
        void add_keypad_entry(std::string name, int32_t code);
        void update_keypad_entry(int32_t id, std::string name, int32_t code, bool enabled);
        void delete_keypad_entry(int32_t id);

        NukiLock::NukiLock* get_nuki_lock() { return &this->nuki_lock_; }
        NukiLock::Config* get_nuki_lock_config() { return &this->nuki_lock_config_; }
        NukiLock::AdvancedConfig* get_nuki_lock_advanced_config() { return &this->nuki_lock_advanced_config_; }

    protected:
        void control(const lock::LockCall &call) override;
        void open_latch() override { this->open_latch_ = true; unlock();}

    private:
        // Task management
        static void nuki_task_fn(void *arg);
        void nuki_task_loop();
        TaskHandle_t nuki_task_handle_{nullptr};

        // Core components
        ESPPreferenceObject pref_;
        BleScanner::Scanner scanner_;
        NukiLock::NukiLock nuki_lock_;

        // Nuki state & configuration
        NukiLock::KeyTurnerState retrieved_key_turner_state_;
        NukiLock::LockAction lock_action_;
        NukiLock::Config nuki_lock_config_;
        NukiLock::AdvancedConfig nuki_lock_advanced_config_;

        // Methods to retrieve or set data
        void update_status();
        void update_config();
        void update_advanced_config();
        void update_event_logs();
        void update_auth_data();
        void validate_pin();
        bool execute_lock_action(NukiLock::LockAction lock_action);

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

        // Action flags
        bool open_latch_{false};
        bool lock_n_go_{false};

        // Timing & Intervals
        uint32_t last_command_executed_time_ = 0;
        uint32_t command_cooldown_millis = 0;
        uint32_t query_interval_auth_data_ = 0;
        uint32_t query_interval_config_ = 0;
        uint32_t ble_general_timeout_ = 0;
        uint32_t ble_command_timeout_ = 0;
        uint32_t pairing_mode_timeout_ = 0;

        // Error tracking & counters
        uint8_t action_attempts_ = 0;
        uint32_t status_update_consecutive_errors_ = 0;

        // Event Logs
        const char* event_;
        uint32_t last_rolling_log_id = 0;
        uint32_t event_log_ready_time_ = 0;
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

}