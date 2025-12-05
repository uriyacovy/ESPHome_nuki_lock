#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#include <map>

#include "nuki_lock.h"
#include "utils.h"

namespace esphome::nuki_lock {

uint32_t global_nuki_lock_id = 1912044075ULL;

void NukiLockComponent::save_settings() {
    NukiLockSettings settings {
        this->security_pin_,
        this->pin_state_
    };

    if (!this->pref_.save(&settings)) {
        ESP_LOGW(TAG, "Failed to save settings");
    }
}

void NukiLockComponent::update_status() {
    this->status_update_ = false;

    char str[50] = {0};

    Nuki::CmdResult cmd_result = this->nuki_lock_.requestKeyTurnerState(&(this->retrieved_key_turner_state_));
    NukiLock::cmdResultToString(cmd_result, str);

    App.feed_wdt();

    if (cmd_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestKeyTurnerState has resulted in %s (%d)", str, cmd_result);

        this->status_update_consecutive_errors_ = 0;
        this->connected_ = true;

        NukiLock::LockState current_lock_state = this->retrieved_key_turner_state_.lockState;
        char current_lock_state_as_string[30] = {0};
        NukiLock::lockstateToString(current_lock_state, current_lock_state_as_string);

        ESP_LOGI(TAG, "Lock state: %s (%d), Battery (state: %#x, critical: %d, level: %d, charging: %s), Time: %d:%d:%d",
            current_lock_state_as_string,
            current_lock_state,
            this->retrieved_key_turner_state_.criticalBatteryState,
            this->nuki_lock_.isBatteryCritical(),
            this->nuki_lock_.getBatteryPerc(),
            YESNO(this->nuki_lock_.isBatteryCharging()),
            this->retrieved_key_turner_state_.currentTimeHour,
            this->retrieved_key_turner_state_.currentTimeMinute,
            this->retrieved_key_turner_state_.currentTimeSecond
        );

        this->publish_state(nuki_lock::nuki_to_lock_state(this->retrieved_key_turner_state_.lockState));

        #ifdef USE_BINARY_SENSOR
        if (this->connected_binary_sensor_ != nullptr) {
            this->connected_binary_sensor_->publish_state(this->connected_);
        }
        
        if (this->battery_critical_binary_sensor_ != nullptr) {
            this->battery_critical_binary_sensor_->publish_state(this->nuki_lock_.isBatteryCritical());
        }

        // If pin needs validation, validate now
        if(this->pin_state_ == PinState::Set) {
            validate_pin();
        }
        
        if (this->door_sensor_binary_sensor_ != nullptr) {
            Nuki::DoorSensorState door_sensor_state = this->retrieved_key_turner_state_.doorSensorState;
            if(door_sensor_state != Nuki::DoorSensorState::Unavailable) {
                this->door_sensor_binary_sensor_->publish_state(nuki_lock::nuki_doorsensor_to_binary(door_sensor_state));
            } else {
                this->door_sensor_binary_sensor_->invalidate_state();
            }
        }
        #endif
        #ifdef USE_SENSOR
        if (this->battery_level_sensor_ != nullptr) {
            this->battery_level_sensor_->publish_state(this->nuki_lock_.getBatteryPerc());
        }
        if (this->bt_signal_sensor_ != nullptr) {
            this->bt_signal_sensor_->publish_state(this->nuki_lock_.getRssi());
        }
        #endif
        #ifdef USE_TEXT_SENSOR
        if (this->door_sensor_state_text_sensor_ != nullptr){
            memset(str, 0, sizeof(str));
            NukiLock::doorSensorStateToString(this->retrieved_key_turner_state_.doorSensorState, str);
            this->door_sensor_state_text_sensor_->publish_state(str);
        }

        if (this->last_lock_action_text_sensor_ != nullptr) {
            memset(str, 0, sizeof(str));
            NukiLock::lockactionToString(this->retrieved_key_turner_state_.lastLockAction, str);
            this->last_lock_action_text_sensor_->publish_state(str);
        }

        if (this->last_lock_action_trigger_text_sensor_ != nullptr) {
            memset(str, 0, sizeof(str));
            NukiLock::triggerToString(this->retrieved_key_turner_state_.lastLockActionTrigger, str);
            this->last_lock_action_trigger_text_sensor_->publish_state(str);
        }

        if (this->last_unlock_user_text_sensor_ != nullptr && this->retrieved_key_turner_state_.lastLockActionTrigger == NukiLock::Trigger::Manual)
        {
            this->last_unlock_user_text_sensor_->publish_state("Manual");
        }
        #endif

        if (this->retrieved_key_turner_state_.lockState == NukiLock::LockState::Locking || 
            this->retrieved_key_turner_state_.lockState == NukiLock::LockState::Unlocking) {
            // Schedule a status update without waiting for the next advertisement because the lock
            // is in a transition state. This will speed up the feedback.
            this->status_update_ = true;
            
            if (this->send_events_) {
                this->event_log_update_ = true;
            }
        }
    } else {
        ESP_LOGE(TAG, "requestKeyTurnerState has resulted in %s (%d)", str, cmd_result);

        this->status_update_ = true;
        this->status_update_consecutive_errors_++;

        if (this->status_update_consecutive_errors_ > MAX_TOLERATED_UPDATES_ERRORS) {
            this->connected_ = false;

            // Publish failed state only when having too many consecutive errors
            this->publish_state(lock::LOCK_STATE_NONE);

            #ifdef USE_BINARY_SENSOR
            if (this->connected_binary_sensor_ != nullptr) {
                this->connected_binary_sensor_->publish_state(this->connected_);
            }
            #endif
        }
    }
}

void NukiLockComponent::update_config() {
    this->config_update_ = false;

    char str[50] = {0};

    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestConfig(&this->nuki_lock_config_);
    NukiLock::cmdResultToString(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestConfig has resulted in %s (%d)", str, conf_req_result);

        ESP_LOGD(TAG, "Device Type: %i", (this->nuki_lock_config_.deviceType == 255 ? 0 : this->nuki_lock_config_.deviceType));
        ESP_LOGD(TAG, "Product Variant: %i", (this->nuki_lock_config_.productVariant == 255 ? 0 : this->nuki_lock_config_.productVariant));

        ESP_LOGD(TAG, "Firmware: %i.%i.%i", this->nuki_lock_config_.firmwareVersion[0], this->nuki_lock_config_.firmwareVersion[1], this->nuki_lock_config_.firmwareVersion[2]);
        ESP_LOGD(TAG, "Hardware: %i.%i", this->nuki_lock_config_.hardwareRevision[0], this->nuki_lock_config_.hardwareRevision[1]);

        ESP_LOGD(TAG, "Has Wifi: %s", YESNO(this->nuki_lock_config_.capabilities == 255 ? 0 : this->nuki_lock_config_.capabilities & 1));
        ESP_LOGD(TAG, "Has Thread: %s", YESNO(this->nuki_lock_config_.capabilities == 255 ? 0 : ((this->nuki_lock_config_.capabilities & 2) != 0 ? 1 : 0)));

        ESP_LOGD(TAG, "Matter Status: %i", (this->nuki_lock_config_.matterStatus == 255 ? 0 : this->nuki_lock_config_.matterStatus));
        memset(str, 0, sizeof(str));
        nuki_lock::homekit_status_to_string(this->nuki_lock_config_.homeKitStatus, str);
        ESP_LOGD(TAG, "Homekit Status: %s", str);

        keypad_paired_ = this->nuki_lock_config_.hasKeypad || (this->nuki_lock_config_.hasKeypadV2 != 0 && this->nuki_lock_config_.hasKeypadV2 != 255);

        #ifdef USE_SWITCH
        if (this->pairing_enabled_switch_ != nullptr) {
            this->pairing_enabled_switch_->publish_state(this->nuki_lock_config_.pairingEnabled);
        }

        if (this->auto_unlatch_enabled_switch_ != nullptr) {
            this->auto_unlatch_enabled_switch_->publish_state(this->nuki_lock_config_.autoUnlatch);
        }
        
        if (this->button_enabled_switch_ != nullptr) {
            this->button_enabled_switch_->publish_state(this->nuki_lock_config_.buttonEnabled);
        }
        
        if (this->led_enabled_switch_ != nullptr) {
            this->led_enabled_switch_->publish_state(this->nuki_lock_config_.ledEnabled);
        }
        
        if (this->single_lock_enabled_switch_ != nullptr) {
            this->single_lock_enabled_switch_->publish_state(this->nuki_lock_config_.singleLock);
        }
        
        if (this->dst_mode_enabled_switch_ != nullptr) {
            this->dst_mode_enabled_switch_->publish_state(this->nuki_lock_config_.dstMode);
        }
        #endif
        #ifdef USE_NUMBER
        if (this->led_brightness_number_ != nullptr) {
            this->led_brightness_number_->publish_state(this->nuki_lock_config_.ledBrightness);
        }
        
        if (this->timezone_offset_number_ != nullptr) {
            this->timezone_offset_number_->publish_state(this->nuki_lock_config_.timeZoneOffset);
        }
        #endif
        #ifdef USE_SELECT
        if (this->fob_action_1_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::fob_action_to_string(this->nuki_lock_config_.fobAction1, str);
            this->fob_action_1_select_->publish_state(str);
        }
        
        if (this->fob_action_2_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::fob_action_to_string(this->nuki_lock_config_.fobAction2, str);
            this->fob_action_2_select_->publish_state(str);
        }
        
        if (this->fob_action_3_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::fob_action_to_string(this->nuki_lock_config_.fobAction3, str);
            this->fob_action_3_select_->publish_state(str);
        }
        
        if (this->timezone_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::timezone_to_string(this->nuki_lock_config_.timeZoneId, str);
            this->timezone_select_->publish_state(str);
        }
        
        if (this->advertising_mode_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::advertising_mode_to_string(this->nuki_lock_config_.advertisingMode, str);
            this->advertising_mode_select_->publish_state(str);
        }
        #endif
    } else {
        ESP_LOGE(TAG, "requestConfig has resulted in %s (%d)", str, conf_req_result);
        this->config_update_ = true;
    }
}

void NukiLockComponent::update_advanced_config() {
    this->advanced_config_update_ = false;

    char str[50] = {0};

    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestAdvancedConfig(&this->nuki_lock_advanced_config_);
    NukiLock::cmdResultToString(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestAdvancedConfig has resulted in %s (%d)", str, conf_req_result);

        #ifdef USE_SWITCH
        if (this->nightmode_enabled_switch_ != nullptr) {
            this->nightmode_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.nightModeEnabled);
        }

        if (this->night_mode_auto_lock_enabled_switch_ != nullptr) {
            this->night_mode_auto_lock_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.nightModeAutoLockEnabled);
        }

