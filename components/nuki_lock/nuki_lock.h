#pragma once

#include "esphome/core/component.h"
#include "esphome/components/lock/lock.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome.h"

#include "NukiBle.h"
#include "NukiConstants.h"

namespace esphome {
namespace nuki_lock {

static const char *TAG = "nukilock.lock ";

class Handler: public Nuki::SmartlockEventHandler {
    public:
        virtual ~Handler() {};
        Handler(bool *notified_p) { this->notified_p_ = notified_p; }
    void notify(Nuki::EventType eventType) {
        *(this->notified_p_) = true;
        ESP_LOGI(TAG, "event notified %d", eventType);      
    }
    private:
        bool *notified_p_;
};

class NukiLock : public lock::Lock, public PollingComponent, public api::CustomAPIDevice {
    public:
        const uint32_t deviceId_ = 2020002;
        const std::string deviceName_ = "Nuki ESPHome"; 

        explicit NukiLock() : Lock(), unpair_(false), open_latch_(false) { this->traits.set_supports_open(true); }

        void setup() override;
        void update() override;
        void lock_n_go();

        void set_is_paired(binary_sensor::BinarySensor *is_paired) { this->is_paired_ = is_paired; }
        void set_battery_critical(binary_sensor::BinarySensor *battery_critical) { this->battery_critical_ = battery_critical; }
        void set_battery_level(sensor::Sensor *battery_level) { this->battery_level_ = battery_level; }
        void set_door_sensor(binary_sensor::BinarySensor *door_sensor) { this->door_sensor_ = door_sensor; }
        void set_door_sensor_state(text_sensor::TextSensor *door_sensor_state) { this->door_sensor_state_ = door_sensor_state; }
        void set_unpair(bool unpair) {this->unpair_ = unpair; }

        float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

        void dump_config() override;

        lock::LockState nuki_to_lock_state(Nuki::LockState);
        bool nuki_doorsensor_to_binary(Nuki::DoorSensorState);
        std::string nuki_doorsensor_to_string(Nuki::DoorSensorState nukiDoorSensorState);

    protected:
        void control(const lock::LockCall &call) override;
        void update_status();
        void open_latch() override { this->open_latch_ = true; unlock();}

        binary_sensor::BinarySensor *is_paired_{nullptr};
        binary_sensor::BinarySensor *battery_critical_{nullptr};
        binary_sensor::BinarySensor *door_sensor_{nullptr};
        text_sensor::TextSensor *door_sensor_state_{nullptr};
        sensor::Sensor *battery_level_{nullptr};
        Nuki::NukiBle *nukiBle_;
        BleScanner scanner_;
        Nuki::KeyTurnerState retrievedKeyTurnerState_;
        Handler *handler_;
        bool status_update_;
        bool unpair_;
        bool open_latch_;
};

} //namespace nuki_lock
} //namespace esphome