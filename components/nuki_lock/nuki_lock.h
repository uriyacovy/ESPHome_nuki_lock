#pragma once

#include "esphome/core/component.h"
#include "esphome/components/lock/lock.h"
#include "esphome/components/button/button.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/api/custom_api_device.h"

#include "NukiLock.h"
#include "NukiConstants.h"
#include "BleScanner.h"

namespace esphome {
    namespace nuki_lock {

        static const char *TAG = "nukilock.lock";

        class NukiLockComponent : public lock::Lock, public PollingComponent, public api::CustomAPIDevice, public Nuki::SmartlockEventHandler {
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

                void set_is_connected(binary_sensor::BinarySensor *is_connected) { this->is_connected_ = is_connected; }
                void set_is_paired(binary_sensor::BinarySensor *is_paired) { this->is_paired_ = is_paired; }
                void set_battery_critical(binary_sensor::BinarySensor *battery_critical) { this->battery_critical_ = battery_critical; }
                void set_battery_level(sensor::Sensor *battery_level) { this->battery_level_ = battery_level; }
                void set_door_sensor(binary_sensor::BinarySensor *door_sensor) { this->door_sensor_ = door_sensor; }
                void set_door_sensor_state(text_sensor::TextSensor *door_sensor_state) { this->door_sensor_state_ = door_sensor_state; }
                void set_unpair_button(button::Button *unpair_button) { this->unpair_button_ = unpair_button; }
                void set_pairing_mode_switch(switch_::Switch *pairing_mode_switch) { this->pairing_mode_switch_ = pairing_mode_switch; }
                
                void set_pairing_timeout(uint16_t pairing_timeout) { this->pairing_timeout_ = pairing_timeout; }

                void add_pairing_mode_on_callback(std::function<void()> &&callback);
                void add_pairing_mode_off_callback(std::function<void()> &&callback);
                void add_paired_callback(std::function<void()> &&callback);

                float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

                void dump_config() override;

                lock::LockState nuki_to_lock_state(NukiLock::LockState);
                bool nuki_doorsensor_to_binary(Nuki::DoorSensorState);
                std::string nuki_doorsensor_to_string(Nuki::DoorSensorState nukiDoorSensorState);

                void notify(Nuki::EventType eventType) override;

                void unpair();

                void set_pairing_mode(bool enabled);

                CallbackManager<void()> pairing_mode_on_callback_{};
                CallbackManager<void()> pairing_mode_off_callback_{};
                CallbackManager<void()> paired_callback_{};

            protected:
                void control(const lock::LockCall &call) override;
                void update_status();
                void update_config();
                bool executeLockAction(NukiLock::LockAction lockAction);
                void open_latch() override { this->open_latch_ = true; unlock();}

                binary_sensor::BinarySensor *is_connected_{nullptr};
                binary_sensor::BinarySensor *is_paired_{nullptr};
                binary_sensor::BinarySensor *battery_critical_{nullptr};
                binary_sensor::BinarySensor *door_sensor_{nullptr};
                text_sensor::TextSensor *door_sensor_state_{nullptr};
                sensor::Sensor *battery_level_{nullptr};
                button::Button *unpair_button_{nullptr};
                switch_::Switch *pairing_mode_switch_{nullptr};

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

                uint16_t pairing_timeout_ = 0;

                bool pairing_mode_ = false;
                uint16_t pairing_mode_timer_ = 0;

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

        class NukiLockUnpairButton : public Component, public button::Button {
            public:
                void set_parent(NukiLockComponent *parent) { this->parent_ = parent; }
            protected:
                void press_action() override;
                void dump_config() override;
                NukiLockComponent *parent_;
        };

        class NukiLockPairingModeSwitch : public Component, public switch_::Switch {
            public:
                Trigger<> *get_turn_on_trigger() const;
                Trigger<> *get_turn_off_trigger() const;
                void set_parent(NukiLockComponent *parent) { this->parent_ = parent; }
            protected:
                void setup() override;
                void dump_config() override;
                void write_state(bool state) override;
                Trigger<> *turn_on_trigger_;
                Trigger<> *turn_off_trigger_;
                NukiLockComponent *parent_;
        };

    } //namespace nuki_lock
} //namespace esphome
