#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/api/custom_api_device.h"
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

NukiLock::ButtonPressAction NukiLockComponent::nuki_button_press_action_to_enum(std::string str)
{
    if (str == "No Action") return NukiLock::ButtonPressAction::NoAction;
    if (str == "Intelligent") return NukiLock::ButtonPressAction::Intelligent;
    if (str == "Unlock") return NukiLock::ButtonPressAction::Unlock;
    if (str == "Lock") return NukiLock::ButtonPressAction::Lock;
    if (str == "Unlatch") return NukiLock::ButtonPressAction::Unlatch;
    if (str == "Lock n Go") return NukiLock::ButtonPressAction::LockNgo;
    if (str == "Show Status") return NukiLock::ButtonPressAction::ShowStatus;
    return (NukiLock::ButtonPressAction)0xff;
}

const char* NukiLockComponent::nuki_button_press_action_to_string(NukiLock::ButtonPressAction action)
{
    switch (action)
    {
        case NukiLock::ButtonPressAction::NoAction: return "No Action";
        case NukiLock::ButtonPressAction::Intelligent: return "Intelligent";
        case NukiLock::ButtonPressAction::Unlock: return "Unlock";
        case NukiLock::ButtonPressAction::Lock: return "Lock";
        case NukiLock::ButtonPressAction::Unlatch: return "Unlatch";
        case NukiLock::ButtonPressAction::LockNgo: return "Lock n Go";
        case NukiLock::ButtonPressAction::ShowStatus: return "Show Status";
        default: return "No Action";
    }
}

uint8_t NukiLockComponent::fob_action_to_int(std::string str)
{
    if(str == "No Action") return 0;
    if(str == "Unlock") return 1;
    if(str == "Lock") return 2;
    if(str == "Lock n Go") return 3;
    if(str == "Intelligent") return 4;
    return 99;
}

std::string NukiLockComponent::fob_action_to_string(uint8_t action)
{
    if(action == 0) return "No Action";
    if(action == 1) return "Unlock";
    if(action == 2) return "Lock";
    if(action == 3) return "Lock n Go";
    if(action == 4) return "Intelligent";
    return "No Action";
}

void NukiLockComponent::update_status()
{
    this->status_update_ = false;
    Nuki::CmdResult cmd_result = this->nuki_lock_.requestKeyTurnerState(&(this->retrieved_key_turner_state_));
    char cmd_result_as_string[30];
    NukiLock::cmdResultToString(cmd_result, cmd_result_as_string);

    if (cmd_result == Nuki::CmdResult::Success) {
        this->status_update_consecutive_errors_ = 0;
        NukiLock::LockState currentLockState = this->retrieved_key_turner_state_.lockState;
        char currentLockStateAsString[30];
        NukiLock::lockstateToString(currentLockState, currentLockStateAsString);

        ESP_LOGI(TAG, "Bat state: %#x, Bat crit: %d, Bat perc:%d lock state: %s (%d) %d:%d:%d",
            this->retrieved_key_turner_state_.criticalBatteryState,
            this->nuki_lock_.isBatteryCritical(),
            this->nuki_lock_.getBatteryPerc(),
            currentLockStateAsString,
            currentLockState,
            this->retrieved_key_turner_state_.currentTimeHour,
            this->retrieved_key_turner_state_.currentTimeMinute,
            this->retrieved_key_turner_state_.currentTimeSecond
        );

        this->publish_state(this->nuki_to_lock_state(this->retrieved_key_turner_state_.lockState));

        #ifdef USE_BINARY_SENSOR
        this->is_connected_binary_sensor_->publish_state(true);
        if (this->battery_critical_binary_sensor_ != nullptr)
            this->battery_critical_binary_sensor_->publish_state(this->nuki_lock_.isBatteryCritical());
        if (this->door_sensor_binary_sensor_ != nullptr)
            this->door_sensor_binary_sensor_->publish_state(this->nuki_doorsensor_to_binary(this->retrieved_key_turner_state_.doorSensorState));
        #endif
        #ifdef USE_SENSOR
        if (this->battery_level_sensor_ != nullptr)
            this->battery_level_sensor_->publish_state(this->nuki_lock_.getBatteryPerc());
        #endif
        #ifdef USE_TEXT_SENSOR
        if (this->door_sensor_state_text_sensor_ != nullptr)
            this->door_sensor_state_text_sensor_->publish_state(this->nuki_doorsensor_to_string(this->retrieved_key_turner_state_.doorSensorState));
        #endif

        this->auth_data_update_ = true;
        this->event_log_update_ = true;

        if (
            this->retrieved_key_turner_state_.lockState == NukiLock::LockState::Locking
            || this->retrieved_key_turner_state_.lockState == NukiLock::LockState::Unlocking
        ) {
            // Schedule a status update without waiting for the next advertisement because the lock
            // is in a transition state. This will speed up the feedback.
            this->status_update_ = true;
        }
    } else {
        ESP_LOGE(TAG, "requestKeyTurnerState failed with error %s (%d)", cmd_result_as_string, cmd_result);
        this->status_update_ = true;

        this->status_update_consecutive_errors_++;
        if (this->status_update_consecutive_errors_ > MAX_TOLERATED_UPDATES_ERRORS) {
            // Publish failed state only when having too many consecutive errors
            #ifdef USE_BINARY_SENSOR
            this->is_connected_binary_sensor_->publish_state(false);
            #endif
            this->publish_state(lock::LOCK_STATE_NONE);
        }
    }
}

