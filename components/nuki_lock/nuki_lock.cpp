#include "esphome/core/log.h"
#include "esphome.h"
#include "nuki_lock.h"

namespace esphome {
namespace nuki_lock {

lock::LockState NukiLock::nuki_to_lock_state(Nuki::LockState nukiLockState) {
    switch(nukiLockState) {
        case Nuki::LockState::Locked:
            return lock::LOCK_STATE_LOCKED;
        case Nuki::LockState::Unlocked:
            return lock::LOCK_STATE_UNLOCKED;
        case Nuki::LockState::MotorBlocked:
            return lock::LOCK_STATE_JAMMED;
        case Nuki::LockState::Locking:
            return lock::LOCK_STATE_LOCKING;
        case Nuki::LockState::Unlocking:
            return lock::LOCK_STATE_UNLOCKING;
        default:
            return lock::LOCK_STATE_NONE;
    }
}

bool NukiLock::nuki_doorsensor_to_binary(Nuki::DoorSensorState nukiDoorSensorState) {
    switch(nukiDoorSensorState) {
        case Nuki::DoorSensorState::DoorClosed:
            return false;
        default:
            return true;
    }
}

std::string NukiLock::nuki_doorsensor_to_string(Nuki::DoorSensorState nukiDoorSensorState) {
    switch(nukiDoorSensorState) {
        case Nuki::DoorSensorState::Unavailable:
            return "unavailable";
        case Nuki::DoorSensorState::Deactivated:
            return "deactivated";
        case Nuki::DoorSensorState::DoorClosed:
            return "closed";
        case Nuki::DoorSensorState::DoorOpened:
            return "opened";
        case Nuki::DoorSensorState::DoorStateUnknown:
            return "unknown";
        case Nuki::DoorSensorState::Calibrating:
            return "calibrating";
        default:
            return "undefined";
    }
}

void NukiLock::update_status()
{
    this->status_update_ = false;
    uint8_t result = this->nukiBle_->requestKeyTurnerState(&(this->retrievedKeyTurnerState_));
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "Bat state: %#x, Bat crit: %d, Bat perc:%d lock state: %d %d:%d:%d",
          this->retrievedKeyTurnerState_.criticalBatteryState,
          this->nukiBle_->isBatteryCritical(), this->nukiBle_->getBatteryPerc(), this->retrievedKeyTurnerState_.lockState, 
          this->retrievedKeyTurnerState_.currentTimeHour,
          this->retrievedKeyTurnerState_.currentTimeMinute, 
          this->retrievedKeyTurnerState_.currentTimeSecond);
        this->publish_state(this->nuki_to_lock_state(this->retrievedKeyTurnerState_.lockState));
        if (this->battery_critical_ != nullptr)
            this->battery_critical_->publish_state(this->nukiBle_->isBatteryCritical());
        if (this->battery_level_ != nullptr)
            this->battery_level_->publish_state(this->nukiBle_->getBatteryPerc());
        if (this->door_sensor_ != nullptr)
            this->door_sensor_->publish_state(this->nuki_doorsensor_to_binary(this->retrievedKeyTurnerState_.doorSensorState));
        if (this->door_sensor_state_ != nullptr)
            this->door_sensor_state_->publish_state(this->nuki_doorsensor_to_string(this->retrievedKeyTurnerState_.doorSensorState));
    } else {
        ESP_LOGE(TAG, "requestKeyTurnerState failed: %d", result);
    }  
}

void NukiLock::setup() {

    ESP_LOGI(TAG, "Starting NUKI BLE...");
    this->nukiBle_ = new Nuki::NukiBle(this->deviceName_, this->deviceId_);
    this->handler_ = new nuki_lock::Handler(&(this->status_update_));

    this->traits.set_supported_states(std::set<lock::LockState> {lock::LOCK_STATE_NONE, lock::LOCK_STATE_LOCKED, 
                                                                 lock::LOCK_STATE_UNLOCKED, lock::LOCK_STATE_JAMMED, 
                                                                 lock::LOCK_STATE_LOCKING, lock::LOCK_STATE_UNLOCKING});
    this->scanner_.initialize();
    this->nukiBle_->registerBleScanner(&this->scanner_);
    this->nukiBle_->initialize();
    
    if (this->unpair_) {
        ESP_LOGW(TAG, "Unpair requested");
        this->nukiBle_->unPairNuki();
    }

    if (this->nukiBle_->isPairedWithLock()) {
        this->status_update_ = true;
        ESP_LOGI(TAG, "%s Nuki paired", this->deviceName_); 
        this->is_paired_->publish_initial_state(true);
        this->nukiBle_->setEventHandler(this->handler_);
    }
    else {
        ESP_LOGW(TAG, "%s Nuki is not paired", this->deviceName_); 
        this->is_paired_->publish_initial_state(false);
    }

    this->publish_state(lock::LOCK_STATE_NONE);

    ESP_LOGI(TAG, "Declaring service");
    register_service(&NukiLock::lock_n_go, "lock_n_go");
}

void NukiLock::lock_n_go() {
    if (this->nukiBle_->isPairedWithLock()) {
        uint8_t result;

        if (state == lock::LOCK_STATE_UNLOCKED) {
            result = this->nukiBle_->lockAction(Nuki::LockAction::LockNgo);
        }

        if (result == Nuki::CmdResult::Success) {
            this->publish_state(state);
        }
        else {
            ESP_LOGE(TAG, "lockAction failed: %d", result);
        }
    }
    else {
        ESP_LOGE(TAG, "Lock N Go service called for unpaired Nuki");
    }
}

void NukiLock::update() {

    this->scanner_.update();

    if (this->nukiBle_->isPairedWithLock()) {
        this->is_paired_->publish_state(true);
        if (this->status_update_) {
            this->update_status();
        }
    }
    else if (! this->unpair_) {
        bool paired = this->nukiBle_->pairNuki();
        if (paired) {
            ESP_LOGI(TAG, "Nuki paired");
            this->update_status();
        }
        this->is_paired_->publish_state(paired);
    }
}

void NukiLock::control(const lock::LockCall &call) {
    if (this->nukiBle_->isPairedWithLock()) {
        auto state = *call.get_state();
        
        uint8_t result;

        if (state == lock::LOCK_STATE_LOCKED) {
            result = this->nukiBle_->lockAction(Nuki::LockAction::Lock);
        } else if (state == lock::LOCK_STATE_UNLOCKED) {
            result = this->nukiBle_->lockAction(this->open_latch_ ? Nuki::LockAction::Unlatch : Nuki::LockAction::Unlock);
            this->open_latch_ = false;
        }
        else {
            ESP_LOGE(TAG, "lockAction unsupported state");
            return;
        }      
        if (result == Nuki::CmdResult::Success) {
            this->publish_state(state);
        }
        else {
            ESP_LOGE(TAG, "lockAction failed: %d", result);
        }
    }
    else {
        ESP_LOGE(TAG, "Lock/Unlock action called for unpaired nuki");
    }
}

void NukiLock::dump_config(){
    LOG_LOCK(TAG, "Nuki Lock", this);    
    LOG_BINARY_SENSOR(TAG, "Is Paired", this->is_paired_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_);
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_);
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_);
    ESP_LOGCONFIG(TAG, "Unpair request is %s", this->unpair_? "true":"false");
}

} //namespace nuki_lock
} //namespace esphome