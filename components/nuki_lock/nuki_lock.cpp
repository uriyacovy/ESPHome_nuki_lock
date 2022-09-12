#include "esphome/core/log.h"
#include "nuki_lock.h"

namespace esphome {
namespace nuki_lock {

lock::LockState NukiLockComponent::nuki_to_lock_state(NukiLock::LockState nukiLockState) {
    switch(nukiLockState) {
        case NukiLock::LockState::Locked:
            return lock::LOCK_STATE_LOCKED;
        case NukiLock::LockState::Unlocked:
            return lock::LOCK_STATE_UNLOCKED;
        case NukiLock::LockState::MotorBlocked:
            return lock::LOCK_STATE_JAMMED;
        case NukiLock::LockState::Locking:
            return lock::LOCK_STATE_LOCKING;
        case NukiLock::LockState::Unlocking:
            return lock::LOCK_STATE_UNLOCKING;
        default:
            return lock::LOCK_STATE_NONE;
    }
}

bool NukiLockComponent::nuki_doorsensor_to_binary(Nuki::DoorSensorState nukiDoorSensorState) {
    switch(nukiDoorSensorState) {
        case Nuki::DoorSensorState::DoorClosed:
            return false;
        default:
            return true;
    }
}

std::string NukiLockComponent::nuki_doorsensor_to_string(Nuki::DoorSensorState nukiDoorSensorState) {
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

void NukiLockComponent::update_status()
{
    this->status_update_ = false;
    uint8_t result = this->nukiLock_->requestKeyTurnerState(&(this->retrievedKeyTurnerState_));

    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "Bat state: %#x, Bat crit: %d, Bat perc:%d lock state: %d %d:%d:%d",
          this->retrievedKeyTurnerState_.criticalBatteryState,
          this->nukiLock_->isBatteryCritical(), this->nukiLock_->getBatteryPerc(), this->retrievedKeyTurnerState_.lockState, 
          this->retrievedKeyTurnerState_.currentTimeHour,
          this->retrievedKeyTurnerState_.currentTimeMinute, 
          this->retrievedKeyTurnerState_.currentTimeSecond);
        this->publish_state(this->nuki_to_lock_state(this->retrievedKeyTurnerState_.lockState));
        this->is_connected_->publish_state(true);
        if (this->battery_critical_ != nullptr)
            this->battery_critical_->publish_state(this->nukiLock_->isBatteryCritical());
        if (this->battery_level_ != nullptr)
            this->battery_level_->publish_state(this->nukiLock_->getBatteryPerc());
        if (this->door_sensor_ != nullptr)
            this->door_sensor_->publish_state(this->nuki_doorsensor_to_binary(this->retrievedKeyTurnerState_.doorSensorState));
        if (this->door_sensor_state_ != nullptr)
            this->door_sensor_state_->publish_state(this->nuki_doorsensor_to_string(this->retrievedKeyTurnerState_.doorSensorState));
    } else {
        ESP_LOGE(TAG, "requestKeyTurnerState failed: %d", result);
        this->is_connected_->publish_state(false);
        this->publish_state(lock::LOCK_STATE_NONE);
        this->status_update_ = true;
    }  
}

void NukiLockComponent::setup() {

    ESP_LOGI(TAG, "Starting NUKI Lock...");
    this->nukiLock_ = new NukiLock::NukiLock(this->deviceName_, this->deviceId_);
    this->handler_ = new nuki_lock::Handler(&(this->status_update_));

    this->traits.set_supported_states(std::set<lock::LockState> {lock::LOCK_STATE_NONE, lock::LOCK_STATE_LOCKED, 
                                                                 lock::LOCK_STATE_UNLOCKED, lock::LOCK_STATE_JAMMED, 
                                                                 lock::LOCK_STATE_LOCKING, lock::LOCK_STATE_UNLOCKING});
    this->scanner_.initialize();
    this->nukiLock_->registerBleScanner(&this->scanner_);
    this->nukiLock_->initialize();
    this->nukiLock_->setConnectTimeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nukiLock_->setConnectRetries(BLE_CONNECT_TIMEOUT_RETRIES);
    
    if (this->unpair_) {
        ESP_LOGW(TAG, "Unpair requested");
        this->nukiLock_->unPairNuki();
    }

    if (this->nukiLock_->isPairedWithLock()) {
        this->status_update_ = true;
        ESP_LOGI(TAG, "%s Nuki paired", this->deviceName_); 
        this->is_paired_->publish_initial_state(true);
        this->nukiLock_->setEventHandler(this->handler_);
    }
    else {
        ESP_LOGW(TAG, "%s Nuki is not paired", this->deviceName_); 
        this->is_paired_->publish_initial_state(false);
    }

    this->publish_state(lock::LOCK_STATE_NONE);

    register_service(&NukiLockComponent::lock_n_go, "lock_n_go");
}

void NukiLockComponent::update() {

    this->scanner_.update();

    if (this->nukiLock_->isPairedWithLock()) {
        this->is_paired_->publish_state(true);
        if (this->status_update_) {
            this->update_status();
        }
    }
    else if (! this->unpair_) {
        bool paired = (this->nukiLock_->pairNuki() == Nuki::PairingResult::Success);
        if (paired) {
            ESP_LOGI(TAG, "Nuki paired");
            this->update_status();
        }
        this->is_paired_->publish_state(paired);
    }
}

void NukiLockComponent::control(const lock::LockCall &call) {
    if (!this->nukiLock_->isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock/Unlock action called for unpaired nuki");
        return;
    }

    auto state = *call.get_state();
    uint8_t result;

    switch(state){
        case lock::LOCK_STATE_LOCKED:
            result = this->nukiLock_->lockAction(NukiLock::LockAction::Lock);
            break;

        case lock::LOCK_STATE_UNLOCKED:{
            NukiLock::LockAction action = NukiLock::LockAction::Unlock;

            if(this->open_latch_){
                action = NukiLock::LockAction::Unlatch;
            }

            if(this->lock_n_go_){
                action = NukiLock::LockAction::LockNgo;
            }

            result = this->nukiLock_->lockAction(action);

            this->open_latch_ = false;
            this->lock_n_go_ = false;
            break;
        }

        default:
            ESP_LOGE(TAG, "lockAction unsupported state");
            return;
    }

    if (result == Nuki::CmdResult::Success) {
        this->publish_state(state);
    }
    else {
        ESP_LOGE(TAG, "lockAction failed: %d", result);
        this->is_connected_->publish_state(false);
        this->publish_state(lock::LOCK_STATE_NONE);
        this->status_update_ = true;
    }
}

void NukiLockComponent::lock_n_go(){
    this->lock_n_go_ = true;
    this->unlock();
}

void NukiLockComponent::dump_config(){
    LOG_LOCK(TAG, "Nuki Lock", this);    
    LOG_BINARY_SENSOR(TAG, "Is Connected", this->is_connected_);
    LOG_BINARY_SENSOR(TAG, "Is Paired", this->is_paired_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_);
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_);
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_);
    ESP_LOGCONFIG(TAG, "Unpair request is %s", this->unpair_? "true":"false");
}

} //namespace nuki_lock
} //namespace esphome