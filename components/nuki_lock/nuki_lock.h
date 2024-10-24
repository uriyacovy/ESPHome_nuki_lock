#pragma once

#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/lock/lock.h"

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

static const char *TAG = "nukilock.lock";

class NukiLockComponent : public lock::Lock, public PollingComponent, public api::CustomAPIDevice, public Nuki::SmartlockEventHandler {
    
    #ifdef USE_BINARY_SENSOR
    SUB_BINARY_SENSOR(is_connected)
    SUB_BINARY_SENSOR(is_paired)
    SUB_BINARY_SENSOR(battery_critical)
    SUB_BINARY_SENSOR(door_sensor)
    #endif
    #ifdef USE_SENSOR
    SUB_SENSOR(battery_level)
    #endif
    #ifdef USE_TEXT_SENSOR
    SUB_TEXT_SENSOR(door_sensor_state)
    #endif
    #ifdef USE_NUMBER
    SUB_NUMBER(led_brightness)
    #endif
    #ifdef USE_BUTTON
    SUB_BUTTON(unpair)
    #endif
    #ifdef USE_SWITCH
    SUB_SWITCH(pairing_mode)
    SUB_SWITCH(button_enabled)
    SUB_SWITCH(led_enabled)
    #endif

    static const uint8_t BLE_CONNECT_TIMEOUT_SEC = 3;
    static const uint8_t BLE_CONNECT_TIMEOUT_RETRIES = 1;
    static const uint8_t MAX_ACTION_ATTEMPTS = 5;
    static const uint8_t MAX_TOLERATED_UPDATES_ERRORS = 5;
    static const uint32_t COOLDOWN_COMMANDS_MILLIS = 1000;
    static const uint32_t COOLDOWN_COMMANDS_EXTENDED_MILLIS = 3000;

    public:
        const uint32_t deviceId_ = 2020002;
        const std::string deviceName_ = "Nuki ESPHome";

        explicit NukiLockComponent() : Lock(), open_latch_(false),
                                    lock_n_go_(false),
                                    keypad_paired_(false),
                                    nukiLock_(deviceName_, deviceId_) {
                this->traits.set_supports_open(true);
                this->nukiLock_.setEventHandler(this);
        }

        void setup() override;
        void update() override;
        void dump_config() override;
        void notify(Nuki::EventType eventType) override;
        float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

        void set_security_pin(uint16_t security_pin) { this->security_pin_ = security_pin; }
        void set_pairing_mode_timeout(uint16_t pairing_mode_timeout) { this->pairing_mode_timeout_ = pairing_mode_timeout; }
        
        void add_pairing_mode_on_callback(std::function<void()> &&callback);
        void add_pairing_mode_off_callback(std::function<void()> &&callback);
        void add_paired_callback(std::function<void()> &&callback);

        CallbackManager<void()> pairing_mode_on_callback_{};
        CallbackManager<void()> pairing_mode_off_callback_{};
        CallbackManager<void()> paired_callback_{};

        lock::LockState nuki_to_lock_state(NukiLock::LockState);
        bool nuki_doorsensor_to_binary(Nuki::DoorSensorState);
        std::string nuki_doorsensor_to_string(Nuki::DoorSensorState nukiDoorSensorState);

        void unpair();
        void set_pairing_mode(bool enabled);
        void set_button_enabled(bool enabled);
        void set_led_enabled(bool enabled);
        void set_led_brightness(float value);

    protected:
        void control(const lock::LockCall &call) override;
        void open_latch() override { this->open_latch_ = true; unlock();}

        void update_status();
        void update_config();
        bool executeLockAction(NukiLock::LockAction lockAction);

        BleScanner::Scanner scanner_;
        NukiLock::KeyTurnerState retrievedKeyTurnerState_;
        NukiLock::LockAction lockAction_;

        uint32_t lastCommandExecutedTime_ = 0;
        uint32_t command_cooldown_millis = 0;
        uint8_t actionAttempts_ = 0;
        uint32_t statusUpdateConsecutiveErrors_ = 0;

        bool status_update_;
        bool config_update_;
        bool open_latch_;
        bool lock_n_go_;

        uint16_t security_pin_ = 0;

        uint16_t pairing_mode_timeout_ = 0;
        bool pairing_mode_ = false;
        uint32_t pairing_mode_timer_ = 0;

    private:
        NukiLock::NukiLock nukiLock_;

        void lock_n_go();
        void print_keypad_entries();
        void add_keypad_entry(std::string name, int code);
        void update_keypad_entry(int id, std::string name, int code, bool enabled);
        void delete_keypad_entry(int id);
        bool valid_keypad_id(int id);
        bool valid_keypad_name(std::string name);
        bool valid_keypad_code(int code);

        std::vector<uint16_t> keypadCodeIds_;
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
class NukiLockUnpairButton : public button::Button, public Parented<NukiLockComponent> {
    public:
        NukiLockUnpairButton() = default;
    protected:
        void press_action() override;
};

class NukiLockPairingModeSwitch : public Component, public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockPairingModeSwitch() = default;
        Trigger<> *get_turn_on_trigger() const;
        Trigger<> *get_turn_off_trigger() const;
    protected:
        void setup() override;
        void write_state(bool state) override;
        Trigger<> *turn_on_trigger_;
        Trigger<> *turn_off_trigger_;
};

class NukiLockButtonEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
   public:
      NukiLockButtonEnabledSwitch() = default;
      Trigger<> *get_turn_on_trigger() const;
      Trigger<> *get_turn_off_trigger() const;

   protected:
      void setup() override;
      void write_state(bool state) override;
      Trigger<> *turn_on_trigger_;
      Trigger<> *turn_off_trigger_;
};

class NukiLockLedEnabledSwitch : public switch_::Switch, public Parented<NukiLockComponent> {
    public:
        NukiLockLedEnabledSwitch() = default;
        Trigger<> *get_turn_on_trigger() const;
        Trigger<> *get_turn_off_trigger() const;

    protected:
        void setup() override;
        void write_state(bool state) override;
        Trigger<> *turn_on_trigger_;
        Trigger<> *turn_off_trigger_;
};

class NukiLockLedBrightnessNumber : public number::Number, public Parented<NukiLockComponent> {
    public:
        NukiLockLedBrightnessNumber() = default;

    protected:
        void setup() override;
        void control(float value) override;
};

} //namespace nuki_lock
} //namespace esphome