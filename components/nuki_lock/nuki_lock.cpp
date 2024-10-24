#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "nuki_lock.h"

namespace esphome {
namespace nuki_lock {

lock::LockState NukiLockComponent::nuki_to_lock_state(NukiLock::LockState nukiLockState) {
    switch(nukiLockState) {
        case NukiLock::LockState::Locked:
            return lock::LOCK_STATE_LOCKED;
        case NukiLock::LockState::Unlocked:
        case NukiLock::LockState::Unlatched:
            return lock::LOCK_STATE_UNLOCKED;
        case NukiLock::LockState::MotorBlocked:
            return lock::LOCK_STATE_JAMMED;
        case NukiLock::LockState::Locking:
            return lock::LOCK_STATE_LOCKING;
        case NukiLock::LockState::Unlocking:
        case NukiLock::LockState::Unlatching:
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
    Nuki::CmdResult cmdResult = this->nukiLock_.requestKeyTurnerState(&(this->retrievedKeyTurnerState_));
    char cmdResultAsString[30];
    NukiLock::cmdResultToString(cmdResult, cmdResultAsString);

    if (cmdResult == Nuki::CmdResult::Success) {
        this->statusUpdateConsecutiveErrors_ = 0;
        NukiLock::LockState currentLockState = this->retrievedKeyTurnerState_.lockState;
        char currentLockStateAsString[30];
        NukiLock::lockstateToString(currentLockState, currentLockStateAsString);

        ESP_LOGI(TAG, "Bat state: %#x, Bat crit: %d, Bat perc:%d lock state: %s (%d) %d:%d:%d",
            this->retrievedKeyTurnerState_.criticalBatteryState,
            this->nukiLock_.isBatteryCritical(),
            this->nukiLock_.getBatteryPerc(),
            currentLockStateAsString,
            currentLockState,
            this->retrievedKeyTurnerState_.currentTimeHour,
            this->retrievedKeyTurnerState_.currentTimeMinute,
            this->retrievedKeyTurnerState_.currentTimeSecond
        );

        this->publish_state(this->nuki_to_lock_state(this->retrievedKeyTurnerState_.lockState));
        this->is_connected_->publish_state(true);
        if (this->battery_critical_ != nullptr)
            this->battery_critical_->publish_state(this->nukiLock_.isBatteryCritical());
        if (this->battery_level_ != nullptr)
            this->battery_level_->publish_state(this->nukiLock_.getBatteryPerc());
        if (this->door_sensor_ != nullptr)
            this->door_sensor_->publish_state(this->nuki_doorsensor_to_binary(this->retrievedKeyTurnerState_.doorSensorState));
        if (this->door_sensor_state_ != nullptr)
            this->door_sensor_state_->publish_state(this->nuki_doorsensor_to_string(this->retrievedKeyTurnerState_.doorSensorState));

        if (
            this->retrievedKeyTurnerState_.lockState == NukiLock::LockState::Locking
            || this->retrievedKeyTurnerState_.lockState == NukiLock::LockState::Unlocking
        ) {
            // Schedule a status update without waiting for the next advertisement because the lock
            // is in a transition state. This will speed up the feedback.
            this->status_update_ = true;
        }
    } else {
        ESP_LOGE(TAG, "requestKeyTurnerState failed with error %s (%d)", cmdResultAsString, cmdResult);
        this->status_update_ = true;

        this->statusUpdateConsecutiveErrors_++;
        if (this->statusUpdateConsecutiveErrors_ > MAX_TOLERATED_UPDATES_ERRORS) {
            // Publish failed state only when having too many consecutive errors
            this->is_connected_->publish_state(false);
            this->publish_state(lock::LOCK_STATE_NONE);
        }
    }
}

void NukiLockComponent::update_config() {
    this->config_update_ = false;

    NukiLock::Config config;
    Nuki::CmdResult confReqResult = this->nukiLock_.requestConfig(&config);
    char confReqResultAsString[30];
    NukiLock::cmdResultToString(confReqResult, confReqResultAsString);

    if (confReqResult == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestConfig has resulted in %s (%d)", confReqResultAsString, confReqResult);
        keypad_paired_ = config.hasKeypad;

    } else {
        ESP_LOGE(TAG, "requestConfig has resulted in %s (%d)", confReqResultAsString, confReqResult);
        this->config_update_ = true;
    }
}

bool NukiLockComponent::executeLockAction(NukiLock::LockAction lockAction) {
    // Publish the assumed transitional lock state
    switch (lockAction) {
        case NukiLock::LockAction::Unlatch:
        case NukiLock::LockAction::Unlock: {
            this->publish_state(lock::LOCK_STATE_UNLOCKING);
            break;
        }
        case NukiLock::LockAction::FullLock:
        case NukiLock::LockAction::Lock:
        case NukiLock::LockAction::LockNgo: {
            this->publish_state(lock::LOCK_STATE_LOCKING);
            break;
        }
    }

    // Execute the action
    Nuki::CmdResult result = this->nukiLock_.lockAction(lockAction);

    char lockActionAsString[30];
    NukiLock::lockactionToString(lockAction, lockActionAsString);
    char resultAsString[30];
    NukiLock::cmdResultToString(result, resultAsString);

    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "lockAction %s (%d) has resulted in %s (%d)", lockActionAsString, lockAction, resultAsString, result);
        return true;
    } else {
        ESP_LOGE(TAG, "lockAction %s (%d) has resulted in %s (%d)", lockActionAsString, lockAction, resultAsString, result);
        return false;
    }
}

void NukiLockComponent::setup() {
    ESP_LOGI(TAG, "Starting NUKI Lock...");

    // Increase Watchdog Timeout
    // Fixes Pairing Crash
    esp_task_wdt_init(15, false);

    this->traits.set_supported_states(
        std::set<lock::LockState> {
            lock::LOCK_STATE_NONE,
            lock::LOCK_STATE_LOCKED,
            lock::LOCK_STATE_UNLOCKED,
            lock::LOCK_STATE_JAMMED,
            lock::LOCK_STATE_LOCKING,
            lock::LOCK_STATE_UNLOCKING
        }
    );

    this->scanner_.initialize("ESPHomeNuki");
    this->scanner_.setScanDuration(10);

    this->nukiLock_.registerBleScanner(&this->scanner_);
    this->nukiLock_.initialize();
    this->nukiLock_.setConnectTimeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nukiLock_.setConnectRetries(BLE_CONNECT_TIMEOUT_RETRIES);

    if(this->security_pin_ > 0) {
        bool result = this->nukiLock_.saveSecurityPincode(this->security_pin_);
        if (result) {
            ESP_LOGI(TAG, "Set pincode done");
        } else {
            ESP_LOGE(TAG, "Set pincode failed!");
        }
    }
    
    if (this->nukiLock_.isPairedWithLock()) {
        this->status_update_ = true;
        ESP_LOGI(TAG, "%s Nuki paired", this->deviceName_);
        this->is_paired_->publish_initial_state(true);
    } else {
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
    // Check for new advertisements
    this->scanner_.update();
    App.feed_wdt();
    delay(20);

    // Terminate stale Bluetooth connections
    this->nukiLock_.updateConnectionState();

    if(this->pairing_mode_ && this->pairing_mode_timer_ != 0) {
        if(millis() > this->pairing_mode_timer_) {
            ESP_LOGV(TAG, "Pairing timed out, turning off pairing mode");
            this->set_pairing_mode(false);
        }
    }

    if (millis() - lastCommandExecutedTime_ < command_cooldown_millis) {
        // Give the lock time to terminate the previous command
        uint32_t millisSinceLastExecution = millis() - lastCommandExecutedTime_;
        uint32_t millisLeft = (millisSinceLastExecution < command_cooldown_millis) ? command_cooldown_millis - millisSinceLastExecution : 1;
        ESP_LOGV(TAG, "Cooldown period, %dms left", millisLeft);
        return;
    }

    if (this->nukiLock_.isPairedWithLock()) {
        this->is_paired_->publish_state(true);

        // Execute (all) actions first, then status updates, then config updates.
        // Only one command (action, status, or config) is executed per update() call.
        if (this->actionAttempts_ > 0) {
            this->actionAttempts_--;

            NukiLock::LockAction currentLockAction = this->lockAction_;
            char currentLockActionAsString[30];
            NukiLock::lockactionToString(currentLockAction, currentLockActionAsString);
            ESP_LOGD(TAG, "Executing lock action %s (%d)... (%d attempts left)", currentLockActionAsString, currentLockAction, this->actionAttempts_);

            bool isExecutionSuccessful = this->executeLockAction(currentLockAction);

            if (isExecutionSuccessful) {
                if(this->lockAction_ == currentLockAction) {
                    // Stop action attempts only if no new action was received in the meantime.
                    // Otherwise, the new action won't be executed.
                    this->actionAttempts_ = 0;
                }
            } else if (this->actionAttempts_ == 0) {
                // Publish failed state only when no attempts are left
                this->is_connected_->publish_state(false);
                this->publish_state(lock::LOCK_STATE_NONE);
            }

            // Schedule a status update without waiting for the next advertisement for a faster feedback
            this->status_update_ = true;

            // Give the lock extra time when successful in order to account for time to turn the key
            command_cooldown_millis = isExecutionSuccessful ? COOLDOWN_COMMANDS_EXTENDED_MILLIS : COOLDOWN_COMMANDS_MILLIS;
            lastCommandExecutedTime_ = millis();

        } else if (this->status_update_) {
            ESP_LOGD(TAG, "Update present, getting data...");
            this->update_status();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            lastCommandExecutedTime_ = millis();

        } else if (this->config_update_) {
            ESP_LOGD(TAG, "Update present, getting config...");
            this->update_config();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            lastCommandExecutedTime_ = millis();
        }
    } else {
        this->is_paired_->publish_state(false);
        this->is_connected_->publish_state(false);

        // Pairing Mode is active
        if(this->pairing_mode_) {
            // Pair Nuki
            bool paired = (this->nukiLock_.pairNuki() == Nuki::PairingResult::Success);
            if (paired) {
                ESP_LOGI(TAG, "Nuki paired successfuly!");
                this->update_status();
                this->paired_callback_.call();
                this->set_pairing_mode(false);
            }
            this->is_paired_->publish_state(paired);
        }    
    }
}

/**
 * @brief Add a new lock action that will be executed on the next update() call.
 */
void NukiLockComponent::control(const lock::LockCall &call) {

    lock::LockState state = *call.get_state();

    switch(state) {
        case lock::LOCK_STATE_LOCKED:
            this->actionAttempts_ = MAX_ACTION_ATTEMPTS;
            this->lockAction_ = NukiLock::LockAction::Lock;
            break;

        case lock::LOCK_STATE_UNLOCKED: {
            this->actionAttempts_ = MAX_ACTION_ATTEMPTS;
            this->lockAction_ = NukiLock::LockAction::Unlock;

            if(this->open_latch_) {
                this->lockAction_ = NukiLock::LockAction::Unlatch;
            }

            if(this->lock_n_go_) {
                this->lockAction_ = NukiLock::LockAction::LockNgo;
            }

            this->open_latch_ = false;
            this->lock_n_go_ = false;
            break;
        }

        default:
            ESP_LOGE(TAG, "lockAction unsupported state");
            return;
    }

    char lockActionAsString[30];
    NukiLock::lockactionToString(this->lockAction_, lockActionAsString);
    ESP_LOGI(TAG, "New lock action received: %s (%d)", lockActionAsString, this->lockAction_);
}

void NukiLockComponent::lock_n_go() {
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
    bool nameValid = !(name == "" || name == "--");
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
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if (!(valid_keypad_name(name) && valid_keypad_code(code))) {
        ESP_LOGE(TAG, "add_keypad_entry invalid parameters");
        return;
    }

    NukiLock::NewKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    size_t nameLen = name.length();
    memcpy(&entry.name, name.c_str(), nameLen > 20 ? 20 : nameLen);
    entry.code = code;
    Nuki::CmdResult result = this->nukiLock_.addKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "add_keypad_entry is sucessful");
    } else {
        ESP_LOGE(TAG, "add_keypad_entry: addKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::update_keypad_entry(int id, std::string name, int code, bool enabled) {
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if (!(valid_keypad_id(id) && valid_keypad_name(name) && valid_keypad_code(code))) {
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
    Nuki::CmdResult result = this->nukiLock_.updateKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "update_keypad_entry is sucessful");
    } else {
        ESP_LOGE(TAG, "update_keypad_entry: updateKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::delete_keypad_entry(int id) {
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if (!valid_keypad_id(id)) {
        ESP_LOGE(TAG, "delete_keypad_entry invalid parameters");
        return;
    }

    Nuki::CmdResult result = this->nukiLock_.deleteKeypadEntry(id);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "delete_keypad_entry is sucessful");
    } else {
        ESP_LOGE(TAG, "delete_keypad_entry: deleteKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::print_keypad_entries() {
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    Nuki::CmdResult result = this->nukiLock_.retrieveKeypadEntries(0, 0xffff);
    if(result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "retrieveKeypadEntries sucess");
        std::list<NukiLock::KeypadEntry> entries;
        this->nukiLock_.getKeypadEntries(&entries);

        entries.sort([](const NukiLock::KeypadEntry& a, const NukiLock::KeypadEntry& b) { return a.codeId < b.codeId; });

        keypadCodeIds_.clear();
        keypadCodeIds_.reserve(entries.size());
        for (const auto& entry : entries) {
            keypadCodeIds_.push_back(entry.codeId);
            ESP_LOGI(TAG, "keypad #%d %s is %s", entry.codeId, entry.name, entry.enabled ? "enabled" : "disabled");
        }
    } else {
        ESP_LOGE(TAG, "print_keypad_entries: retrieveKeypadEntries failed (result %d)", result);
    }
}

void NukiLockComponent::dump_config() {
    LOG_LOCK(TAG, "Nuki Lock", this);
    LOG_BINARY_SENSOR(TAG, "Is Connected", this->is_connected_);
    LOG_BINARY_SENSOR(TAG, "Is Paired", this->is_paired_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_);
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_);
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_);
}