        if (this->night_mode_auto_unlock_disabled_switch_ != nullptr) {
            this->night_mode_auto_unlock_disabled_switch_->publish_state(this->nuki_lock_advanced_config_.nightModeAutoUnlockDisabled);
        }

        if (this->night_mode_immediate_lock_on_start_switch_ != nullptr) {
            this->night_mode_immediate_lock_on_start_switch_->publish_state(this->nuki_lock_advanced_config_.nightModeImmediateLockOnStart);
        }

        if (this->auto_lock_enabled_switch_ != nullptr) {
            this->auto_lock_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.autoLockEnabled);
        }

        if (this->auto_unlock_disabled_switch_ != nullptr) {
            this->auto_unlock_disabled_switch_->publish_state(this->nuki_lock_advanced_config_.autoUnLockDisabled);
        }

        if (this->immediate_auto_lock_enabled_switch_ != nullptr) {
            this->immediate_auto_lock_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.immediateAutoLockEnabled);
        }

        if (this->auto_update_enabled_switch_ != nullptr) {
            this->auto_update_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.autoUpdateEnabled);
        }

        // Gen 1-4 only
        if (!this->nuki_lock_.isLockUltra() && this->auto_battery_type_detection_enabled_switch_ != nullptr) {
            this->auto_battery_type_detection_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.automaticBatteryTypeDetection);
        }

        // Ultra only
        if (this->nuki_lock_.isLockUltra() && this->slow_speed_during_night_mode_enabled_switch_ != nullptr) {
            this->slow_speed_during_night_mode_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.enableSlowSpeedDuringNightMode);
        }

        if (this->detached_cylinder_enabled_switch_ != nullptr) {
            this->detached_cylinder_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.detachedCylinder);
        }
        #endif

        #ifdef USE_NUMBER
        if (this->lock_n_go_timeout_number_ != nullptr) {
            this->lock_n_go_timeout_number_->publish_state(this->nuki_lock_advanced_config_.lockNgoTimeout);
        }
        if (this->auto_lock_timeout_number_ != nullptr) {
            this->auto_lock_timeout_number_->publish_state(this->nuki_lock_advanced_config_.autoLockTimeOut);
        }
        if (this->unlatch_duration_number_ != nullptr) {
            this->unlatch_duration_number_->publish_state(this->nuki_lock_advanced_config_.unlatchDuration);
        }
        if (this->unlocked_position_offset_number_ != nullptr) {
            this->unlocked_position_offset_number_->publish_state(this->nuki_lock_advanced_config_.unlockedPositionOffsetDegrees);
        }
        if (this->locked_position_offset_number_ != nullptr) {
            this->locked_position_offset_number_->publish_state(this->nuki_lock_advanced_config_.lockedPositionOffsetDegrees);
        }
        if (this->single_locked_position_offset_number_ != nullptr) {
            this->single_locked_position_offset_number_->publish_state(this->nuki_lock_advanced_config_.singleLockedPositionOffsetDegrees);
        }
        if (this->unlocked_to_locked_transition_offset_number_ != nullptr) {
            this->unlocked_to_locked_transition_offset_number_->publish_state(this->nuki_lock_advanced_config_.unlockedToLockedTransitionOffsetDegrees);
        }
        #endif

        #ifdef USE_SELECT
        if (this->single_button_press_action_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::button_press_action_to_string(this->nuki_lock_advanced_config_.singleButtonPressAction, str);
            this->single_button_press_action_select_->publish_state(str);
        }

        if (this->double_button_press_action_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::button_press_action_to_string(this->nuki_lock_advanced_config_.doubleButtonPressAction, str);
            this->double_button_press_action_select_->publish_state(str);
        }

        // Gen 1-4 only
        if (!this->nuki_lock_.isLockUltra() && this->battery_type_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::battery_type_to_string(this->nuki_lock_advanced_config_.batteryType, str);
            this->battery_type_select_->publish_state(str);
        }

        // Ultra
        if (this->nuki_lock_.isLockUltra() && this->motor_speed_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::motor_speed_to_string(this->nuki_lock_advanced_config_.motorSpeed, str);
            this->motor_speed_select_->publish_state(str);
        }
        #endif
    } else {
        ESP_LOGE(TAG, "requestAdvancedConfig has resulted in %s (%d)", str, conf_req_result);
        this->advanced_config_update_ = true;
    }
}