void NukiLockComponent::update_config() {
    this->config_update_ = false;

    NukiLock::Config config;
    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestConfig(&config);
    char conf_req_result_as_string[30];
    NukiLock::cmdResultToString(conf_req_result, conf_req_result_as_string);

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestConfig has resulted in %s (%d)", conf_req_result_as_string, conf_req_result);
        keypad_paired_ = config.hasKeypad;

        #ifdef USE_SWITCH
        if (this->auto_unlatch_enabled_switch_ != nullptr)
            this->auto_unlatch_enabled_switch_->publish_state(config.autoUnlatch);
        if (this->button_enabled_switch_ != nullptr)
            this->button_enabled_switch_->publish_state(config.buttonEnabled);
        if (this->led_enabled_switch_ != nullptr)
            this->led_enabled_switch_->publish_state(config.ledEnabled);
        #endif
        #ifdef USE_NUMBER
        if (this->led_brightness_number_ != nullptr)
            this->led_brightness_number_->publish_state(config.ledBrightness);
        #endif
        #ifdef USE_SELECT
        if (this->fob_action_1_select_ != nullptr)
            this->fob_action_1_select_->publish_state(this->fob_action_to_string(config.fobAction1));
        if (this->fob_action_2_select_ != nullptr)
            this->fob_action_2_select_->publish_state(this->fob_action_to_string(config.fobAction2));
        if (this->fob_action_3_select_ != nullptr)
            this->fob_action_3_select_->publish_state(this->fob_action_to_string(config.fobAction3));
        #endif

    } else {
        ESP_LOGE(TAG, "requestConfig has resulted in %s (%d)", conf_req_result_as_string, conf_req_result);
        this->config_update_ = true;
    }
}

void NukiLockComponent::update_advanced_config() {
    this->advanced_config_update_ = false;

    NukiLock::AdvancedConfig advanced_config;
    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestAdvancedConfig(&advanced_config);
    char conf_req_result_as_string[30];
    NukiLock::cmdResultToString(conf_req_result, conf_req_result_as_string);

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestAdvancedConfig has resulted in %s (%d)", conf_req_result_as_string, conf_req_result);

        #ifdef USE_SWITCH
        if (this->nightmode_enabled_switch_ != nullptr)
            this->nightmode_enabled_switch_->publish_state(advanced_config.nightModeEnabled);
        
        if (this->night_mode_auto_lock_enabled_switch_ != nullptr)
            this->night_mode_auto_lock_enabled_switch_->publish_state(advanced_config.nightModeAutoLockEnabled);
        
        if (this->night_mode_auto_unlock_disabled_switch_ != nullptr)
            this->night_mode_auto_unlock_disabled_switch_->publish_state(advanced_config.nightModeAutoUnlockDisabled);
        
        if (this->night_mode_immediate_lock_on_start_switch_ != nullptr)
            this->night_mode_immediate_lock_on_start_switch_->publish_state(advanced_config.nightModeImmediateLockOnStart);
        
        if (this->auto_lock_enabled_switch_ != nullptr)
            this->auto_lock_enabled_switch_->publish_state(advanced_config.autoLockEnabled);

        if (this->auto_unlock_disabled_switch_ != nullptr)
            this->auto_unlock_disabled_switch_->publish_state(advanced_config.autoUnLockDisabled);

        if (this->immediate_auto_lock_enabled_switch_ != nullptr)
            this->immediate_auto_lock_enabled_switch_->publish_state(advanced_config.immediateAutoLockEnabled);

        if (this->auto_update_enabled_switch_ != nullptr)
            this->auto_update_enabled_switch_->publish_state(advanced_config.autoUpdateEnabled);
        #endif
        #ifdef USE_SELECT
        if (this->single_button_press_action_select_ != nullptr)
            this->single_button_press_action_select_->publish_state(this->nuki_button_press_action_to_string(advanced_config.singleButtonPressAction));

        if (this->double_button_press_action_select_ != nullptr)
            this->double_button_press_action_select_->publish_state(this->nuki_button_press_action_to_string(advanced_config.doubleButtonPressAction));
        #endif

    } else {
        ESP_LOGE(TAG, "requestAdvancedConfig has resulted in %s (%d)", conf_req_result_as_string, conf_req_result);
        this->advanced_config_update_ = true;
    }
}

