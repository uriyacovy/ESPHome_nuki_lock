#pragma once

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

namespace esphome {
namespace nuki_lock {

static const char *TAG = "nuki_lock.lock";

enum PinState
{
    NotSet = 0,
    Valid = 1,
    Invalid = 2
};

struct NukiLockSettings
{
    uint32_t security_pin;
    PinState pin_state;
};

class NukiLockComponent : public lock::Lock, public PollingComponent, public Nuki::SmartlockEventHandler {
    #ifdef USE_BINARY_SENSOR
    SUB_BINARY_SENSOR(is_connected)
    SUB_BINARY_SENSOR(is_paired)
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
    #endif
    #ifdef USE_SWITCH
    SUB_SWITCH(pairing_mode)
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
    #endif

    static const uint8_t BLE_CONNECT_TIMEOUT_SEC = 3;
    static const uint8_t BLE_CONNECT_TIMEOUT_RETRIES = 1;

    static const uint8_t MAX_ACTION_ATTEMPTS = 5;
    static const uint8_t MAX_TOLERATED_UPDATES_ERRORS = 5;

    static const uint32_t COOLDOWN_COMMANDS_MILLIS = 1000;
    static const uint32_t COOLDOWN_COMMANDS_EXTENDED_MILLIS = 3000;

    static const uint8_t MAX_AUTH_DATA_ENTRIES = 10;
    static const uint8_t MAX_EVENT_LOG_ENTRIES = 3;

    public:
        const uint32_t deviceId_ = 2020002;
        const std::string deviceName_ = "Nuki ESPHome";

        explicit NukiLockComponent() : Lock(), open_latch_(false),
                                    lock_n_go_(false),
                                    keypad_paired_(false),
                                    nuki_lock_(deviceName_, deviceId_) {
                this->traits.set_supports_open(true);
                this->nuki_lock_.setEventHandler(this);
        }

        void setup() override;
        void update() override;
        void dump_config() override;
        void notify(Nuki::EventType event_type) override;
        float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

        void set_ultra_pairing_mode(bool ultra_pairing_mode) { this->ultra_pairing_mode_ = ultra_pairing_mode; }
        void set_alt_connect_mode(bool alt_connect_mode) { this->alt_connect_mode_ = alt_connect_mode; }
        void set_pairing_as_app(bool pairing_as_app) { this->pairing_as_app_ = pairing_as_app; }
        void set_pairing_mode_timeout(uint32_t pairing_mode_timeout) { this->pairing_mode_timeout_ = pairing_mode_timeout; }
        void set_query_interval_config(uint32_t query_interval_config) { this->query_interval_config_ = query_interval_config; }
        void set_query_interval_auth_data(uint32_t query_interval_auth_data) { this->query_interval_auth_data_ = query_interval_auth_data; }
        void set_event(const char *event) {
            this->event_ = event;
            if(strcmp(event, "esphome.none") != 0) {
                this->send_events_ = true;
            }
        }
        void set_security_pin(uint32_t security_pin) {
            this->security_pin_config_ = security_pin;
            this->security_pin_ = security_pin;
        }

        void add_pairing_mode_on_callback(std::function<void()> &&callback);
        void add_pairing_mode_off_callback(std::function<void()> &&callback);
        void add_paired_callback(std::function<void()> &&callback);

        CallbackManager<void()> pairing_mode_on_callback_{};
        CallbackManager<void()> pairing_mode_off_callback_{};
        CallbackManager<void()> paired_callback_{};

        lock::LockState nuki_to_lock_state(NukiLock::LockState);
        bool nuki_doorsensor_to_binary(Nuki::DoorSensorState);

        uint8_t fob_action_to_int(const char *str);
        void fob_action_to_string(const int action, char* str);

        Nuki::BatteryType battery_type_to_enum(const char* str);
        void battery_type_to_string(const Nuki::BatteryType battery_type, char* str);

        NukiLock::MotorSpeed motor_speed_to_enum(const char* str);
        void motor_speed_to_string(const NukiLock::MotorSpeed speed, char* str);

        NukiLock::ButtonPressAction button_press_action_to_enum(const char* str);
        void button_press_action_to_string(NukiLock::ButtonPressAction action, char* str);

        Nuki::TimeZoneId timezone_to_enum(const char *str);
        void timezone_to_string(const Nuki::TimeZoneId timeZoneId, char* str);

        Nuki::AdvertisingMode advertising_mode_to_enum(const char *str);
        void advertising_mode_to_string(const Nuki::AdvertisingMode mode, char* str);

        void homekit_status_to_string(const int status, char* str);

        void pin_state_to_string(const PinState value, char* str);

        void use_security_pin(uint32_t security_pin);

        void unpair();
        void set_pairing_mode(bool enabled);
        void save_settings();