void NukiLockComponent::update_auth_data() {
    this->auth_data_update_ = false;

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    Nuki::CmdResult auth_data_req_result = this->nuki_lock_.retrieveAuthorizationEntries(0, MAX_AUTH_DATA_ENTRIES);
    char auth_data_req_result_as_string[30] = {0};
    NukiLock::cmdResultToString(auth_data_req_result, auth_data_req_result_as_string);

    App.feed_wdt();

    if (auth_data_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "retrieveAuthorizationEntries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);
        this->auth_data_ready_time_ = millis() + 5000;
    } else {
        ESP_LOGE(TAG, "retrieveAuthorizationEntries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);
        this->auth_data_update_ = true;
    }
}

void NukiLockComponent::process_auth_data() {
    ESP_LOGD(TAG, "Process Authorization Entries");

    std::list<NukiLock::AuthorizationEntry> authEntries;
    this->nuki_lock_.getAuthorizationEntries(&authEntries);

    App.feed_wdt();

    if (authEntries.empty()) {
        ESP_LOGW(TAG, "No auth entries!");
        return;
    }

    ESP_LOGD(TAG, "Authorization Entry Count: %d", authEntries.size());

    authEntries.sort([](const NukiLock::AuthorizationEntry& a, const NukiLock::AuthorizationEntry& b) {
        return a.authId < b.authId;
    });

    if (authEntries.size() > MAX_AUTH_DATA_ENTRIES) {
        authEntries.resize(MAX_AUTH_DATA_ENTRIES);
        ESP_LOGW(TAG, "Authorization entry count exceeds maximum, resized to maximum!");
    }

    this->auth_entries_count_ = 0;

    for(const auto& entry : authEntries) {
        if (this->auth_entries_count_ >= MAX_AUTH_DATA_ENTRIES) break;

        AuthEntry& auth_entry = this->auth_entries_[this->auth_entries_count_];
        auth_entry.authId = entry.authId;

        strncpy(auth_entry.name, reinterpret_cast<const char*>(entry.name), MAX_NAME_LEN - 1);
        auth_entry.name[MAX_NAME_LEN - 1] = '\0';

        ESP_LOGD(TAG, "Authorization entry[%d] type: %d name: %s", entry.authId, entry.idType, entry.name);

        this->auth_entries_count_++;
    }
}

void NukiLockComponent::update_event_logs() {
    this->event_log_update_ = false;

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    Nuki::CmdResult event_log_req_result = this->nuki_lock_.retrieveLogEntries(0, MAX_EVENT_LOG_ENTRIES, 1, false);
    char event_log_req_result_as_string[30] = {0};
    NukiLock::cmdResultToString(event_log_req_result, event_log_req_result_as_string);

    App.feed_wdt();

    if (event_log_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "retrieveLogEntries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);
        this->event_log_ready_time_ = millis() + 5000;
    } else {
        ESP_LOGE(TAG, "retrieveLogEntries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);
        this->event_log_update_ = true;
    }
}

void NukiLockComponent::process_log_entries() {
    ESP_LOGD(TAG, "Process Event Log Entries");

    std::list<NukiLock::LogEntry> log_entries;
    this->nuki_lock_.getLogEntries(&log_entries);

    App.feed_wdt();

    if (log_entries.empty()) {
        ESP_LOGW(TAG, "No log entries!");
        return;
    }

    ESP_LOGD(TAG, "Log Entry Count: %d", log_entries.size());

    if (log_entries.size() > MAX_EVENT_LOG_ENTRIES) {
        log_entries.resize(MAX_EVENT_LOG_ENTRIES);
        ESP_LOGW(TAG, "Log entry count exceeds maximum, resized to maximum!");
    }

    log_entries.sort([](const NukiLock::LogEntry& a, const NukiLock::LogEntry& b) {
        return a.index < b.index;
    });

    char buffer[50] = {0};
    char num_buffer[16];
    uint32_t auth_index = 0;

    std::map<std::string, std::string> event_data;

    for (const auto& log : log_entries) {
        event_data.clear();

        if (log.loggingType == NukiLock::LoggingType::LockAction ||
            log.loggingType == NukiLock::LoggingType::KeypadAction) {
            
            int sizeName = sizeof(log.name);
            strncpy(buffer, reinterpret_cast<const char*>(log.name), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            if (strcmp(buffer, "") == 0) {
                strcpy(buffer, "Manual");
            }

            if (log.index > auth_index) {
                auth_index = log.index;
                this->auth_id_ = log.authId;

                strncpy(this->auth_name_, buffer, sizeof(this->auth_name_) - 1);
                this->auth_name_[sizeof(this->auth_name_) - 1] = '\0';

                const char* authNameFromEntries = get_auth_name(this->auth_id_);
                if (authNameFromEntries) {
                    strncpy(this->auth_name_, authNameFromEntries, sizeof(this->auth_name_) - 1);
                    this->auth_name_[sizeof(this->auth_name_) - 1] = '\0';
                }
            }
        }

        if (this->send_events_) {
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.index);
            event_data["index"] = num_buffer;

            snprintf(num_buffer, sizeof(num_buffer), "%u", log.authId);
            event_data["authorizationId"] = num_buffer;

            const char* authName = get_auth_name(log.authId);
            if (!authName) authName = this->auth_name_;
            event_data["authorizationName"] = authName;

            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampYear);
            event_data["timeYear"] = num_buffer;
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampMonth);
            event_data["timeMonth"] = num_buffer;
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampDay);
            event_data["timeDay"] = num_buffer;
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampHour);
            event_data["timeHour"] = num_buffer;
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampMinute);
            event_data["timeMinute"] = num_buffer;
            snprintf(num_buffer, sizeof(num_buffer), "%u", log.timeStampSecond);
            event_data["timeSecond"] = num_buffer;

            NukiLock::loggingTypeToString(log.loggingType, buffer);
            event_data["type"] = buffer;

            switch (log.loggingType) {
                case NukiLock::LoggingType::LockAction:
                    NukiLock::lockactionToString((NukiLock::LockAction)log.data[0], buffer);
                    event_data["action"] = buffer;
                    NukiLock::triggerToString((NukiLock::Trigger)log.data[1], buffer);
                    event_data["trigger"] = buffer;
                    NukiLock::completionStatusToString((NukiLock::CompletionStatus)log.data[3], buffer);
                    event_data["completionStatus"] = buffer;
                    break;

                case NukiLock::LoggingType::KeypadAction:
                {
                    NukiLock::lockactionToString((NukiLock::LockAction)log.data[0], buffer);
                    event_data["action"] = buffer;

                    switch (log.data[1]) {
                        case 0: event_data["trigger"] = "arrowkey"; break;
                        case 1: event_data["trigger"] = "code"; break;
                        case 2: event_data["trigger"] = "fingerprint"; break;
                        default: event_data["trigger"] = "unknown"; break;
                    }

                    if (log.data[2] == 9)
                        event_data["trigger"] = "notAuthorized";
                    else if (log.data[2] == 224)
                        event_data["trigger"] = "invalidCode";
                    else {
                        NukiLock::completionStatusToString((NukiLock::CompletionStatus)log.data[2], buffer);
                        event_data["completionStatus"] = buffer;
                    }

                    unsigned int codeId = 256U * log.data[4] + log.data[3];
                    snprintf(num_buffer, sizeof(num_buffer), "%u", codeId);
                    event_data["codeId"] = num_buffer;
                    break;
                }

                case NukiLock::LoggingType::DoorSensor:
                    switch (log.data[0]) {
                        case 0: event_data["action"] = "DoorOpened"; break;
                        case 1: event_data["action"] = "DoorClosed"; break;
                        case 2: event_data["action"] = "SensorJammed"; break;
                        default: event_data["action"] = "Unknown"; break;
                    }
                    break;
            }

            if (log.index > this->last_rolling_log_id) {
                this->last_rolling_log_id = log.index;
                this->event_log_received_callback_.call(log);

                #ifdef USE_API_HOMEASSISTANT_SERVICES
                ESP_LOGD(TAG, "Send event to Home Assistant on %s", this->event_);
                this->fire_homeassistant_event(this->event_, event_data);
                #endif
            }
        }
    }

    #ifdef USE_TEXT_SENSOR
    if (this->last_unlock_user_text_sensor_ != nullptr) {
        this->last_unlock_user_text_sensor_->publish_state(this->auth_name_);
    }
    #endif
}

const char* NukiLockComponent::get_auth_name(uint32_t authId) const {
    for (size_t i = 0; i < auth_entries_count_; i++) {
        if (auth_entries_[i].authId == authId) {
            return auth_entries_[i].name;
        }
    }
    return nullptr;
}

bool NukiLockComponent::execute_lock_action(NukiLock::LockAction lock_action) {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot execute lock action");
        return false;
    }

    // Publish the assumed transitional lock state
    switch (lock_action) {
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
    Nuki::CmdResult result = this->nuki_lock_.lockAction(lock_action);

    App.feed_wdt();

    char lock_action_as_string[30] = {0};
    NukiLock::lockactionToString(lock_action, lock_action_as_string);

    char result_as_string[30] = {0};
    NukiLock::cmdResultToString(result, result_as_string);

    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "lockAction %s (%d) has resulted in %s (%d)", lock_action_as_string, lock_action, result_as_string, result);
        return true;
    } else {
        ESP_LOGE(TAG, "lockAction %s (%d) has resulted in %s (%d)", lock_action_as_string, lock_action, result_as_string, result);
        return false;
    }
}