void NukiLockComponent::update_auth_data()
{
    this->auth_data_update_ = false;

    Nuki::CmdResult conf_req_result = (Nuki::CmdResult)-1;
    int retryCount = 0;
    while(retryCount < 3)
    {
        ESP_LOGD(TAG, "Retrieve Auth Data");
        conf_req_result = this->nuki_lock_.retrieveAuthorizationEntries(0, MAX_AUTH_DATA_ENTRIES);

        if(conf_req_result != Nuki::CmdResult::Success)
        {
            ++retryCount;
            App.feed_wdt();
        }
        else
        {
            break;
        }
    }

    App.feed_wdt();
    delay(5000);

    std::list<NukiLock::AuthorizationEntry> authEntries;
    this->nuki_lock_.getAuthorizationEntries(&authEntries);

    authEntries.sort([](const NukiLock::AuthorizationEntry& a, const NukiLock::AuthorizationEntry& b)
    {
        return a.authId < b.authId;
    });

    if(authEntries.size() > MAX_AUTH_DATA_ENTRIES)
    {
        authEntries.resize(MAX_AUTH_DATA_ENTRIES);
    }

    for(const auto& entry : authEntries)
    {
        ESP_LOGD(TAG, "Authorization entry[%d] type: %d name: %s", entry.authId, entry.idType, entry.name);
        this->auth_entries_[entry.authId] = std::string(reinterpret_cast<const char*>(entry.name));
    }
}

void NukiLockComponent::update_event_logs()
{
    this->event_log_update_ = false;

    Nuki::CmdResult conf_req_result = (Nuki::CmdResult)-1;
    int retryCount = 0;
    while(retryCount < 3)
    {
        ESP_LOGD(TAG, "Retrieve Event Logs");
        conf_req_result = this->nuki_lock_.retrieveLogEntries(0, MAX_EVENT_LOG_ENTRIES, 1, false);

        if(conf_req_result != Nuki::CmdResult::Success)
        {
            ++retryCount;
            App.feed_wdt();
        }
        else
        {
            break;
        }
    }

    App.feed_wdt();
    delay(5000);

    std::list<NukiLock::LogEntry> log;
    this->nuki_lock_.getLogEntries(&log);

    if(log.size() > MAX_EVENT_LOG_ENTRIES)
    {
        log.resize(MAX_EVENT_LOG_ENTRIES);
    }

    log.sort([](const NukiLock::LogEntry& a, const NukiLock::LogEntry& b)
    {
        return a.index < b.index;
    });

    if(log.size() > 0)
    {
        this->process_log_entries(log);
    }
}

