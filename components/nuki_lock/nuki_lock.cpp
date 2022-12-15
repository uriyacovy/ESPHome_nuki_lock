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
    Nuki::CmdResult result = this->nukiLock_->requestKeyTurnerState(&(this->retrievedKeyTurnerState_));

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

    NukiLock::Config config;
    result = this->nukiLock_->requestConfig(&config);
    if (result == Nuki::CmdResult::Success) {
        keypad_paired_ = config.hasKeypad;
    }
    else {
        ESP_LOGE(TAG, "print_keypad_entries: requestConfig failed (result %d)", result);
        return;
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
    register_service(&NukiLockComponent::print_keypad_entries, "print_keypad_entries");
    register_service(&NukiLockComponent::add_keypad_entry, "add_keypad_entry", {"name", "code"});
    register_service(&NukiLockComponent::update_keypad_entry, "update_keypad_entry", {"id", "name", "code", "enabled"});
    register_service(&NukiLockComponent::delete_keypad_entry, "delete_keypad_entry", {"id"});
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
    Nuki::CmdResult result;

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
                state = lock::LockState::LOCK_STATE_LOCKING;
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

bool NukiLockComponent::valid_keypad_id(int id) {
    bool idValid = std::find(keypadCodeIds_.begin(), keypadCodeIds_.end(), id) != keypadCodeIds_.end();
    if (!idValid) {
        ESP_LOGE(TAG, "keypad id %d unknown.", id);
    }
    return idValid;
}

bool NukiLockComponent::valid_keypad_name(std::string name) {
    bool nameValid = ! (name == "" || name == "--");
    if (!nameValid) {
        ESP_LOGE(TAG, "keypad name '%s' is invalid.", name.c_str());
    }
    return nameValid;
}

bool NukiLockComponent::valid_keypad_code(int code) {
    bool codeValid = (code > 100000 && code < 1000000 && (std::to_string(code).find('0') == std::string::npos));
    if (!codeValid) {
        ESP_LOGE(TAG, "keypad code %d is invalid. Code must be 6 digits, without 0.", code);
    }
    return codeValid;
}

void NukiLockComponent::add_keypad_entry(std::string name, int code) {
    if (! keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if (! (valid_keypad_name(name) && valid_keypad_code(code)) ) {
        ESP_LOGE(TAG, "add_keypad_entry invalid parameters");
        return;
    }
        
    NukiLock::NewKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    size_t nameLen = name.length();
    memcpy(&entry.name, name.c_str(), nameLen > 20 ? 20 : nameLen);
    entry.code = code;
    Nuki::CmdResult result = this->nukiLock_->addKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "add_keypad_entry is sucessful");
    }
    else {
        ESP_LOGE(TAG, "add_keypad_entry: addKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::update_keypad_entry(int id, std::string name, int code, bool enabled) {
    if (! keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if (! (valid_keypad_id(id) && valid_keypad_name(name) && valid_keypad_code(code)) ) {
        ESP_LOGE(TAG, "update_keypad_entry invalid parameters");
        return;
    }

    NukiLock::UpdatedKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.codeId = id;
    size_t nameLen = name.length();
    memcpy(&entry.name, name.c_str(), nameLen > 20 ? 20 : nameLen);
    entry.code = code;
    entry.enabled = enabled ? 1 : 0;
    Nuki::CmdResult result = this->nukiLock_->updateKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "update_keypad_entry is sucessful");
    }
    else {
        ESP_LOGE(TAG, "update_keypad_entry: updateKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::delete_keypad_entry(int id) {
    if (! keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }
        
    if (! valid_keypad_id(id)) {
        ESP_LOGE(TAG, "delete_keypad_entry invalid parameters");
        return;
    }

    Nuki::CmdResult result = this->nukiLock_->deleteKeypadEntry(id);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "delete_keypad_entry is sucessful");
    }
    else {
        ESP_LOGE(TAG, "delete_keypad_entry: deleteKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::print_keypad_entries() {
    if (! keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    Nuki::CmdResult result = this->nukiLock_->retrieveKeypadEntries(0, 0xffff);
    if(result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "retrieveKeypadEntries sucess"); 
        std::list<NukiLock::KeypadEntry> entries;
        this->nukiLock_->getKeypadEntries(&entries);

        entries.sort([](const NukiLock::KeypadEntry& a, const NukiLock::KeypadEntry& b) { return a.codeId < b.codeId; });

        keypadCodeIds_.clear();
        keypadCodeIds_.reserve(entries.size());
        for (const auto& entry : entries) {
            keypadCodeIds_.push_back(entry.codeId);
            ESP_LOGI(TAG, "keypad #%d %s is %s", entry.codeId, entry.name, entry.enabled ? "enabled" : "disabled");
        }
    }
    else {
        ESP_LOGE(TAG, "print_keypad_entries: retrieveKeypadEntries failed (result %d)", result);
    }
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