void NukiLockComponent::set_security_pin(uint32_t new_pin) {
    ESP_LOGI(TAG, "Setting security pin: %u", new_pin);

    if (new_pin > 999999) {
        ESP_LOGE(TAG, "Invalid pin: %u (max. 6 digits/999999) - abort.", new_pin);
        return;
    }

    // Reset override pin if new pin is 0
    if (new_pin == 0 && this->security_pin_ != 0) {
        ESP_LOGI(TAG, "Clearing security pin override");
    }
    this->security_pin_ = new_pin;

    const uint32_t pin_to_use = this->security_pin_ != 0 ? this->security_pin_ : this->security_pin_config_.value_or(0);

    // Update pin state
    if (pin_to_use == 0) {
        this->pin_state_ = PinState::NotSet;
        ESP_LOGW(TAG, "Security pin is not set! Some functions may be unavailable.");
        this->save_settings();
        this->publish_pin_state();
        return;
    }

    // Save pin
    const bool result = this->nuki_lock_.isLockUltra() ? this->nuki_lock_.saveUltraPincode(pin_to_use) : this->nuki_lock_.saveSecurityPincode(static_cast<uint16_t>(pin_to_use));

    if (!result) {
        ESP_LOGE(TAG, "Failed to save security pin");
        this->pin_state_ = PinState::Invalid;
        this->save_settings();
        this->publish_pin_state();
        return;
    }

    ESP_LOGI(TAG, "Successfully saved security pin");
    this->pin_state_ = PinState::Set;
    this->save_settings();
    this->publish_pin_state();

    // Validate pin if lock is paired and connected
    if (this->nuki_lock_.isPairedWithLock() && this->connected_) {
        ESP_LOGD(TAG, "Validating new security pin");
        this->validate_pin();
    } else {
        ESP_LOGD(TAG, "Skipping pin validation (not paired or not connected)");
    }
}

void NukiLockComponent::validate_pin()
{
    ESP_LOGD(TAG, "Check if pin is valid and save state");

    if(this->pin_state_ == PinState::NotSet) {
        ESP_LOGD(TAG, "Pin is not set, no validation needed!");
        return;
    }

    this->pin_validation_pending_ = true;
}

void NukiLockComponent::nuki_task_fn(void *arg)
{
    esp_task_wdt_add(NULL);

    auto *lock_component = static_cast<NukiLockComponent *>(arg);
    lock_component->nuki_task_loop();
}

void NukiLockComponent::nuki_task_loop()
{
    // Increase Watchdog Timeout
    // Fixes Pairing Crash
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 15000,
        .trigger_panic = false
    }; 
    esp_task_wdt_reconfigure(&wdt_config);

    uint32_t last_loop_time = millis();

    while(true)
    {
        // Only execute main logic every 500ms
        uint32_t current_time = millis();
        if (current_time - last_loop_time < 500) {
            delay(10);  // Small delay to prevent tight loop
            continue;
        }
        last_loop_time = current_time;

        // Check for new advertisements
        this->scanner_.update();
        App.feed_wdt();
        delay(20);

        // Terminate stale Bluetooth connections
        this->nuki_lock_.updateConnectionState();

        App.feed_wdt();

        if (millis() - last_command_executed_time_ < command_cooldown_millis) {
            // Give the lock time to terminate the previous command
            uint64_t millisSinceLastExecution = millis() - last_command_executed_time_;
            uint64_t millisLeft = (millisSinceLastExecution < command_cooldown_millis) ? command_cooldown_millis - millisSinceLastExecution : 1;
            ESP_LOGV(TAG, "Cooldown period, %dms left", millisLeft);
            continue;
        }

        if (this->nuki_lock_.isPairedWithLock()) {
            // Validate PIN in task without blocking
            if (this->pin_validation_pending_) {
                if (this->pin_validation_attempts_ == 0) {
                    // First attempt, start timer
                    this->pin_validation_start_time_ = millis();
                    this->pin_validation_attempts_ = 4;
                }

                // Check if enough time has passed since last attempt (100ms)
                if (millis() - this->pin_validation_start_time_ >= 100) {
                    this->pin_validation_start_time_ = millis();
                    this->pin_validation_attempts_--;
                    
                    ESP_LOGD(TAG, "verifySecurityPin attempts left: %d", this->pin_validation_attempts_);
                    
                    App.feed_wdt();
                    Nuki::CmdResult pin_result = this->nuki_lock_.verifySecurityPin();
                    App.feed_wdt();
                    
                    if(pin_result == Nuki::CmdResult::Success) {
                        ESP_LOGI(TAG, "Nuki Lock PIN is valid");
                        this->pin_state_ = PinState::Valid;
                        this->pin_validation_pending_ = false;
                        this->save_settings();
                        this->publish_pin_state();
                    } else if (this->pin_validation_attempts_ == 0) {
                        ESP_LOGD(TAG, "Nuki Lock PIN is invalid or not set");
                        this->pin_state_ = PinState::Invalid;
                        this->pin_validation_pending_ = false;
                        this->save_settings();
                        this->publish_pin_state();
                    } else {
                        ESP_LOGW(TAG, "verifySecurityPin: result %d, retry...", pin_result);
                    }
                }
            }

            // Execute (all) actions first, then status updates, then config updates.
            // Only one command (action, status, config, or auth data) is executed per update() call.
            if (this->action_attempts_ > 0) {
                this->action_attempts_--;

                NukiLock::LockAction currentLockAction = this->lock_action_;
                char currentlock_action_as_string[30] = {0};
                NukiLock::lockactionToString(currentLockAction, currentlock_action_as_string);

                ESP_LOGD(TAG, "Executing lock action %s (%d)... (%d attempts left)", currentlock_action_as_string, currentLockAction, this->action_attempts_);

                bool isExecutionSuccessful = this->execute_lock_action(currentLockAction);

                App.feed_wdt();

                if (isExecutionSuccessful) {
                    if (this->lock_action_ == currentLockAction) {
                        // Stop action attempts only if no new action was received in the meantime.
                        // Otherwise, the new action won't be executed.
                        this->action_attempts_ = 0;
                    }
                } else if (this->action_attempts_ == 0) {
                    this->connected_ = false;
                    
                    // Publish failed state only when no attempts are left
                    this->publish_state(lock::LOCK_STATE_NONE);

                    #ifdef USE_BINARY_SENSOR
                    if (this->connected_binary_sensor_ != nullptr)
                    {
                        this->connected_binary_sensor_->publish_state(this->connected_);
                    }  
                    #endif
                }

                // Schedule a status update without waiting for the next advertisement for a faster feedback
                this->status_update_ = true;

                // Give the lock extra time when successful in order to account for time to turn the key
                command_cooldown_millis = isExecutionSuccessful ? COOLDOWN_COMMANDS_EXTENDED_MILLIS : COOLDOWN_COMMANDS_MILLIS;

            } else if (this->status_update_) {
                ESP_LOGD(TAG, "Requesting status...");
                this->update_status();
                command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            } else if (this->config_update_) {
                ESP_LOGD(TAG, "Requesting config...");
                this->update_config();
                command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            } else if (this->auth_data_update_) {
                ESP_LOGD(TAG, "Requesting auth data...");
                this->update_auth_data();
                command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            } else if (this->event_log_update_) {
                ESP_LOGD(TAG, "Requesting event logs...");
                this->update_event_logs();
                command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            } else if (this->advanced_config_update_) {
                ESP_LOGD(TAG, "Requesting advanced config...");
                this->update_advanced_config();
                command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
            }

            // Process retrieved data
            if (this->auth_data_ready_time_ > 0 && millis() >= this->auth_data_ready_time_) {
                this->process_auth_data();
                this->auth_data_ready_time_ = 0;
            } else if (this->event_log_ready_time_ > 0 && millis() >= this->event_log_ready_time_) {
                this->process_log_entries();
                this->event_log_ready_time_ = 0;
            }

            last_command_executed_time_ = millis();

        } else {
            this->connected_ = false;

            #ifdef USE_BINARY_SENSOR
            if (this->paired_binary_sensor_ != nullptr) {
                this->paired_binary_sensor_->publish_state(false);
            }
            if (this->connected_binary_sensor_ != nullptr) {
                this->connected_binary_sensor_->publish_state(connected_);
            }
            #endif

            // Pairing Mode is active
            if (this->pairing_mode_) {
                // Pair Nuki
                Nuki::AuthorizationIdType type = this->pairing_as_app_.value_or(false) ? Nuki::AuthorizationIdType::App : Nuki::AuthorizationIdType::Bridge;
                
                App.feed_wdt();

                bool paired = this->nuki_lock_.pairNuki(type) == Nuki::PairingResult::Success;

                App.feed_wdt();

                if (paired) {
                    this->setup_lock(true);

                    this->paired_callback_.call();
                    this->set_pairing_mode(false);
                }

                #ifdef USE_BINARY_SENSOR
                if (this->paired_binary_sensor_ != nullptr) {
                    this->paired_binary_sensor_->publish_state(paired);
                }
                #endif
            }
        }
    }
}