void NukiLockComponent::process_log_entries(const std::list<NukiLock::LogEntry>& logEntries)
{
    char str[50];
    char authName[33];
    uint32_t authIndex = 0;
    std::map<std::string, std::string> event_data;

    for(const auto& log : logEntries)
    {
        memset(authName, 0, sizeof(authName));
        authName[0] = '\0';

        if((log.loggingType == NukiLock::LoggingType::LockAction || log.loggingType == NukiLock::LoggingType::KeypadAction))
        {
            int sizeName = sizeof(log.name);
            memcpy(authName, log.name, sizeName);
            if(authName[sizeName - 1] != '\0')
            {
                authName[sizeName] = '\0';
            }

            if(log.index > authIndex)
            {
                authIndex = log.index;
                this->auth_id_ = log.authId;

                memset(this->auth_name_, 0, sizeof(this->auth_name_));
                memcpy(this->auth_name_, authName, sizeof(authName));

                if(authName[sizeName - 1] != '\0' && this->auth_entries_.count(this->auth_id_) > 0)
                {
                    memset(this->auth_name_, 0, sizeof(this->auth_name_));
                    memcpy(this->auth_name_, this->auth_entries_[this->auth_id_].c_str(), sizeof(this->auth_entries_[this->auth_id_].c_str()));
                }
            }
        }

        event_data["index"] = std::to_string(log.index);
        event_data["authorizationId"] = std::to_string(log.authId);
        event_data["authorizationName"] = authName;

        if(this->auth_entries_.count(log.authId) > 0)
        {
            event_data["authorizationName"] = this->auth_entries_[log.authId];
        }

        event_data["timeYear"] = std::to_string(log.timeStampYear);
        event_data["timeMonth"] = std::to_string(log.timeStampMonth);
        event_data["timeDay"] = std::to_string(log.timeStampDay);
        event_data["timeHour"] = std::to_string(log.timeStampHour);
        event_data["timeMinute"] = std::to_string(log.timeStampMinute);
        event_data["timeSecond"] = std::to_string(log.timeStampSecond);

        memset(str, 0, sizeof(str));
        NukiLock::loggingTypeToString(log.loggingType, str);
        event_data["type"] = str;

        switch(log.loggingType)
        {
            case NukiLock::LoggingType::LockAction:
                memset(str, 0, sizeof(str));
                NukiLock::lockactionToString((NukiLock::LockAction)log.data[0], str);
                event_data["action"] = str;

                memset(str, 0, sizeof(str));
                NukiLock::triggerToString((NukiLock::Trigger)log.data[1], str);
                event_data["trigger"] = str;

                memset(str, 0, sizeof(str));
                NukiLock::completionStatusToString((NukiLock::CompletionStatus)log.data[3], str);
                event_data["completionStatus"] = str;
                break;

            case NukiLock::LoggingType::KeypadAction:
                memset(str, 0, sizeof(str));
                NukiLock::lockactionToString((NukiLock::LockAction)log.data[0], str);
                event_data["action"] = str;

                switch(log.data[1])
                {
                    case 0:
                        event_data["trigger"] = "arrowkey";
                        break;
                    case 1:
                        event_data["trigger"] = "code";
                        break;
                    case 2:
                        event_data["trigger"] = "fingerprint";
                        break;
                    default:
                        event_data["trigger"] = "unknown";
                        break;
                }

                memset(str, 0, sizeof(str));

                if(log.data[2] == 9)
                {
                    event_data["trigger"] = "notAuthorized";
                }
                else if (log.data[2] == 224)
                {
                    event_data["trigger"] = "invalidCode";
                }
                else
                {
                    NukiLock::completionStatusToString((NukiLock::CompletionStatus)log.data[2], str);
                    event_data["completionStatus"] = str;
                }

                event_data["codeId"] = std::to_string(256U*log.data[4]+log.data[3]);
                break;

            case NukiLock::LoggingType::DoorSensor:
                switch(log.data[0])
                {
                    case 0:
                        event_data["action"] = "DoorOpened";
                        break;
                    case 1:
                        event_data["action"] = "DoorClosed";
                        break;
                    case 2:
                        event_data["action"] = "SensorJammed";
                        break;
                    default:
                        event_data["action"] = "Unknown";
                        break;
                }
                break;
        }
        
        // Send as Home Assistant Event
        if(log.index > this->last_rolling_log_id)
        {
            this->last_rolling_log_id = log.index;

            if (strcmp(event_, "esphome.none") != 0)
            {
                auto capi = new esphome::api::CustomAPIDevice();
                ESP_LOGD(TAG, "Send event to Home Assistant on %s", event_);
                capi->fire_homeassistant_event(event_, event_data);
            }
        }
    }

    #ifdef USE_TEXT_SENSOR
    if (this->last_unlock_user_text_sensor_ != nullptr)
        this->last_unlock_user_text_sensor_->publish_state(this->auth_name_);
    #endif
}