        #ifdef USE_NUMBER
        void set_config_number(const char* config, float value);
        #endif
        #ifdef USE_SWITCH
        void set_config_switch(const char* config, bool value);
        #endif
        #ifdef USE_SELECT
        void set_config_select(const char* config, const char* value);
        #endif

    protected:
        void control(const lock::LockCall &call) override;
        void open_latch() override { this->open_latch_ = true; unlock();}

        void update_status();
        void update_config();
        void update_advanced_config();

        void update_event_logs();
        void update_auth_data();
        void process_log_entries(const std::list<NukiLock::LogEntry>& log_entries);

        void setup_intervals(bool setup = true);
        void publish_pin_state();

        bool is_pin_valid();
        uint32_t get_pin();

        bool execute_lock_action(NukiLock::LockAction lock_action);

        BleScanner::Scanner scanner_;
        NukiLock::KeyTurnerState retrieved_key_turner_state_;
        NukiLock::LockAction lock_action_;

        #ifdef USE_API
        api::CustomAPIDevice custom_api_device_;
        #endif

        uint32_t last_command_executed_time_ = 0;
        uint32_t command_cooldown_millis = 0;
        uint8_t action_attempts_ = 0;
        uint32_t status_update_consecutive_errors_ = 0;

        bool status_update_;
        bool config_update_;
        bool advanced_config_update_;
        bool auth_data_update_;
        bool event_log_update_;
        bool open_latch_;
        bool lock_n_go_;

        uint32_t query_interval_auth_data_ = 0;
        uint32_t query_interval_config_ = 0;

        uint32_t pairing_mode_timeout_ = 0;
        bool pairing_mode_ = false;

        bool ultra_pairing_mode_ = false;
        bool pairing_as_app_ = false;
        bool alt_connect_mode_ = false;

        PinState pin_state_ = PinState::NotSet;
        uint32_t security_pin_ = 0;
        uint32_t security_pin_config_ = 0;

        const char* event_;
        bool send_events_ = false;
        uint32_t last_rolling_log_id = 0;

        std::map<uint32_t, std::string> auth_entries_;
        uint32_t auth_id_ = 0;
        char auth_name_[33] = {0};

        ESPPreferenceObject pref_;

    private:
        NukiLock::NukiLock nuki_lock_;

        void lock_n_go();
        void print_keypad_entries();
        void add_keypad_entry(std::string name, int32_t code);
        void update_keypad_entry(int32_t id, std::string name, int32_t code, bool enabled);
        void delete_keypad_entry(int32_t id);
        bool valid_keypad_id(int32_t id);
        bool valid_keypad_name(std::string name);
        bool valid_keypad_code(int32_t code);

        std::vector<uint16_t> keypad_code_ids_;
        bool keypad_paired_;
};

// Actions
template<typename... Ts> class NukiLockUnpairAction : public Action<Ts...> {
    public:
        NukiLockUnpairAction(NukiLockComponent *parent) : parent_(parent) {}

        void play(Ts... x) { this->parent_->unpair(); }

    protected:
        NukiLockComponent *parent_;
};

template<typename... Ts> class NukiLockPairingModeAction : public Action<Ts...> {
    public:
        NukiLockPairingModeAction(NukiLockComponent *parent) : parent_(parent) {}
        TEMPLATABLE_VALUE(bool, pairing_mode)

        void play(Ts... x) { this->parent_->set_pairing_mode(this->pairing_mode_.value(x...)); }

    protected:
        NukiLockComponent *parent_;
};

template<typename... Ts> class NukiLockSecurityPinAction : public Action<Ts...> {
    public:
        NukiLockSecurityPinAction(NukiLockComponent *parent) : parent_(parent) {}
        TEMPLATABLE_VALUE(uint32_t, security_pin)

        void play(Ts... x) { this->parent_->use_security_pin(this->security_pin_.value(x...)); }

    protected:
        NukiLockComponent *parent_;
};

// Callbacks
class PairingModeOnTrigger : public Trigger<> {
    public:
        explicit PairingModeOnTrigger(NukiLockComponent *parent) {
            parent->add_pairing_mode_on_callback([this]() { this->trigger(); });
        }
};

class PairingModeOffTrigger : public Trigger<> {
    public:
        explicit PairingModeOffTrigger(NukiLockComponent *parent) {
            parent->add_pairing_mode_off_callback([this]() { this->trigger(); });
        }
};

class PairedTrigger : public Trigger<> {
    public:
        explicit PairedTrigger(NukiLockComponent *parent) {
            parent->add_paired_callback([this]() { this->trigger(); });
        }
};

// Entities
#ifdef USE_BUTTON
class NukiLockUnpairButton : public button::Button, public Parented<NukiLockComponent> {
    public:
        NukiLockUnpairButton() = default;
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
#endif

} //namespace nuki_lock
} //namespace esphome