void NukiLockComponent::setup() {
    ESP_LOGCONFIG(TAG, "Running setup");

    // Restore settings from flash
    this->pref_ = global_preferences->make_preference<NukiLockSettings>(global_nuki_lock_id);

    NukiLockSettings recovered;
    if (!this->pref_.load(&recovered)) {
        recovered = {0};
    }

    this->pin_state_ = recovered.pin_state;
    this->security_pin_ = recovered.security_pin;

    this->traits.set_supports_open(true);

    this->traits.set_supported_states({
        lock::LOCK_STATE_NONE,
        lock::LOCK_STATE_LOCKED,
        lock::LOCK_STATE_UNLOCKED,
        lock::LOCK_STATE_JAMMED,
        lock::LOCK_STATE_LOCKING,
        lock::LOCK_STATE_UNLOCKING
    });

    this->scanner_.initialize("ESPHomeNuki", true, 40, 40);
    this->scanner_.setScanDuration(0);

    App.feed_wdt();

    ESP_LOGD(TAG, "Prepare security pin for initialization");

    if (this->security_pin_ > 999999) {
        ESP_LOGE(TAG, "Invalid security pin (override) detected! The pin can't be longer than 6 digits. Unset override pin.");
        this->security_pin_ = 0;
        this->pin_state_ = PinState::Invalid;
        this->save_settings();
    }

    uint32_t pin_to_use = 0;
    if (this->security_pin_ != 0) {
        ESP_LOGW(TAG, "Note: Using security pin override, not yaml config pin!");
        ESP_LOGD(TAG, "Security pin: %u (override)", this->security_pin_);
        pin_to_use = this->security_pin_;
    } else if(this->security_pin_config_.value_or(0) != 0) {
        ESP_LOGD(TAG, "Security pin: %u (yaml config)", this->security_pin_config_.value_or(0));
        pin_to_use = this->security_pin_config_.value_or(0);
    } else {
        this->pin_state_ = PinState::NotSet;
        ESP_LOGW(TAG, "The security pin is not set - it is crucial to pair a 5th Gen Smart Lock (Ultra / Go / Pro).");
    }

    if (pin_to_use != 0) {
        if (pin_to_use > 999999) {
            ESP_LOGE(TAG, "Invalid security pin detected! The pin can't be longer than 6 digits.");
            this->pin_state_ = PinState::Invalid;
            this->save_settings();
        } else {
            ESP_LOGD(TAG, "Set security pin before init: %u", pin_to_use);
            this->nuki_lock_.saveUltraPincode((unsigned int)pin_to_use, false);
        }
    }

    this->nuki_lock_.setDebugConnect(true);
    this->nuki_lock_.setDebugCommunication(false);
    this->nuki_lock_.setDebugReadableData(false);
    this->nuki_lock_.setDebugHexData(false);
    this->nuki_lock_.setDebugCommand(false);

    this->nuki_lock_.setEventHandler(this);
    this->nuki_lock_.initialize();
    this->nuki_lock_.registerBleScanner(&this->scanner_);
    this->nuki_lock_.setConnectTimeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nuki_lock_.setConnectRetries(BLE_CONNECT_RETRIES);
    this->nuki_lock_.setDisconnectTimeout(BLE_DISCONNECT_TIMEOUT);
    this->nuki_lock_.setGeneralTimeout(this->ble_general_timeout_ * 1000);
    this->nuki_lock_.setCommandTimeout(this->ble_command_timeout_ * 1000);
    
    App.feed_wdt();

    this->publish_pin_state();
    this->publish_state(lock::LOCK_STATE_NONE);

    #ifdef USE_API
        #ifdef USE_API_CUSTOM_SERVICES
        this->register_service(&NukiLockComponent::lock_n_go, "lock_n_go");
        this->register_service(&NukiLockComponent::print_keypad_entries, "print_keypad_entries");
        this->register_service(&NukiLockComponent::add_keypad_entry, "add_keypad_entry", {"name", "code"});
        this->register_service(&NukiLockComponent::update_keypad_entry, "update_keypad_entry", {"id", "name", "code", "enabled"});
        this->register_service(&NukiLockComponent::delete_keypad_entry, "delete_keypad_entry", {"id"});
        #else
        ESP_LOGW(TAG, "CUSTOM API SERVICES ARE DISABLED");
        ESP_LOGW(TAG, "Please set 'api:' -> 'custom_services: true' to use API services.");
        ESP_LOGW(TAG, "More information here: https://esphome.io/components/api.html");
        #endif

        #ifndef USE_API_HOMEASSISTANT_SERVICES
        ESP_LOGW(TAG, "NUKI EVENT LOGS ARE DISABLED");
        ESP_LOGW(TAG, "Please set 'api:' -> 'homeassistant_services: true' to fire Home Assistant events.");
        ESP_LOGW(TAG, "More information here: https://esphome.io/components/api.html");
        #endif
    #endif

    // Initial connection
    if (this->nuki_lock_.isPairedWithLock()) {
        this->setup_lock();

        #ifdef USE_BINARY_SENSOR
        if (this->paired_binary_sensor_ != nullptr)
        {
            this->paired_binary_sensor_->publish_state(true);
        }
        #endif
    } else {
        ESP_LOGI(TAG, "This component is not paired yet. Enable the pairing mode to pair with your smart lock.");

        #ifdef USE_BINARY_SENSOR
        if (this->paired_binary_sensor_ != nullptr)
        {
            this->paired_binary_sensor_->publish_state(false);
        }
        #endif
    }

    xTaskCreatePinnedToCore(nuki_task_fn, "nuki_task", 4096, (void *) this, 2, &this->nuki_task_handle_, 0);
    if (this->nuki_task_handle_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create Nuki task");
        this->mark_failed();
        return;
    }
    ESP_LOGD(TAG, "Nuki Task created");
}



void NukiLockComponent::setup_lock(bool new_pairing) {
    const char* pairing_type = this->pairing_as_app_.value_or(false) ? "App" : "Bridge";
    const char* lock_type = this->nuki_lock_.isLockUltra() ? "5th Gen (Ultra / Go / Pro)" : "1st - 4th Gen";
    ESP_LOGI(TAG, "This component is paired as %s with a %s smart lock!", pairing_type, lock_type);

    const uint32_t pin_to_use = this->security_pin_ != 0 ? this->security_pin_ : this->security_pin_config_.value_or(0);

    if(new_pairing) {
        if (this->security_pin_ != 0) {
            ESP_LOGW(TAG, "Using security pin override instead of YAML config");
        }

        if (pin_to_use == 0) {
            ESP_LOGD(TAG, "No security pin configured, skipping pin setup");
            this->pin_state_ = PinState::NotSet;
        } else if (pin_to_use > 999999) {
            ESP_LOGE(TAG, "Invalid security pin detected! Maximum is 6 digits (999999)");
            this->pin_state_ = PinState::Invalid;
        } else if (!this->nuki_lock_.isLockUltra() && pin_to_use > 65535) {
            ESP_LOGE(TAG, "Security pin exceeds maximum of 65535 for 1st to 4th Gen Smart Locks", pin_to_use);
            this->pin_state_ = PinState::Invalid;
        } else {
            const bool result = this->nuki_lock_.isLockUltra() ? this->nuki_lock_.saveUltraPincode(pin_to_use) : this->nuki_lock_.saveSecurityPincode(static_cast<uint16_t>(pin_to_use));

            if (result) {
                ESP_LOGI(TAG, "Successfully set security pin");
                this->pin_state_ = PinState::Set;
            } else {
                ESP_LOGE(TAG, "Failed to set security pin");
                this->pin_state_ = PinState::Invalid;
            }
        }

        this->save_settings();
        this->publish_pin_state();
    }

    if(pin_to_use != 0) {
        this->validate_pin();
    }

    // Initialize Lock: Request config and auth data
    this->status_update_ = true;
    this->config_update_ = true;
    this->advanced_config_update_ = true;
    if (this->send_events_) {
        this->auth_data_update_ = true;
        this->event_log_update_ = true;
    }

    this->setup_intervals();
}

void NukiLockComponent::publish_pin_state() {
    #ifdef USE_TEXT_SENSOR
    char pin_state_as_string[20] = {0};
    nuki_lock::pin_state_to_string(this->pin_state_, pin_state_as_string);

    if (this->pin_state_text_sensor_ != nullptr && this->pin_state_text_sensor_->state != pin_state_as_string) {
        this->pin_state_text_sensor_->publish_state(pin_state_as_string);
    }
    #endif
}