bool NukiLockComponent::execute_lock_action(NukiLock::LockAction lockAction) {
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
    Nuki::CmdResult result = this->nuki_lock_.lockAction(lockAction);

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

    this->nuki_lock_.registerBleScanner(&this->scanner_);
    this->nuki_lock_.initialize();
    this->nuki_lock_.setConnectTimeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nuki_lock_.setConnectRetries(BLE_CONNECT_TIMEOUT_RETRIES);

    if(this->security_pin_ > 0) {
        bool result = this->nuki_lock_.saveSecurityPincode(this->security_pin_);
        if (result) {
            ESP_LOGI(TAG, "Set pincode done");
        } else {
            ESP_LOGE(TAG, "Set pincode failed!");
        }
    }
    
    if (this->nuki_lock_.isPairedWithLock()) {
        this->status_update_ = true;
        this->config_update_ = true;
        this->advanced_config_update_ = true;
        this->auth_data_update_ = true;
        this->event_log_update_ = true;
        ESP_LOGI(TAG, "%s Nuki paired", this->deviceName_);
        #ifdef USE_BINARY_SENSOR
        this->is_paired_binary_sensor_->publish_initial_state(true);
        #endif
    } else {
        ESP_LOGW(TAG, "%s Nuki is not paired", this->deviceName_);
        #ifdef USE_BINARY_SENSOR
        this->is_paired_binary_sensor_->publish_initial_state(false);
        #endif
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
    this->nuki_lock_.updateConnectionState();

    if(this->pairing_mode_ && this->pairing_mode_timer_ != 0) {
        if(millis() > this->pairing_mode_timer_) {
            ESP_LOGV(TAG, "Pairing timed out, turning off pairing mode");
            this->set_pairing_mode(false);
        }
    }

    if (millis() - last_command_executed_time_ < command_cooldown_millis) {
        // Give the lock time to terminate the previous command
        uint32_t millisSinceLastExecution = millis() - last_command_executed_time_;
        uint32_t millisLeft = (millisSinceLastExecution < command_cooldown_millis) ? command_cooldown_millis - millisSinceLastExecution : 1;
        ESP_LOGV(TAG, "Cooldown period, %dms left", millisLeft);
        return;
    }

    if (this->nuki_lock_.isPairedWithLock()) {
        #ifdef USE_BINARY_SENSOR
        this->is_paired_binary_sensor_->publish_state(true);
        #endif

        // Execute (all) actions first, then status updates, then config updates.
        // Only one command (action, status, config, or auth data) is executed per update() call.
        if (this->action_attempts_ > 0) {
            this->action_attempts_--;

            NukiLock::LockAction currentLockAction = this->lock_action_;
            char currentLockActionAsString[30];
            NukiLock::lockactionToString(currentLockAction, currentLockActionAsString);
            ESP_LOGD(TAG, "Executing lock action %s (%d)... (%d attempts left)", currentLockActionAsString, currentLockAction, this->action_attempts_);

            bool isExecutionSuccessful = this->execute_lock_action(currentLockAction);

            if (isExecutionSuccessful) {
                if(this->lock_action_ == currentLockAction) {
                    // Stop action attempts only if no new action was received in the meantime.
                    // Otherwise, the new action won't be executed.
                    this->action_attempts_ = 0;
                }
            } else if (this->action_attempts_ == 0) {
                // Publish failed state only when no attempts are left
                #ifdef USE_BINARY_SENSOR
                this->is_connected_binary_sensor_->publish_state(false);
                #endif
                this->publish_state(lock::LOCK_STATE_NONE);
            }

            // Schedule a status update without waiting for the next advertisement for a faster feedback
            this->status_update_ = true;

            // Give the lock extra time when successful in order to account for time to turn the key
            command_cooldown_millis = isExecutionSuccessful ? COOLDOWN_COMMANDS_EXTENDED_MILLIS : COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();

        } else if (this->status_update_) {
            ESP_LOGD(TAG, "Update present, getting data...");
            this->update_status();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();

        } else if (this->config_update_) {
            ESP_LOGD(TAG, "Update present, getting config...");
            this->update_config();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();
        } else if (this->advanced_config_update_) {
            ESP_LOGD(TAG, "Update present, getting advanced config...");
            this->update_advanced_config();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();
        } else if (this->auth_data_update_) {
            ESP_LOGD(TAG, "Update present, getting auth data...");
            this->update_auth_data();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();
        } else if (this->event_log_update_) {
            ESP_LOGD(TAG, "Update present, getting event logs...");
            this->update_event_logs();

            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            last_command_executed_time_ = millis();
        }
    } else {
        #ifdef USE_BINARY_SENSOR
        this->is_paired_binary_sensor_->publish_state(false);
        this->is_connected_binary_sensor_->publish_state(false);
        #endif

        // Pairing Mode is active
        if(this->pairing_mode_) {
            // Pair Nuki
            bool paired = (this->nuki_lock_.pairNuki() == Nuki::PairingResult::Success);
            if (paired) {
                ESP_LOGI(TAG, "Nuki paired successfuly!");
                this->update_status();
                this->paired_callback_.call();
                this->set_pairing_mode(false);
            }
            #ifdef USE_BINARY_SENSOR
            this->is_paired_binary_sensor_->publish_state(paired);
            #endif
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
            this->action_attempts_ = MAX_ACTION_ATTEMPTS;
            this->lock_action_ = NukiLock::LockAction::Lock;
            break;

        case lock::LOCK_STATE_UNLOCKED: {
            this->action_attempts_ = MAX_ACTION_ATTEMPTS;
            this->lock_action_ = NukiLock::LockAction::Unlock;

            if(this->open_latch_) {
                this->lock_action_ = NukiLock::LockAction::Unlatch;
            }

            if(this->lock_n_go_) {
                this->lock_action_ = NukiLock::LockAction::LockNgo;
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
    NukiLock::lockactionToString(this->lock_action_, lockActionAsString);
    ESP_LOGI(TAG, "New lock action received: %s (%d)", lockActionAsString, this->lock_action_);
}

void NukiLockComponent::lock_n_go() {
    this->lock_n_go_ = true;
    this->unlock();
}

bool NukiLockComponent::valid_keypad_id(int id) {
    bool idValid = std::find(keypad_code_ids_.begin(), keypad_code_ids_.end(), id) != keypad_code_ids_.end();
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
    Nuki::CmdResult result = this->nuki_lock_.addKeypadEntry(entry);
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
    Nuki::CmdResult result = this->nuki_lock_.updateKeypadEntry(entry);
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

    Nuki::CmdResult result = this->nuki_lock_.deleteKeypadEntry(id);
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

    Nuki::CmdResult result = this->nuki_lock_.retrieveKeypadEntries(0, 0xffff);
    if(result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "retrieveKeypadEntries sucess");
        std::list<NukiLock::KeypadEntry> entries;
        this->nuki_lock_.getKeypadEntries(&entries);

        entries.sort([](const NukiLock::KeypadEntry& a, const NukiLock::KeypadEntry& b) { return a.codeId < b.codeId; });

        keypad_code_ids_.clear();
        keypad_code_ids_.reserve(entries.size());
        for (const auto& entry : entries) {
            keypad_code_ids_.push_back(entry.codeId);
            ESP_LOGI(TAG, "keypad #%d %s is %s", entry.codeId, entry.name, entry.enabled ? "enabled" : "disabled");
        }
    } else {
        ESP_LOGE(TAG, "print_keypad_entries: retrieveKeypadEntries failed (result %d)", result);
    }
}

void NukiLockComponent::dump_config() {
    ESP_LOGCONFIG(TAG, "NUKI LOCK:");

    LOG_LOCK(TAG, "Nuki Lock", this);
    #ifdef USE_BINARY_SENSOR
    LOG_BINARY_SENSOR(TAG, "Is Connected", this->is_connected_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Is Paired", this->is_paired_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_binary_sensor_);
    #endif
    #ifdef USE_TEXT_SENSOR
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Unlock User", this->last_unlock_user_text_sensor_);
    #endif
    #ifdef USE_SENSOR
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_sensor_);
    #endif
    #ifdef USE_SWITCH
    LOG_SWITCH(TAG, "Pairing Mode", this->pairing_mode_switch_);
    LOG_SWITCH(TAG, "Auto Unlatch Enabled", this->auto_unlatch_enabled_switch_);
    LOG_SWITCH(TAG, "Button Enabled", this->button_enabled_switch_);
    LOG_SWITCH(TAG, "LED Enabled", this->led_enabled_switch_);
    LOG_SWITCH(TAG, "Night Mode Enabled", this->nightmode_enabled_switch_);
    LOG_SWITCH(TAG, "Night Mode Auto Lock", this->night_mode_auto_lock_enabled_switch_);
    LOG_SWITCH(TAG, "Night Mode Auto Unlock Disabled", this->night_mode_auto_unlock_disabled_switch_);
    LOG_SWITCH(TAG, "Night Mode Immediate Lock On Start", this->night_mode_immediate_lock_on_start_switch_);
    LOG_SWITCH(TAG, "Auto Lock", this->auto_lock_enabled_switch_);
    LOG_SWITCH(TAG, "Auto Unlock Disabled", this->auto_unlock_disabled_switch_);
    LOG_SWITCH(TAG, "Immediate Auto Lock", this->immediate_auto_lock_enabled_switch_);
    LOG_SWITCH(TAG, "Automatic Updates", this->auto_update_enabled_switch_);
    #endif
    #ifdef USE_NUMBER
    LOG_NUMBER(TAG, "LED Brightness", this->led_brightness_number_);
    #endif
    #ifdef USE_SELECT
    LOG_SELECT(TAG, "Single Button Press Action", this->single_button_press_action_select_);
    LOG_SELECT(TAG, "Double Button Press Action", this->double_button_press_action_select_);
    #endif
}

void NukiLockComponent::notify(Nuki::EventType eventType) {
    this->status_update_ = true;
    this->config_update_ = true;
    this->advanced_config_update_ = true;
    this->auth_data_update_ = true;
    this->event_log_update_ = true;
    ESP_LOGI(TAG, "event notified %d", eventType);
}

void NukiLockComponent::unpair() {
    if(this->nuki_lock_.isPairedWithLock()) {
        this->nuki_lock_.unPairNuki();
        ESP_LOGI(TAG, "Unpaired Nuki! Turn on Pairing Mode to pair a new Nuki.");
    } else {
        ESP_LOGE(TAG, "Unpair action called for unpaired Nuki");
    }
}

void NukiLockComponent::set_pairing_mode(bool enabled) {
    this->pairing_mode_ = enabled;

    #ifdef USE_SWITCH
    if (this->pairing_mode_switch_ != nullptr) {
        this->pairing_mode_switch_->publish_state(enabled);
    }
    #endif

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

#ifdef USE_SELECT
void NukiLockComponent::set_config_select(std::string config, const std::string &value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if(config == "single_button_press_action")
    {
        NukiLock::ButtonPressAction action = this->nuki_button_press_action_to_enum(value);
        cmd_result = this->nuki_lock_.setSingleButtonPressAction(action);
        is_advanced = true;
    }
    else if(config == "double_button_press_action")
    {
        NukiLock::ButtonPressAction action = this->nuki_button_press_action_to_enum(value);
        cmd_result = this->nuki_lock_.setDoubleButtonPressAction(action);
        is_advanced = true;
    }
    else if(config == "fob_action_1")
    {
        const uint8_t action = this->fob_action_to_int(value);
        if(action != 99)
        {
            cmd_result = this->nuki_lock_.setFobAction(1, action);
        }
    }
    else if(config == "fob_action_2")
    {
        const uint8_t action = this->fob_action_to_int(value);
        if(action != 99)
        {
            cmd_result = this->nuki_lock_.setFobAction(2, action);
        }
    }
    else if(config == "fob_action_3")
    {
        const uint8_t action = this->fob_action_to_int(value);
        if(action != 99)
        {
            cmd_result = this->nuki_lock_.setFobAction(3, action);
        }
    }

    if(cmd_result == Nuki::CmdResult::Success)
    {
        if(config == "single_button_press_action")
        {
            if (this->single_button_press_action_select_ != nullptr) this->single_button_press_action_select_->publish_state(value);
        } 
        else if(config == "double_button_press_action")
        {
            if (this->double_button_press_action_select_ != nullptr) this->double_button_press_action_select_->publish_state(value);
        } 
        else if(config == "fob_action_1")
        {
            if (this->fob_action_1_select_ != nullptr) this->fob_action_1_select_->publish_state(value);
        } 
        else if(config == "fob_action_2")
        {
            if (this->fob_action_2_select_ != nullptr) this->fob_action_2_select_->publish_state(value);
        } 
        else if(config == "fob_action_3")
        {
            if (this->fob_action_3_select_ != nullptr) this->fob_action_3_select_->publish_state(value);
        } 
        
        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    }
}
#endif

#ifdef USE_SWITCH
void NukiLockComponent::set_config_switch(std::string config, bool value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if(config == "auto_unlatch_enabled")
    {
        cmd_result = this->nuki_lock_.enableAutoUnlatch(value);
    }
    else if(config == "button_enabled")
    {
        cmd_result = this->nuki_lock_.enableButton(value);
    }
    else if(config == "led_enabled")
    {
        cmd_result = this->nuki_lock_.enableLedFlash(value);
    }
    else if(config == "nightmode_enabled")
    {
        cmd_result = this->nuki_lock_.enableNightMode(value);
        is_advanced = true;
    }
    else if(config == "night_mode_auto_lock_enabled")
    {
        cmd_result = this->nuki_lock_.enableNightModeAutoLock(value);
        is_advanced = true;
    }
    else if(config == "night_mode_auto_unlock_disabled")
    {
        cmd_result = this->nuki_lock_.disableNightModeAutoUnlock(value);
        is_advanced = true;
    }
    else if(config == "night_mode_immediate_lock_on_start")
    {
        cmd_result = this->nuki_lock_.enableNightModeImmediateLockOnStart(value);
        is_advanced = true;
    }
    else if(config == "auto_lock_enabled")
    {
        cmd_result = this->nuki_lock_.enableAutoLock(value);
        is_advanced = true;
    }
    else if(config == "auto_unlock_disabled")
    {
        cmd_result = this->nuki_lock_.disableAutoUnlock(value);
        is_advanced = true;
    }
    else if(config == "immediate_auto_lock_enabled")
    {
        cmd_result = this->nuki_lock_.enableImmediateAutoLock(value);
        is_advanced = true;
    }
    else if(config == "auto_update_enabled")
    {
        cmd_result = this->nuki_lock_.enableAutoUpdate(value);
        is_advanced = true;
    }

    if(cmd_result == Nuki::CmdResult::Success)
    {
        if(config == "auto_unlatch_enabled")
        {
            if (this->auto_unlatch_enabled_switch_ != nullptr) this->auto_unlatch_enabled_switch_->publish_state(value);
        }
        else if(config == "button_enabled")
        {
            if (this->button_enabled_switch_ != nullptr) this->button_enabled_switch_->publish_state(value);
        }        
        else if(config == "led_enabled")
        {
            if (this->led_enabled_switch_ != nullptr) this->led_enabled_switch_->publish_state(value);
        }        
        else if(config == "nightmode_enabled")
        {
            if (this->nightmode_enabled_switch_ != nullptr) this->nightmode_enabled_switch_->publish_state(value);
        }        
        else if(config == "night_mode_auto_lock_enabled")
        {
            if (this->night_mode_auto_lock_enabled_switch_ != nullptr) this->night_mode_auto_lock_enabled_switch_->publish_state(value);
        }        
        else if(config == "night_mode_auto_unlock_disabled")
        {
            if (this->night_mode_auto_unlock_disabled_switch_ != nullptr) this->night_mode_auto_unlock_disabled_switch_->publish_state(value);
        }        
        else if(config == "night_mode_immediate_lock_on_start")
        {
            if (this->night_mode_immediate_lock_on_start_switch_ != nullptr) this->night_mode_immediate_lock_on_start_switch_->publish_state(value);
        }        
        else if(config == "auto_lock_enabled")
        {
            if (this->auto_lock_enabled_switch_ != nullptr) this->auto_lock_enabled_switch_->publish_state(value);
        }        
        else if(config == "auto_unlock_disabled")
        {
            if (this->auto_unlock_disabled_switch_ != nullptr) this->auto_unlock_disabled_switch_->publish_state(value);
        }        
        else if(config == "immediate_auto_lock_enabled")
        {
            if (this->immediate_auto_lock_enabled_switch_ != nullptr) this->immediate_auto_lock_enabled_switch_->publish_state(value);
        }        
        else if(config == "auto_update_enabled")
        {
            if (this->auto_update_enabled_switch_ != nullptr) this->auto_update_enabled_switch_->publish_state(value);
        }        
        
        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    }
}
#endif
#ifdef USE_NUMBER
void NukiLockComponent::set_config_number(std::string config, float value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if(config == "led_brightness")
    {
        cmd_result = this->nuki_lock_.setLedBrightness(value);
    }

    if(cmd_result == Nuki::CmdResult::Success)
    {
        if(config == "led_brightness")
        {
            if (this->led_brightness_number_ != nullptr) this->led_brightness_number_->publish_state(value);
        } 
        
        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    }
}
#endif

#ifdef USE_BUTTON
void NukiLockUnpairButton::press_action() {
    this->parent_->unpair();
}
#endif
#ifdef USE_SELECT
void NukiLockSingleButtonPressActionSelect::control(const std::string &action) {
    this->parent_->set_config_select("single_button_press_action", action);
}
void NukiLockDoubleButtonPressActionSelect::control(const std::string &action) {
    this->parent_->set_config_select("double_button_press_action", action);
}
void NukiLockFobAction1Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_1", action);
}
void NukiLockFobAction2Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_2", action);
}
void NukiLockFobAction3Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_3", action);
}
#endif
#ifdef USE_SWITCH
void NukiLockPairingModeSwitch::write_state(bool state) {
    this->parent_->set_pairing_mode(state);
}

void NukiLockAutoUnlatchEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("auto_unlatch_enabled", state);
}

void NukiLockButtonEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("button_enabled", state);
}

void NukiLockLedEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("led_enabled", state);
}

void NukiLockNightModeEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("nightmode_enabled", state);
}

void NukiLockNightModeAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("night_mode_auto_lock_enabled", state);
}

void NukiLockNightModeAutoUnlockDisabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("night_mode_auto_unlock_disabled", state);
}

void NukiLockNightModeImmediateLockOnStartEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("night_mode_immediate_lock_on_start", state);
}

void NukiLockAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("auto_lock_enabled", state);
}

void NukiLockAutoUnlockDisabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("auto_unlock_disabled", state);
}

void NukiLockImmediateAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("immediate_auto_lock_enabled", state);
}

void NukiLockAutoUpdateEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("auto_update_enabled", state);
}
#endif
#ifdef USE_NUMBER
void NukiLockLedBrightnessNumber::control(float value) {
    this->parent_->set_config_number("led_brightness", value);
}
#endif

// Callbacks
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