void NukiLockComponent::notify(Nuki::EventType eventType) {
    this->status_update_ = true;
    this->config_update_ = true;
    ESP_LOGI(TAG, "event notified %d", eventType);
}

// Unpair Button
void NukiLockUnpairButton::press_action() {
    this->parent_->unpair();
}

void NukiLockUnpairButton::dump_config() {
    LOG_BUTTON(TAG, "Unpair", this);
}

void NukiLockComponent::unpair() {
    if(this->nukiLock_.isPairedWithLock()) {
        this->nukiLock_.unPairNuki();
        ESP_LOGI(TAG, "Unpaired Nuki! Turn on Pairing Mode to pair a new Nuki.");
    } else {
        ESP_LOGE(TAG, "Unpair action called for unpaired Nuki");
    }
}

// Pairing Mode Switch
void NukiLockPairingModeSwitch::setup() {
    this->publish_state(false);
}

void NukiLockPairingModeSwitch::dump_config() {
    LOG_SWITCH(TAG, "Pairing Mode", this);
}

void NukiLockPairingModeSwitch::write_state(bool state) {
    this->parent_->set_pairing_mode(state);
}

void NukiLockComponent::set_pairing_mode(bool enabled) {
    this->pairing_mode_ = enabled;
    this->pairing_mode_switch_->publish_state(enabled);

    if(enabled) {
        ESP_LOGI(TAG, "Pairing Mode turned on for %d seconds", this->pairing_mode_timeout_);
        this->pairing_mode_on_callback_.call();

        ESP_LOGI(TAG, "Waiting for Nuki to enter pairing mode...");

        // Turn on for ... seconds
        uint32_t now_millis = millis();
        this->pairing_mode_timer_ = now_millis + (this->pairing_mode_timeout_ * 1000);
    } else {
        ESP_LOGI(TAG, "Pairing Mode turned off");
        this->pairing_mode_timer_ = 0;
        this->pairing_mode_off_callback_.call();
    }
}

void NukiLockComponent::add_pairing_mode_on_callback(std::function<void()> &&callback) {
    this->pairing_mode_on_callback_.add(std::move(callback));
}

void NukiLockComponent::add_pairing_mode_off_callback(std::function<void()> &&callback) {
    this->pairing_mode_off_callback_.add(std::move(callback));
}

void NukiLockComponent::add_paired_callback(std::function<void()> &&callback) {
    this->paired_callback_.add(std::move(callback));
}

} //namespace nuki_lock
} //namespace esphome