void NukiLockComponent::setup_intervals(bool setup) {
    this->cancel_interval("update_config");
    this->cancel_interval("update_auth_data");

    if(setup) {
        this->set_interval("update_config", this->query_interval_config_ * 1000, [this]() {
            this->config_update_ = true;
            this->advanced_config_update_ = true;
        });
    
        this->set_interval("update_auth_data", this->query_interval_auth_data_ * 1000, [this]() {
            this->auth_data_update_ = true;
        });
    }
}

/**
 * @brief Add a new lock action that will be executed on the next update() call.
 */
void NukiLockComponent::control(const lock::LockCall &call) {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot execute lock action");
        return;
    }

    lock::LockState state = *call.get_state();

    switch(state) {
        case lock::LOCK_STATE_LOCKED:
            this->action_attempts_ = MAX_ACTION_ATTEMPTS;
            this->lock_action_ = NukiLock::LockAction::Lock;
            break;

        case lock::LOCK_STATE_UNLOCKED: {
            this->action_attempts_ = MAX_ACTION_ATTEMPTS;
            this->lock_action_ = NukiLock::LockAction::Unlock;

            if (this->open_latch_) {
                this->lock_action_ = NukiLock::LockAction::Unlatch;
            }

            if (this->lock_n_go_) {
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

    char lock_action_as_string[30] = {0};
    NukiLock::lockactionToString(this->lock_action_, lock_action_as_string);
    lock_action_as_string[sizeof(lock_action_as_string) - 1] = '\0';
    ESP_LOGI(TAG, "New lock action received: %s (%d)", lock_action_as_string, this->lock_action_);
}

void NukiLockComponent::lock_n_go() {
    this->lock_n_go_ = true;
    this->unlock();
}

bool NukiLockComponent::valid_keypad_id(int32_t id) {
    bool is_valid = std::find(keypad_code_ids_.begin(), keypad_code_ids_.end(), id) != keypad_code_ids_.end();
    if (!is_valid) {
        ESP_LOGE(TAG, "Keypad id %d unknown.", id);
    }
    return is_valid;
}

bool NukiLockComponent::valid_keypad_name(std::string name) {
    bool name_valid = !(name == "" || name == "--");
    if (!name_valid) {
        ESP_LOGE(TAG, "Keypad name '%s' is invalid.", name.c_str());
    }
    return name_valid;
}

bool NukiLockComponent::valid_keypad_code(int32_t code) {
    bool code_valid = (code > 100000 && code < 1000000 && (std::to_string(code).find('0') == std::string::npos));
    if (!code_valid) {
        ESP_LOGE(TAG, "Keypad code %d is invalid. Code must be 6 digits, without 0.", code);
    }
    return code_valid;
}

void NukiLockComponent::add_keypad_entry(std::string name, int32_t code) {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot add keypad entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    if (!(valid_keypad_name(name) && valid_keypad_code(code))) {
        ESP_LOGE(TAG, "add_keypad_entry invalid parameters");
        return;
    }

    NukiLock::NewKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    size_t name_len = name.length();
    memcpy(&entry.name, name.c_str(), name_len > 20 ? 20 : name_len);
    entry.code = code;

    Nuki::CmdResult result = this->nuki_lock_.addKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "add_keypad_entry is sucessful");
    } else {
        ESP_LOGE(TAG, "add_keypad_entry: addKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::update_keypad_entry(int32_t id, std::string name, int32_t code, bool enabled) {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot update keypad entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    if (!(valid_keypad_id(id) && valid_keypad_name(name) && valid_keypad_code(code))) {
        ESP_LOGE(TAG, "update_keypad_entry invalid parameters");
        return;
    }

    NukiLock::UpdatedKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.codeId = id;
    size_t name_len = name.length();
    memcpy(&entry.name, name.c_str(), name_len > 20 ? 20 : name_len);
    entry.code = code;
    entry.enabled = enabled ? 1 : 0;

    Nuki::CmdResult result = this->nuki_lock_.updateKeypadEntry(entry);
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "update_keypad_entry is sucessful");
    } else {
        ESP_LOGE(TAG, "update_keypad_entry: updateKeypadEntry failed (result %d)", result);
    }
}

void NukiLockComponent::delete_keypad_entry(int32_t id) {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot retrieve delete entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
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
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot retrieve keypad entries");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    Nuki::CmdResult result = this->nuki_lock_.retrieveKeypadEntries(0, 0xffff);
    if (result == Nuki::CmdResult::Success) {
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
    ESP_LOGCONFIG(TAG, "nuki_lock:");

    #ifdef USE_API_HOMEASSISTANT_SERVICES
    if (strcmp(this->event_, "esphome.none") != 0) {
        ESP_LOGCONFIG(TAG, "  Event: %s", this->event_);
    } else {
        ESP_LOGCONFIG(TAG, "  Event: Disabled (event name set to none)");
    }
    #else
    ESP_LOGCONFIG(TAG, "  Event: Disabled (Home Assistant services not enabled)");
    #endif

    ESP_LOGCONFIG(TAG, "  Pairing Identity: %s", this->pairing_as_app_.value_or(false) ? "App" : "Bridge");
    ESP_LOGCONFIG(TAG, "  Is Paired: %s", YESNO(this->is_paired()));

    ESP_LOGCONFIG(TAG, "  Pairing mode timeout: %us", this->pairing_mode_timeout_);
    ESP_LOGCONFIG(TAG, "  Configuration query interval: %us", this->query_interval_config_);
    ESP_LOGCONFIG(TAG, "  Auth Data query interval: %us", this->query_interval_auth_data_);
    ESP_LOGCONFIG(TAG, "  BLE general timeout: %us", this->ble_general_timeout_);
    ESP_LOGCONFIG(TAG, "  BLE command timeout: %us", this->ble_command_timeout_);

    char pin_state_as_string[30] = {0};
    nuki_lock::pin_state_to_string(this->pin_state_, pin_state_as_string);
    ESP_LOGCONFIG(TAG, "  Last known security pin state: %s", pin_state_as_string);

    ESP_LOGCONFIG(TAG, "  Task Stack High Watermark: %i", (uint16_t)uxTaskGetStackHighWaterMark(nuki_task_handle_));

    LOG_LOCK(TAG, "Nuki Lock", this);
    #ifdef USE_BINARY_SENSOR
    LOG_BINARY_SENSOR(TAG, "Connected", this->connected_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Paired", this->paired_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_binary_sensor_);
    #endif
    #ifdef USE_TEXT_SENSOR
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Unlock User", this->last_unlock_user_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Lock Action", this->last_lock_action_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Lock Action Trigger", this->last_lock_action_trigger_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Pin Status", this->pin_state_text_sensor_);
    #endif
    #ifdef USE_SENSOR
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_sensor_);
    LOG_SENSOR(TAG, "Bluetooth Signal", this->bt_signal_sensor_);
    #endif
    #ifdef USE_BUTTON
    LOG_BUTTON(TAG, "Unpair", this->unpair_button_);
    LOG_BUTTON(TAG, "Request Calibration", this->request_calibration_button_);
    #endif
    #ifdef USE_SWITCH
    LOG_SWITCH(TAG, "Pairing Mode", this->pairing_mode_switch_);
    LOG_SWITCH(TAG, "Pairing Enabled", this->pairing_enabled_switch_);
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
    LOG_SWITCH(TAG, "Single Lock Enabled", this->single_lock_enabled_switch_);
    LOG_SWITCH(TAG, "DST Mode Enabled", this->dst_mode_enabled_switch_);
    LOG_SWITCH(TAG, "Slow Speed During Night Mode Enabled", this->slow_speed_during_night_mode_enabled_switch_);
    LOG_SWITCH(TAG, "Detached Cylinder Enabled", this->detached_cylinder_enabled_switch_);
    #endif
    #ifdef USE_NUMBER
    LOG_NUMBER(TAG, "LED Brightness", this->led_brightness_number_);
    LOG_NUMBER(TAG, "Timezone Offset", this->timezone_offset_number_);
    LOG_NUMBER(TAG, "LockNGo Timeout", this->lock_n_go_timeout_number_);
    LOG_NUMBER(TAG, "Auto Lock Timeout", this->auto_lock_timeout_number_);
    LOG_NUMBER(TAG, "Unlatch Duration", this->unlatch_duration_number_);
    LOG_NUMBER(TAG, "Unlocked Position Offset Degrees", this->unlocked_position_offset_number_);
    LOG_NUMBER(TAG, "Locked Position Offset Degrees", this->locked_position_offset_number_);
    LOG_NUMBER(TAG, "Single Locked Position Offset Degrees", this->single_locked_position_offset_number_);
    LOG_NUMBER(TAG, "Unlocked To Locked Transition Offset Degrees", this->unlocked_to_locked_transition_offset_number_);
    #endif
    #ifdef USE_SELECT
    LOG_SELECT(TAG, "Single Button Press Action", this->single_button_press_action_select_);
    LOG_SELECT(TAG, "Double Button Press Action", this->double_button_press_action_select_);
    LOG_SELECT(TAG, "Fob Action 1", this->fob_action_1_select_);
    LOG_SELECT(TAG, "Fob Action 2", this->fob_action_2_select_);
    LOG_SELECT(TAG, "Fob Action 3", this->fob_action_3_select_);
    LOG_SELECT(TAG, "Timezone", this->timezone_select_);
    LOG_SELECT(TAG, "Advertising Mode", this->advertising_mode_select_);
    LOG_SELECT(TAG, "Battery Type", this->battery_type_select_);
    LOG_SELECT(TAG, "Motor Speed", this->motor_speed_select_);
    #endif
}

void NukiLockComponent::notify(Nuki::EventType event_type) {
    ESP_LOGI(TAG, "Event notified %d", event_type);

    if(event_type == Nuki::EventType::KeyTurnerStatusReset) {
        // IDK
        ESP_LOGD(TAG, "KeyTurnerStatusReset");
    } else if (event_type == Nuki::EventType::ERROR_BAD_PIN) {
        // Invalid Pin
        ESP_LOGW(TAG, "Nuki reported an invalid security PIN");

        ESP_LOGD(TAG, "NVS PIN 1st-4th Gen: %d", this->nuki_lock_.getSecurityPincode());
        ESP_LOGD(TAG, "NVS PIN 5th Gen (Ultra / Go / Pro): %d", this->nuki_lock_.getUltraPincode());
        ESP_LOGD(TAG, "ESPHome PIN (override): %d", this->security_pin_);
        ESP_LOGD(TAG, "ESPHome PIN (YAML): %d", this->security_pin_config_.value_or(0));

        const uint32_t saved_pin = this->nuki_lock_.isLockUltra() ? this->nuki_lock_.getUltraPincode() : this->nuki_lock_.getSecurityPincode();
        const uint32_t actual_pin = this->security_pin_ != 0 ? this->security_pin_ : this->security_pin_config_.value_or(0);

        if(saved_pin != actual_pin) {
            ESP_LOGW(TAG, "The PIN stored in NVS does not match your configured PIN. Please remove leading zeros if any.");
        }

        this->pin_state_ = PinState::Invalid;
        this->save_settings();
        this->publish_pin_state();
    } else if(event_type == Nuki::EventType::KeyTurnerStatusUpdated) {
        ESP_LOGD(TAG, "KeyTurnerStatusUpdated");

        // Request status update (incl. event log request)
        this->status_update_ = true;
    } else if(event_type == Nuki::EventType::BLE_ERROR_ON_DISCONNECT) {
        ESP_LOGE(TAG, "Failed to disconnect from Nuki. Restarting ESPHome...");
        delay(100);  // NOLINT
        App.safe_reboot();
    }
}

void NukiLockComponent::unpair() {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot unpair");
        return;
    }

    this->nuki_lock_.unPairNuki();

    this->connected_ = false;

    this->publish_state(lock::LOCK_STATE_NONE);

    // Reset pin (override)
    this->security_pin_ = 0;
    if(this->security_pin_ == 0 && this->security_pin_config_.value_or(0) == 0) {
        this->pin_state_ = PinState::NotSet;
        ESP_LOGD(TAG, "The security pin is now unset!");
    } else {
        this->pin_state_ = PinState::Set;
    }
    this->publish_pin_state();
    this->save_settings();

    this->setup_intervals(false);
    this->pin_validation_pending_ = false;

    ESP_LOGI(TAG, "Unpaired Nuki! Turn on Pairing Mode to pair a new Nuki.");
}

void NukiLockComponent::request_calibration() {
    if (!this->nuki_lock_.isPairedWithLock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot request calibration");
        return;
    }

    Nuki::CmdResult result = this->nuki_lock_.requestCalibration();
    if (result == Nuki::CmdResult::Success) {
        ESP_LOGI(TAG, "Calibration requested successfully");
    } else {
        ESP_LOGE(TAG, "Failed to request calibration (result %d)", result);
    }
}

void NukiLockComponent::set_pairing_mode(bool enabled) {
    this->pairing_mode_ = enabled;

    #ifdef USE_SWITCH
    if (this->pairing_mode_switch_ != nullptr) {
        this->pairing_mode_switch_->publish_state(enabled);
    }
    #endif

    cancel_timeout("pairing_mode_timeout");

    if (enabled) {
        ESP_LOGI(TAG, "Pairing Mode turned on for %d seconds", this->pairing_mode_timeout_);
        this->pairing_mode_on_callback_.call();

        if (this->security_pin_ != 0) {
            ESP_LOGW(TAG, "Note: Using security pin override to pair, not yaml config pin!");
        } else if(this->security_pin_config_.value_or(0) != 0) {
            ESP_LOGD(TAG, "Using security pin from yaml config to pair.");
        } else {
            ESP_LOGW(TAG, "Note: The security pin is crucial to pair a 5th Gen Smart Lock (Ultra / Go / Pro) but is currently not set.");
        }

        ESP_LOGD(TAG, "NVS PIN 1st-4th Gen: %d", this->nuki_lock_.getSecurityPincode());
        ESP_LOGD(TAG, "NVS PIN 5th Gen (Ultra / Go / Pro): %d", this->nuki_lock_.getUltraPincode());
        ESP_LOGD(TAG, "ESPHome PIN (override): %d", this->security_pin_);
        ESP_LOGD(TAG, "ESPHome PIN (YAML): %d", this->security_pin_config_.value_or(0));

        ESP_LOGI(TAG, "Waiting for Nuki to enter pairing mode...");

        this->set_timeout("pairing_mode_timeout", this->pairing_mode_timeout_ * 1000, [this]()
        {
            ESP_LOGV(TAG, "Pairing timed out, turning off pairing mode");
            this->set_pairing_mode(false);
        });
    } else {
        ESP_LOGI(TAG, "Pairing Mode turned off");
        this->pairing_mode_off_callback_.call();
    }
}

#ifdef USE_BUTTON
void NukiLockUnpairButton::press_action() {
    this->parent_->unpair();
}

void NukiLockRequestCalibrationButton::press_action() {
    this->parent_->request_calibration();
}
#endif
#ifdef USE_SELECT
void NukiLockSingleButtonPressActionSelect::control(const std::string &value) {
    NukiLock::ButtonPressAction action = nuki_lock::button_press_action_to_enum(value.c_str());
    if(this->parent_->get_nuki_lock()->setSingleButtonPressAction(action)) {
        this->parent_->get_nuki_lock_advanced_config()->singleButtonPressAction = action;
        this->publish_state(value.c_str());
    }
}

void NukiLockDoubleButtonPressActionSelect::control(const std::string &value) {
    NukiLock::ButtonPressAction action = nuki_lock::button_press_action_to_enum(value.c_str());
    if(this->parent_->get_nuki_lock()->setDoubleButtonPressAction(action)) {
        this->parent_->get_nuki_lock_advanced_config()->doubleButtonPressAction = action;
        this->publish_state(value.c_str());
    }
}

void NukiLockFobAction1Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action != 99 && this->parent_->get_nuki_lock()->setFobAction(1, action)) {
        this->parent_->get_nuki_lock_config()->fobAction1 = action;
        this->publish_state(value.c_str());
    }
}

void NukiLockFobAction2Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action != 99 && this->parent_->get_nuki_lock()->setFobAction(2, action)) {
        this->parent_->get_nuki_lock_config()->fobAction2 = action;
        this->publish_state(value.c_str());
    }
}

void NukiLockFobAction3Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action != 99 && this->parent_->get_nuki_lock()->setFobAction(3, action)) {
        this->parent_->get_nuki_lock_config()->fobAction3 = action;
        this->publish_state(value.c_str());
    }
}

void NukiLockTimeZoneSelect::control(const std::string &value) {
    Nuki::TimeZoneId tzid = nuki_lock::timezone_to_enum(value.c_str());
    if(this->parent_->get_nuki_lock()->setTimeZoneId(tzid)) {
        this->parent_->get_nuki_lock_config()->timeZoneId = tzid;
        this->publish_state(value.c_str());
    }
}

void NukiLockAdvertisingModeSelect::control(const std::string &value) {
    Nuki::AdvertisingMode mode = nuki_lock::advertising_mode_to_enum(value.c_str());
    if(this->parent_->get_nuki_lock()->setAdvertisingMode(mode)) {
        this->parent_->get_nuki_lock_config()->advertisingMode = mode;
        this->publish_state(value.c_str());
    }
}

void NukiLockBatteryTypeSelect::control(const std::string &value) {
    if(!this->parent_->get_nuki_lock()->isLockUltra()) {
        Nuki::BatteryType type = nuki_lock::battery_type_to_enum(value.c_str());
        if(this->parent_->get_nuki_lock()->setBatteryType(type)) {
            this->parent_->get_nuki_lock_advanced_config()->batteryType = type;
            this->publish_state(value.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Battery Type is not supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
    }
}

void NukiLockMotorSpeedSelect::control(const std::string &value) {
    if(this->parent_->get_nuki_lock()->isLockUltra()) {
        NukiLock::MotorSpeed speed = nuki_lock::motor_speed_to_enum(value.c_str());
        if(this->parent_->get_nuki_lock()->setMotorSpeed(speed)) {
            this->parent_->get_nuki_lock_advanced_config()->motorSpeed = speed;
            this->publish_state(value.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Motor Speed is only supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
    }
}
#endif
#ifdef USE_SWITCH
void NukiLockPairingModeSwitch::write_state(bool state) {
    this->parent_->set_pairing_mode(state);
}

void NukiLockPairingEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enablePairing(state)) {
        this->parent_->get_nuki_lock_config()->pairingEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockAutoUnlatchEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableAutoUnlatch(state)) {
        this->parent_->get_nuki_lock_config()->autoUnlatch = state;
        this->publish_state(state);
    }
}

void NukiLockButtonEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableButton(state)) {
        this->parent_->get_nuki_lock_config()->buttonEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockLedEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableLedFlash(state)) {
        this->parent_->get_nuki_lock_config()->ledEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockNightModeEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableNightMode(state)) {
        this->parent_->get_nuki_lock_advanced_config()->nightModeEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockNightModeAutoLockEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableNightModeAutoLock(state)) {
        this->parent_->get_nuki_lock_advanced_config()->nightModeAutoLockEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockNightModeAutoUnlockDisabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->disableNightModeAutoUnlock(state)) {
        this->parent_->get_nuki_lock_advanced_config()->nightModeAutoUnlockDisabled = state;
        this->publish_state(state);
    }
}

void NukiLockNightModeImmediateLockOnStartEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableNightModeImmediateLockOnStart(state)) {
        this->parent_->get_nuki_lock_advanced_config()->nightModeImmediateLockOnStart = state;
        this->publish_state(state);
    }
}

void NukiLockAutoLockEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableAutoLock(state)) {
        this->parent_->get_nuki_lock_advanced_config()->autoLockEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockAutoUnlockDisabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->disableAutoUnlock(state)) {
        this->parent_->get_nuki_lock_advanced_config()->autoUnLockDisabled = state;
        this->publish_state(state);
    }
}

void NukiLockImmediateAutoLockEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableImmediateAutoLock(state)) {
        this->parent_->get_nuki_lock_advanced_config()->immediateAutoLockEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockAutoUpdateEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableAutoUpdate(state)) {
        this->parent_->get_nuki_lock_advanced_config()->autoUpdateEnabled = state;
        this->publish_state(state);
    }
}

void NukiLockSingleLockEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableSingleLock(state)) {
        this->parent_->get_nuki_lock_config()->singleLock = state;
        this->publish_state(state);
    }
}

void NukiLockDstModeEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableDst(state)) {
        this->parent_->get_nuki_lock_config()->dstMode = state;
        this->publish_state(state);
    }
}

void NukiLockAutoBatteryTypeDetectionEnabledSwitch::write_state(bool state) {
    if(!this->parent_->get_nuki_lock()->isLockUltra()) {
        if(this->parent_->get_nuki_lock()->enableAutoBatteryTypeDetection(state)) {
            this->parent_->get_nuki_lock_advanced_config()->automaticBatteryTypeDetection = state;
            this->publish_state(state);
        }
    } else {
        ESP_LOGE(TAG, "Auto Battery Type Detection is not supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
    }
}

void NukiLockSlowSpeedDuringNightModeEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->isLockUltra()) {
        if(this->parent_->get_nuki_lock()->enableSlowSpeedDuringNightMode(state)) {
            this->parent_->get_nuki_lock_advanced_config()->enableSlowSpeedDuringNightMode = state;
            this->publish_state(state);
        }
    } else {
        ESP_LOGE(TAG, "Slow Speed During Night Mode is only supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
    }
}
void NukiLockDetachedCylinderEnabledSwitch::write_state(bool state) {
    if(this->parent_->get_nuki_lock()->enableDetachedCylinder(state)) {
        this->parent_->get_nuki_lock_advanced_config()->detachedCylinder = state;
        this->publish_state(state);
    }
}
#endif
#ifdef USE_NUMBER
void NukiLockLedBrightnessNumber::control(float value) {
    if(this->parent_->get_nuki_lock()->setLedBrightness(value)) {
        this->parent_->get_nuki_lock_config()->ledBrightness = value;
        this->publish_state(value);
    }
}
void NukiLockTimeZoneOffsetNumber::control(float value) {
    if (value >= -60 && value <= 60) {
        if(this->parent_->get_nuki_lock()->setTimeZoneOffset(value)) {
            this->parent_->get_nuki_lock_config()->timeZoneOffset = value;
            this->publish_state(value);
        }
    }
}
void NukiLockLockNGoTimeoutNumber::control(float value) {
    if (value >= 5 && value <= 60) {
        if(this->parent_->get_nuki_lock()->setLockNgoTimeout(value)) {
            this->parent_->get_nuki_lock_advanced_config()->lockNgoTimeout = value;
            this->publish_state(value);
        }
    }
}
void NukiLockAutoLockTimeoutNumber::control(float value) {
    if (value >= 30 && value <= 1800) {
        if(this->parent_->get_nuki_lock()->setAutoLockTimeOut(value)) {
            this->parent_->get_nuki_lock_advanced_config()->autoLockTimeOut = value;
            this->publish_state(value);
        }
    }
}
void NukiLockUnlatchDurationNumber::control(float value) {
    if (value >= 1 && value <= 30) {
        if(this->parent_->get_nuki_lock()->setUnlatchDuration(value)) {
            this->parent_->get_nuki_lock_advanced_config()->unlatchDuration = value;
            this->publish_state(value);
        }
    }
}
void NukiLockUnlockedPositionOffsetDegreesNumber::control(float value) {
    if (value >= -90 && value <= 180) {
        if(this->parent_->get_nuki_lock()->setUnlockedPositionOffsetDegrees(value)) {
            this->parent_->get_nuki_lock_advanced_config()->unlockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    }
}
void NukiLockLockedPositionOffsetDegreesNumber::control(float value) {
    if (value >= -180 && value <= 90) {
        if(this->parent_->get_nuki_lock()->setLockedPositionOffsetDegrees(value)) {
            this->parent_->get_nuki_lock_advanced_config()->lockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    }
}
void NukiLockSingleLockedPositionOffsetDegreesNumber::control(float value) {
    if (value >= -180 && value <= 180) {
        if(this->parent_->get_nuki_lock()->setSingleLockedPositionOffsetDegrees(value)) {
            this->parent_->get_nuki_lock_advanced_config()->singleLockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    }
}
void NukiLockUnlockedToLockedTransitionOffsetDegreesNumber::control(float value) {
    if (value >= -180 && value <= 180) {
        if(this->parent_->get_nuki_lock()->setUnlockedToLockedTransitionOffsetDegrees(value)) {
            this->parent_->get_nuki_lock_advanced_config()->unlockedToLockedTransitionOffsetDegrees = value;
            this->publish_state(value);
        }
    }
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

void NukiLockComponent::add_event_log_received_callback(std::function<void(NukiLock::LogEntry)> &&callback)
{
    this->event_log_received_callback_.add(std::move(callback));
}

}