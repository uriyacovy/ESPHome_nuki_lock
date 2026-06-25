#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#include <algorithm>
#include <map>
#include <vector>

#include "nuki_lock.h"
#include "nuki_lock_utils.h"

namespace esphome::nuki_lock {

static const char *const TAG = "nuki_lock.lock";

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
    CmdResult cmd_result = this->nuki_lock_.request_key_turner_state(&(this->retrieved_key_turner_state_));
    if (cmd_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->status_update_ = false;
    this->nuki_op_active_ = false;

    char str[50] = {0};
    cmd_result_to_string(cmd_result, str);

    App.feed_wdt();

    if (cmd_result == CmdResult::Success) {
        ESP_LOGD(TAG, "request_key_turner_state has resulted in %s (%d)", str, cmd_result);

        this->status_update_consecutive_errors_ = 0;
        this->connected_ = true;

        LockState current_lock_state = this->retrieved_key_turner_state_.lockState;
        char current_lock_state_as_string[30] = {0};
        lock_state_to_string(current_lock_state, current_lock_state_as_string);

        ESP_LOGI(TAG, "Lock state: %s (%d), Battery (state: %#x, critical: %d, level: %d, charging: %s), Time: %d:%d:%d",
            current_lock_state_as_string,
            current_lock_state,
            this->retrieved_key_turner_state_.criticalBatteryState,
            this->nuki_lock_.is_battery_critical(),
            this->nuki_lock_.get_battery_perc(),
            YESNO(this->nuki_lock_.is_battery_charging()),
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
            this->battery_critical_binary_sensor_->publish_state(this->nuki_lock_.is_battery_critical());
        }

        if (this->battery_charging_binary_sensor_ != nullptr) {
            this->battery_charging_binary_sensor_->publish_state(this->nuki_lock_.is_battery_charging());
        }

        if (this->keypad_battery_critical_binary_sensor_ != nullptr) {
            this->keypad_battery_critical_binary_sensor_->publish_state(this->nuki_lock_.is_keypad_battery_critical());
        }

        if (this->door_sensor_battery_critical_binary_sensor_ != nullptr) {
            this->door_sensor_battery_critical_binary_sensor_->publish_state(this->nuki_lock_.is_door_sensor_battery_critical());
        }

        // Whether the lock's remote-access (cloud/SSE) uplink is currently connected, via
        // any transport (bridge, the lock's own WiFi, or Thread) - only supported by Smart
        // Lock 4th Generation and Ultra; 255 is the struct's default-init sentinel.
        if (this->remote_access_connected_binary_sensor_ != nullptr) {
            uint8_t remote_access_status = this->retrieved_key_turner_state_.remoteAccessStatus;
            if (remote_access_status != 255) {
                this->remote_access_connected_binary_sensor_->publish_state((remote_access_status & 8) == 8);
            } else {
                this->remote_access_connected_binary_sensor_->invalidate_state();
            }
        }

        // If pin needs validation, validate now
        if(this->pin_state_ == PinState::Set) {
            validate_pin();
        }
        
        if (this->door_sensor_binary_sensor_ != nullptr) {
            DoorSensorState door_sensor_state = this->retrieved_key_turner_state_.doorSensorState;
            if(door_sensor_state != DoorSensorState::Unavailable) {
                this->door_sensor_binary_sensor_->publish_state(nuki_lock::nuki_doorsensor_to_binary(door_sensor_state));
            } else {
                this->door_sensor_binary_sensor_->invalidate_state();
            }
        }
        #endif
        #ifdef USE_SENSOR
        if (this->battery_level_sensor_ != nullptr) {
            this->battery_level_sensor_->publish_state(this->nuki_lock_.get_battery_perc());
        }
        if (this->bt_signal_sensor_ != nullptr) {
            this->bt_signal_sensor_->publish_state(this->nuki_lock_.get_rssi());
        }
        // Only supported by Smart Lock 4th Generation and Ultra; negative values are the
        // actual RSSI, 0 means invalid and 1 means not supported.
        if (this->wifi_connection_strength_sensor_ != nullptr) {
            int8_t wifi_connection_strength = this->retrieved_key_turner_state_.wifiConnectionStrength;
            if (wifi_connection_strength < 0) {
                this->wifi_connection_strength_sensor_->publish_state(wifi_connection_strength);
            } else {
                this->wifi_connection_strength_sensor_->publish_state(NAN);
            }
        }
        #endif
        #ifdef USE_TEXT_SENSOR
        if (this->door_sensor_state_text_sensor_ != nullptr){
            memset(str, 0, sizeof(str));
            door_sensor_state_to_string(this->retrieved_key_turner_state_.doorSensorState, str);
            this->door_sensor_state_text_sensor_->publish_state(str);
        }

        if (this->last_lock_action_text_sensor_ != nullptr) {
            memset(str, 0, sizeof(str));
            lock_action_to_string(this->retrieved_key_turner_state_.lastLockAction, str);
            this->last_lock_action_text_sensor_->publish_state(str);
        }

        if (this->last_lock_action_trigger_text_sensor_ != nullptr) {
            memset(str, 0, sizeof(str));
            trigger_to_string(this->retrieved_key_turner_state_.lastLockActionTrigger, str);
            this->last_lock_action_trigger_text_sensor_->publish_state(str);
        }

        if (this->last_unlock_user_text_sensor_ != nullptr && this->retrieved_key_turner_state_.lastLockActionTrigger == NukiTrigger::Manual)
        {
            this->last_unlock_user_text_sensor_->publish_state("Manual");
        }

        // WiFi/MQTT/Thread connection status - only supported by Smart Lock 4th Generation
        // and Ultra; 255 is the struct's default-init sentinel for "not received yet".
        char status_str[80] = {0};
        if (this->wifi_connection_status_text_sensor_ != nullptr &&
            this->retrieved_key_turner_state_.wifiConnectionStatus != 255) {
            wifi_connection_status_to_string(this->retrieved_key_turner_state_.wifiConnectionStatus, status_str);
            this->wifi_connection_status_text_sensor_->publish_state(status_str);
        }
        if (this->mqtt_connection_status_text_sensor_ != nullptr &&
            this->retrieved_key_turner_state_.mqttConnectionStatus != 255) {
            mqtt_connection_status_to_string(this->retrieved_key_turner_state_.mqttConnectionStatus, status_str);
            this->mqtt_connection_status_text_sensor_->publish_state(status_str);
        }
        if (this->thread_connection_status_text_sensor_ != nullptr &&
            this->retrieved_key_turner_state_.threadConnectionStatus != 255) {
            thread_connection_status_to_string(this->retrieved_key_turner_state_.threadConnectionStatus, status_str);
            this->thread_connection_status_text_sensor_->publish_state(status_str);
        }
        #endif

        if (this->retrieved_key_turner_state_.lockState == LockState::Locking ||
            this->retrieved_key_turner_state_.lockState == LockState::Unlocking ||
            this->retrieved_key_turner_state_.lockState == LockState::Calibration) {
            // Schedule a status update without waiting for the next advertisement because the lock
            // is in a transition state. This will speed up the feedback.
            this->status_update_ = true;
            
            if (this->send_events_) {
                this->event_log_update_ = true;
            }
        }
    } else {
        ESP_LOGE(TAG, "request_key_turner_state has resulted in %s (%d)", str, cmd_result);

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
    CmdResult conf_req_result = this->nuki_lock_.request_config(&this->nuki_lock_config_);
    if (conf_req_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->config_update_ = false;
    this->nuki_op_active_ = false;

    char str[50] = {0};
    cmd_result_to_string(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == CmdResult::Success) {
        ESP_LOGD(TAG, "request_config has resulted in %s (%d)", str, conf_req_result);

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
            this->pairing_enabled_switch_->publish_state(this->nuki_lock_config_.pairing_enabled);
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
        ESP_LOGE(TAG, "request_config has resulted in %s (%d)", str, conf_req_result);
        this->config_update_ = true;
    }
}

void NukiLockComponent::update_advanced_config() {
    CmdResult conf_req_result = this->nuki_lock_.request_advanced_config(&this->nuki_lock_advanced_config_);
    if (conf_req_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->advanced_config_update_ = false;
    this->nuki_op_active_ = false;

    char str[50] = {0};
    cmd_result_to_string(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == CmdResult::Success) {
        ESP_LOGD(TAG, "request_advanced_config has resulted in %s (%d)", str, conf_req_result);

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
        if (!this->nuki_lock_.is_lock_ultra() && this->auto_battery_type_detection_enabled_switch_ != nullptr) {
            this->auto_battery_type_detection_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.automaticBatteryTypeDetection);
        }

        // Ultra only
        if (this->nuki_lock_.is_lock_ultra() && this->slow_speed_during_night_mode_enabled_switch_ != nullptr) {
            this->slow_speed_during_night_mode_enabled_switch_->publish_state(this->nuki_lock_advanced_config_.enable_slow_speed_during_night_mode);
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
        if (!this->nuki_lock_.is_lock_ultra() && this->battery_type_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::battery_type_to_string(this->nuki_lock_advanced_config_.batteryType, str);
            this->battery_type_select_->publish_state(str);
        }

        // Ultra
        if (this->nuki_lock_.is_lock_ultra() && this->motor_speed_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            nuki_lock::motor_speed_to_string(this->nuki_lock_advanced_config_.motorSpeed, str);
            this->motor_speed_select_->publish_state(str);
        }
        #endif
    } else {
        ESP_LOGE(TAG, "request_advanced_config has resulted in %s (%d)", str, conf_req_result);
        this->advanced_config_update_ = true;
    }
}

void NukiLockComponent::update_auth_data() {
    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        this->auth_data_update_ = false;
        this->nuki_op_active_ = false;
        return;
    }

    CmdResult auth_data_req_result = this->nuki_lock_.retrieve_authorization_entries(0, MAX_AUTH_DATA_ENTRIES);
    if (auth_data_req_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->auth_data_update_ = false;
    this->nuki_op_active_ = false;

    char auth_data_req_result_as_string[30] = {0};
    cmd_result_to_string(auth_data_req_result, auth_data_req_result_as_string);

    App.feed_wdt();

    if (auth_data_req_result == CmdResult::Success) {
        ESP_LOGD(TAG, "retrieve_authorization_entries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);
        this->auth_data_ready_time_ = millis() + 5000;
    } else {
        ESP_LOGE(TAG, "retrieve_authorization_entries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);
        this->auth_data_update_ = true;
    }
}

void NukiLockComponent::process_auth_data() {
    ESP_LOGD(TAG, "Process Authorization Entries");

    std::vector<AuthorizationEntry> authEntries;
    this->nuki_lock_.get_authorization_entries(&authEntries);

    App.feed_wdt();

    if (authEntries.empty()) {
        ESP_LOGW(TAG, "No auth entries!");
        return;
    }

    ESP_LOGD(TAG, "Authorization Entry Count: %d", authEntries.size());

    std::sort(authEntries.begin(), authEntries.end(), [](const AuthorizationEntry& a, const AuthorizationEntry& b) {
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

void NukiLockComponent::update_battery_report() {
    CmdResult result = this->nuki_lock_.request_battery_report(&this->battery_report_);
    if (result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->battery_report_update_ = false;
    this->nuki_op_active_ = false;

    char str[50] = {0};
    cmd_result_to_string(result, str);

    App.feed_wdt();

    if (result == CmdResult::Success) {
        ESP_LOGD(TAG, "request_battery_report has resulted in %s (%d)", str, result);

        #ifdef USE_SENSOR
        if (this->battery_voltage_sensor_ != nullptr) {
            this->battery_voltage_sensor_->publish_state(this->battery_report_.batteryVoltage / 1000.0f);
        }

        if (this->battery_drain_sensor_ != nullptr) {
            this->battery_drain_sensor_->publish_state(this->battery_report_.batteryDrain);
        }

        if (this->motor_current_sensor_ != nullptr) {
            this->motor_current_sensor_->publish_state(this->battery_report_.maxTurnCurrent);
        }
        #endif
    } else {
        ESP_LOGE(TAG, "request_battery_report has resulted in %s (%d)", str, result);
        this->battery_report_update_ = true;
    }
}

void NukiLockComponent::update_event_logs() {
    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        this->event_log_update_ = false;
        this->nuki_op_active_ = false;
        return;
    }

    CmdResult event_log_req_result = this->nuki_lock_.retrieve_log_entries(0, MAX_EVENT_LOG_ENTRIES, 1, true);
    if (event_log_req_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }
    this->event_log_update_ = false;
    this->nuki_op_active_ = false;

    char event_log_req_result_as_string[30] = {0};
    cmd_result_to_string(event_log_req_result, event_log_req_result_as_string);

    App.feed_wdt();

    if (event_log_req_result == CmdResult::Success) {
        ESP_LOGD(TAG, "retrieve_log_entries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);
        this->event_log_ready_time_ = millis() + 5000;

        #ifdef USE_SWITCH
        if (this->logging_enabled_switch_ != nullptr) {
            this->logging_enabled_switch_->publish_state(this->nuki_lock_.get_logging_enabled());
        }
        #endif
    } else {
        ESP_LOGE(TAG, "retrieve_log_entries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);
        this->event_log_update_ = true;
    }
}

void NukiLockComponent::process_log_entries() {
    ESP_LOGD(TAG, "Process Event Log Entries");

    std::vector<LogEntry> log_entries;
    this->nuki_lock_.get_log_entries(&log_entries);

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

    std::sort(log_entries.begin(), log_entries.end(), [](const LogEntry& a, const LogEntry& b) {
        return a.index < b.index;
    });

    char buffer[50] = {0};
    char num_buffer[16];
    uint32_t auth_index = 0;

    std::map<std::string, std::string> event_data;

    for (const auto& log : log_entries) {
        event_data.clear();

        if (log.loggingType == LoggingType::LockAction ||
            log.loggingType == LoggingType::KeypadAction) {
            
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

            logging_type_to_string(log.loggingType, buffer);
            event_data["type"] = buffer;

            switch (log.loggingType) {
                case LoggingType::LockAction:
                    lock_action_to_string((LockAction)log.data[0], buffer);
                    event_data["action"] = buffer;
                    trigger_to_string((NukiTrigger)log.data[1], buffer);
                    event_data["trigger"] = buffer;
                    completion_status_to_string((CompletionStatus)log.data[3], buffer);
                    event_data["completionStatus"] = buffer;
                    break;

                case LoggingType::KeypadAction:
                {
                    lock_action_to_string((LockAction)log.data[0], buffer);
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
                        completion_status_to_string((CompletionStatus)log.data[2], buffer);
                        event_data["completionStatus"] = buffer;
                    }

                    unsigned int codeId = 256U * log.data[4] + log.data[3];
                    snprintf(num_buffer, sizeof(num_buffer), "%u", codeId);
                    event_data["codeId"] = num_buffer;
                    break;
                }

                case LoggingType::DoorSensor:
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

// Performs one step of the currently executing lock action attempt. Safe to call
// repeatedly (once per loop() tick) until it completes - see nuki_op_active_ in nuki_lock.h.
void NukiLockComponent::execute_lock_action_step() {
    if (!this->lock_action_in_flight_) {
        // Starting a new attempt: snapshot which action we're executing so a *new*
        // control() call arriving while this attempt is still working doesn't change
        // which action the in-flight BLE command is for.
        this->action_attempts_--;
        this->executing_lock_action_ = this->lock_action_;
        this->executing_lock_action_flags_ = (this->auto_unlock_ ? 1 : 0) | (this->force_ ? 2 : 0);
        this->executing_lock_action_name_suffix_ = this->name_suffix_;
        this->force_ = false;
        this->auto_unlock_ = false;
        this->name_suffix_.clear();
        this->lock_action_in_flight_ = true;

        char currentlock_action_as_string[30] = {0};
        lock_action_to_string(this->executing_lock_action_, currentlock_action_as_string);
        ESP_LOGD(TAG, "Executing lock action %s (%d)... (%d attempts left)", currentlock_action_as_string, this->executing_lock_action_, this->action_attempts_);

        // Publish the assumed transitional lock state
        switch (this->executing_lock_action_) {
            case LockAction::Unlatch:
            case LockAction::Unlock: {
                this->publish_state(lock::LOCK_STATE_UNLOCKING);
                break;
            }
            case LockAction::FullLock:
            case LockAction::Lock:
            case LockAction::LockNgo: {
                this->publish_state(lock::LOCK_STATE_LOCKING);
                break;
            }
        }
    }

    CmdResult result = this->nuki_lock_.lock_action(
        this->executing_lock_action_, 1, this->executing_lock_action_flags_,
        this->executing_lock_action_name_suffix_.empty() ? nullptr : this->executing_lock_action_name_suffix_.c_str(),
        this->executing_lock_action_name_suffix_.empty() ? 0 : (uint8_t)std::min(this->executing_lock_action_name_suffix_.size(), (size_t)19));

    App.feed_wdt();

    if (result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }

    this->lock_action_in_flight_ = false;
    this->nuki_op_active_ = false;

    char lock_action_as_string[30] = {0};
    lock_action_to_string(this->executing_lock_action_, lock_action_as_string);

    char result_as_string[30] = {0};
    cmd_result_to_string(result, result_as_string);

    bool is_execution_successful = (result == CmdResult::Success);
    if (is_execution_successful) {
        ESP_LOGI(TAG, "lock_action %s (%d) has resulted in %s (%d)", lock_action_as_string, this->executing_lock_action_, result_as_string, result);

        if (this->lock_action_ == this->executing_lock_action_) {
            // Stop action attempts only if no new action was received in the meantime.
            // Otherwise, the new action won't be executed.
            this->action_attempts_ = 0;
        }
    } else {
        ESP_LOGE(TAG, "lock_action %s (%d) has resulted in %s (%d)", lock_action_as_string, this->executing_lock_action_, result_as_string, result);

        if (this->action_attempts_ == 0) {
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
    }

    // Schedule a status update without waiting for the next advertisement for a faster feedback
    this->status_update_ = true;

    // Give the lock extra time when successful in order to account for time to turn the key;
    // on failure, wait the configurable retry delay before the next attempt.
    this->command_cooldown_millis = is_execution_successful ? COOLDOWN_COMMANDS_EXTENDED_MILLIS : this->command_retry_delay_millis_;
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
    const bool result = this->nuki_lock_.is_lock_ultra() ? this->nuki_lock_.save_ultra_pincode(pin_to_use) : this->nuki_lock_.save_security_pincode(static_cast<uint16_t>(pin_to_use));

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
    if (this->nuki_lock_.is_paired_with_lock() && this->connected_) {
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

// Performs one step of pin validation (spaced ~100ms apart, up to 4 attempts). Safe to
// call repeatedly (once per loop() tick) until it completes - see nuki_op_active_ in
// nuki_lock.h.
void NukiLockComponent::validate_pin_step() {
    if (this->pin_validation_attempts_ == 0) {
        // First attempt, start timer
        this->pin_validation_start_time_ = millis();
        this->pin_validation_attempts_ = 4;
    }

    // Check if enough time has passed since last attempt (100ms)
    if (millis() - this->pin_validation_start_time_ < 100) {
        return;
    }
    this->pin_validation_start_time_ = millis();

    App.feed_wdt();
    CmdResult pin_result = this->nuki_lock_.verify_security_pin();
    App.feed_wdt();

    if (pin_result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }

    this->nuki_op_active_ = false;
    this->pin_validation_attempts_--;
    ESP_LOGD(TAG, "verify_security_pin attempts left: %d", this->pin_validation_attempts_);

    if (pin_result == CmdResult::Success) {
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
        ESP_LOGW(TAG, "verify_security_pin: result %d, retry...", pin_result);
    }
}

// Queues a Nuki command to run on the single in-flight Nuki BLE command slot - see
// nuki_op_active_ in nuki_lock.h. `command` is typically a `nuki_lock_.setXxx(...)` call;
// `on_done` is called once with the terminal result, asynchronously (not necessarily on
// the same tick this was queued on). If `command` fails (any result other than Success),
// it is retried up to command_retries_ times, waiting command_retry_delay_millis_ between
// attempts, before on_done is called with the (final) failure result.
void NukiLockComponent::queue_nuki_command(std::function<CmdResult()> command, std::function<void(CmdResult)> on_done) {
    this->pending_nuki_command_ = std::move(command);
    this->pending_nuki_command_done_ = std::move(on_done);
    this->pending_command_retries_left_ = this->command_retries_;
}

void NukiLockComponent::process_pending_nuki_command() {
    CmdResult result = this->pending_nuki_command_();
    if (result == CmdResult::Working) {
        return;  // still in flight on the single Nuki command slot; retry next tick
    }

    this->nuki_op_active_ = false;

    if (result != CmdResult::Success && this->pending_command_retries_left_ > 0) {
        this->pending_command_retries_left_--;
        ESP_LOGW(TAG, "Queued Nuki command failed (result %d), retrying in %ums (%d attempts left)",
            result, this->command_retry_delay_millis_, this->pending_command_retries_left_);
        this->command_cooldown_millis = this->command_retry_delay_millis_;
        return;  // pending_nuki_command_/_done_ stay set - process_pending_nuki_command()
                 // runs again once the cooldown above passes, retrying the same command.
    }

    this->command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
    std::function<void(CmdResult)> on_done = std::move(this->pending_nuki_command_done_);
    this->pending_nuki_command_ = nullptr;
    this->pending_nuki_command_done_ = nullptr;
    if (on_done) {
        on_done(result);
    }
}

void NukiLockComponent::loop()
{
    // nuki_lock_ is not registered with App as its own Component (NukiLockComponent is the
    // one ESPHome schedules), so its BLEClientBase::loop() - which drives GATTC app
    // registration and other connection-lifecycle bookkeeping - has to be driven manually.
    // Unlike the Nuki-protocol work below, this runs on every single call, not gated to
    // 500ms, since BLE connection state benefits from being processed promptly.
    this->nuki_lock_.loop();

    // Throttle to every 500ms while idle - there's nothing to gain from running this any
    // faster when there's no operation in flight. But while a Nuki BLE operation is active,
    // each tick only advances its challenge/command/accept state machine by one step, so a
    // full 500ms gate between steps adds up to seconds of pure polling overhead on top of
    // actual BLE round-trip time, eating into the per-step command timeouts. Re-check much
    // more often (50ms) in that case so multi-step handshakes progress close to BLE speed.
    uint32_t current_time = millis();
    uint32_t loop_interval = this->nuki_op_active_ ? 50 : 500;
    if (current_time - this->last_loop_time_ < loop_interval) {
        return;
    }
    this->last_loop_time_ = current_time;

    // Advertisements are now scanned by esp32_ble_tracker (configured by the user's own
    // esp32_ble_tracker: block) and delivered via NukiBle::parse_device(), so there is no
    // scanner to kick here anymore.
    App.feed_wdt();

    // Terminate stale Bluetooth connections
    this->nuki_lock_.update_connection_state();
    App.feed_wdt();

    // Once a Nuki BLE operation is in flight (nuki_op_active_), it must keep being
    // re-driven every tick regardless of the cooldown below - the cooldown only applies
    // *between* completed operations, never while one is still working towards a result.
    if (!this->nuki_op_active_ && millis() - last_command_executed_time_ < command_cooldown_millis) {
        // Give the lock time to terminate the previous command
        uint64_t millisSinceLastExecution = millis() - last_command_executed_time_;
        uint64_t millisLeft = (millisSinceLastExecution < command_cooldown_millis) ? command_cooldown_millis - millisSinceLastExecution : 1;
        ESP_LOGV(TAG, "Cooldown period, %dms left", millisLeft);
        return;
    }

    if (this->nuki_lock_.is_paired_with_lock()) {
        // Only one Nuki BLE command can be in flight at a time. Whichever operation is
        // already active keeps being re-invoked (regardless of which other flags below
        // are also true) until it reaches a terminal result; only then can a new
        // operation be picked, in priority order: PIN validation, lock actions, queued
        // entity/service commands, then the various periodic polls.
        if (this->nuki_op_active_) {
            this->nuki_op_step_();
        } else if (this->pin_validation_pending_) {
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->validate_pin_step(); };
            this->nuki_op_step_();
        } else if (this->action_attempts_ > 0) {
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->execute_lock_action_step(); };
            this->nuki_op_step_();
        } else if (this->pending_nuki_command_) {
            ESP_LOGD(TAG, "Executing queued Nuki command...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->process_pending_nuki_command(); };
            this->nuki_op_step_();
            // command_cooldown_millis is set by process_pending_nuki_command() itself -
            // it differs between success, a retry-after-failure, and exhausted retries.
        } else if (this->status_update_) {
            ESP_LOGD(TAG, "Requesting status...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_status(); };
            this->nuki_op_step_();
            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
        } else if (this->config_update_) {
            ESP_LOGD(TAG, "Requesting config...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_config(); };
            this->nuki_op_step_();
            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
        } else if (this->auth_data_update_) {
            ESP_LOGD(TAG, "Requesting auth data...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_auth_data(); };
            this->nuki_op_step_();
            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
        } else if (this->event_log_update_) {
            ESP_LOGD(TAG, "Requesting event logs...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_event_logs(); };
            this->nuki_op_step_();
            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
        } else if (this->advanced_config_update_) {
            ESP_LOGD(TAG, "Requesting advanced config...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_advanced_config(); };
            this->nuki_op_step_();
            command_cooldown_millis = COOLDOWN_COMMANDS_MILLIS;
        } else if (this->battery_report_update_) {
            ESP_LOGD(TAG, "Requesting battery report...");
            this->nuki_op_active_ = true;
            this->nuki_op_step_ = [this] { this->update_battery_report(); };
            this->nuki_op_step_();
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
            // Pair Nuki (pair_nuki() performs one step per call and reports progress via
            // its return value, so this does not block loop() while pairing is ongoing)
            AuthorizationIdType type = this->pairing_as_app_.value_or(false) ? AuthorizationIdType::App : AuthorizationIdType::Bridge;

            App.feed_wdt();

            bool paired = this->nuki_lock_.pair_nuki(type) == PairingResult::Success;

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

void NukiLockComponent::setup() {
    ESP_LOGCONFIG(TAG, "Running setup");

    // Restore settings from flash. Keyed off device_name_ (unique per instance, see the
    // NukiLockComponent constructor) so multiple nuki_lock: instances on one device don't
    // clobber each other's stored PIN settings.
    this->pref_ = global_preferences->make_preference<NukiLockSettings>(
        esphome::fnv1_hash(this->device_name_ + "_settings"));

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
            this->nuki_lock_.save_ultra_pincode((unsigned int)pin_to_use, false);
        }
    }

    this->nuki_lock_.set_debug_connect(true);
    this->nuki_lock_.set_debug_communication(false);
    this->nuki_lock_.set_debug_readable_data(false);
    this->nuki_lock_.set_debug_hex_data(false);
    this->nuki_lock_.set_debug_command(false);

    // nuki_lock_ is not registered with App as its own Component, so its own
    // Component::setup() override (BLEClientBase::setup(), which assigns its
    // connection_index_) has to be called explicitly - see loop() for the equivalent.
    this->nuki_lock_.setup();
    this->nuki_lock_.set_event_handler(this);
    this->nuki_lock_.initialize();
    this->nuki_lock_.set_connect_timeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nuki_lock_.set_connect_retries(BLE_CONNECT_RETRIES);
    this->nuki_lock_.set_disconnect_timeout(BLE_DISCONNECT_TIMEOUT);
    this->nuki_lock_.set_general_timeout(this->ble_general_timeout_ * 1000);
    this->nuki_lock_.set_command_timeout(this->ble_command_timeout_ * 1000);
    this->nuki_lock_.set_config_cache_ttl(this->config_cache_ttl_ * 1000);

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
        this->register_service(&NukiLockComponent::set_lock_action_options, "set_lock_action_options", {"force", "auto_unlock", "name_suffix"});
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
    if (this->nuki_lock_.is_paired_with_lock()) {
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
}

void NukiLockComponent::setup_lock(bool new_pairing) {
    const char* pairing_type = this->pairing_as_app_.value_or(false) ? "App" : "Bridge";
    const char* lock_type = this->nuki_lock_.is_lock_ultra() ? "5th Gen (Ultra / Go / Pro)" : "1st - 4th Gen";
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
        } else if (!this->nuki_lock_.is_lock_ultra() && pin_to_use > 65535) {
            ESP_LOGE(TAG, "Security pin exceeds maximum of 65535 for 1st to 4th Gen Smart Locks", pin_to_use);
            this->pin_state_ = PinState::Invalid;
        } else {
            const bool result = this->nuki_lock_.is_lock_ultra() ? this->nuki_lock_.save_ultra_pincode(pin_to_use) : this->nuki_lock_.save_security_pincode(static_cast<uint16_t>(pin_to_use));

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
    this->battery_report_update_ = true;
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
    this->cancel_interval("update_battery_report");

    if(setup) {
        this->set_interval("update_config", this->query_interval_config_ * 1000, [this]() {
            this->config_update_ = true;
            this->advanced_config_update_ = true;
        });

        this->set_interval("update_auth_data", this->query_interval_auth_data_ * 1000, [this]() {
            this->auth_data_update_ = true;
        });

        this->set_interval("update_battery_report", this->query_interval_battery_report_ * 1000, [this]() {
            this->battery_report_update_ = true;
        });
    }
}

/**
 * @brief Add a new lock action that will be executed on the next update() call.
 */
void NukiLockComponent::control(const lock::LockCall &call) {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot execute lock action");
        return;
    }

    lock::LockState state = *call.get_state();

    switch(state) {
        case lock::LOCK_STATE_LOCKED:
            this->action_attempts_ = this->command_retries_;
            this->lock_action_ = LockAction::Lock;
            break;

        case lock::LOCK_STATE_UNLOCKED: {
            this->action_attempts_ = this->command_retries_;
            this->lock_action_ = LockAction::Unlock;

            if (this->open_latch_) {
                this->lock_action_ = LockAction::Unlatch;
            }

            if (this->lock_n_go_) {
                this->lock_action_ = LockAction::LockNgo;
            }

            this->open_latch_ = false;
            this->lock_n_go_ = false;
            break;
        }

        default:
            ESP_LOGE(TAG, "lock_action unsupported state");
            return;
    }

    char lock_action_as_string[30] = {0};
    lock_action_to_string(this->lock_action_, lock_action_as_string);
    lock_action_as_string[sizeof(lock_action_as_string) - 1] = '\0';
    ESP_LOGI(TAG, "New lock action received: %s (%d)", lock_action_as_string, this->lock_action_);
}

void NukiLockComponent::lock_n_go() {
    this->lock_n_go_ = true;
    this->unlock();
}

void NukiLockComponent::set_lock_action_options(bool force, bool auto_unlock, std::string name_suffix) {
    this->force_ = force;
    this->auto_unlock_ = auto_unlock;
    this->name_suffix_ = std::move(name_suffix);
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
    ESP_LOGCONFIG(TAG, "  Battery Report query interval: %us", this->query_interval_battery_report_);
    ESP_LOGCONFIG(TAG, "  BLE general timeout: %us", this->ble_general_timeout_);
    ESP_LOGCONFIG(TAG, "  BLE command timeout: %us", this->ble_command_timeout_);
    ESP_LOGCONFIG(TAG, "  Command retries: %u", this->command_retries_);
    ESP_LOGCONFIG(TAG, "  Command retry delay: %ums", this->command_retry_delay_millis_);
    ESP_LOGCONFIG(TAG, "  Config cache TTL: %us", this->config_cache_ttl_);

    char pin_state_as_string[30] = {0};
    nuki_lock::pin_state_to_string(this->pin_state_, pin_state_as_string);
    ESP_LOGCONFIG(TAG, "  Last known security pin state: %s", pin_state_as_string);

    LOG_LOCK(TAG, "Nuki Lock", this);
    #ifdef USE_BINARY_SENSOR
    LOG_BINARY_SENSOR(TAG, "Connected", this->connected_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Paired", this->paired_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Battery Critical", this->battery_critical_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Battery Charging", this->battery_charging_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Keypad Battery Critical", this->keypad_battery_critical_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor Battery Critical", this->door_sensor_battery_critical_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Door Sensor", this->door_sensor_binary_sensor_);
    LOG_BINARY_SENSOR(TAG, "Remote Access Connected", this->remote_access_connected_binary_sensor_);
    #endif
    #ifdef USE_TEXT_SENSOR
    LOG_TEXT_SENSOR(TAG, "Door Sensor State", this->door_sensor_state_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Unlock User", this->last_unlock_user_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Lock Action", this->last_lock_action_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Lock Action Trigger", this->last_lock_action_trigger_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Pin Status", this->pin_state_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "WiFi Connection Status", this->wifi_connection_status_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "MQTT Connection Status", this->mqtt_connection_status_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Thread Connection Status", this->thread_connection_status_text_sensor_);
    #endif
    #ifdef USE_SENSOR
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_sensor_);
    LOG_SENSOR(TAG, "Battery Voltage", this->battery_voltage_sensor_);
    LOG_SENSOR(TAG, "Battery Drain", this->battery_drain_sensor_);
    LOG_SENSOR(TAG, "Motor Current", this->motor_current_sensor_);
    LOG_SENSOR(TAG, "Bluetooth Signal", this->bt_signal_sensor_);
    LOG_SENSOR(TAG, "WiFi Connection Strength", this->wifi_connection_strength_sensor_);
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
    LOG_SWITCH(TAG, "Logging Enabled", this->logging_enabled_switch_);
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

void NukiLockComponent::notify(EventType event_type) {
    ESP_LOGI(TAG, "Event notified %d", event_type);

    if(event_type == EventType::KeyTurnerStatusReset) {
        // IDK
        ESP_LOGD(TAG, "KeyTurnerStatusReset");
    } else if (event_type == EventType::ERROR_BAD_PIN) {
        // Invalid Pin
        ESP_LOGW(TAG, "Nuki reported an invalid security PIN");

        ESP_LOGD(TAG, "NVS PIN 1st-4th Gen: %d", this->nuki_lock_.get_security_pincode());
        ESP_LOGD(TAG, "NVS PIN 5th Gen (Ultra / Go / Pro): %d", this->nuki_lock_.get_ultra_pincode());
        ESP_LOGD(TAG, "ESPHome PIN (override): %d", this->security_pin_);
        ESP_LOGD(TAG, "ESPHome PIN (YAML): %d", this->security_pin_config_.value_or(0));

        const uint32_t saved_pin = this->nuki_lock_.is_lock_ultra() ? this->nuki_lock_.get_ultra_pincode() : this->nuki_lock_.get_security_pincode();
        const uint32_t actual_pin = this->security_pin_ != 0 ? this->security_pin_ : this->security_pin_config_.value_or(0);

        if(saved_pin != actual_pin) {
            ESP_LOGW(TAG, "The PIN stored in NVS does not match your configured PIN. Please remove leading zeros if any.");
        }

        this->pin_state_ = PinState::Invalid;
        this->save_settings();
        this->publish_pin_state();
    } else if(event_type == EventType::KeyTurnerStatusUpdated) {
        ESP_LOGD(TAG, "KeyTurnerStatusUpdated");

        // Request status update (incl. event log request)
        this->status_update_ = true;
    } else if(event_type == EventType::BLE_ERROR_ON_DISCONNECT) {
        // Reason code covers both a genuine link-layer connection timeout (e.g. service
        // discovery simply took too long, or the radio link was briefly lost - a normal,
        // recoverable BLE event, especially likely during pairing) and BLEClientBase's own
        // internal "never got CLOSE_EVT after we asked to disconnect" safety net - neither
        // warrants rebooting the whole device. The next connect() attempt (driven by
        // pair_nuki()/cmd_state_machine() polling, once state() is back to IDLE) already
        // recovers on its own.
        ESP_LOGW(TAG, "BLE connection ended unexpectedly (timeout), will retry automatically");
    }
}

void NukiLockComponent::unpair() {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot unpair");
        return;
    }

    this->nuki_lock_.unpair_nuki();

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
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot request calibration");
        return;
    }

    this->queue_nuki_command([this] { return this->nuki_lock_.request_calibration(); }, [](CmdResult result) {
        if (result == CmdResult::Success) {
            // Don't poll status here - calibration is a physical motor sweep that hasn't
            // finished yet at this point, so an immediate status request would just report
            // "calibration" again. The lock flips its advertised status-changed bit once
            // calibration actually completes, which parse_device() already picks up via
            // EventType::KeyTurnerStatusUpdated -> status_update_ = true, at the right time.
            ESP_LOGI(TAG, "Calibration requested successfully");
        } else {
            ESP_LOGE(TAG, "Failed to request calibration (result %d)", result);
        }
    });
}

void NukiLockComponent::set_pairing_mode(bool enabled) {
    this->pairing_mode_ = enabled;
    this->nuki_lock_.set_pairing_mode_active(enabled);

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

        ESP_LOGD(TAG, "NVS PIN 1st-4th Gen: %d", this->nuki_lock_.get_security_pincode());
        ESP_LOGD(TAG, "NVS PIN 5th Gen (Ultra / Go / Pro): %d", this->nuki_lock_.get_ultra_pincode());
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
    ButtonPressAction action = nuki_lock::button_press_action_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, action] { return this->parent_->get_nuki_lock()->set_single_button_press_action(action); }, [this, action, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->singleButtonPressAction = action;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockDoubleButtonPressActionSelect::control(const std::string &value) {
    ButtonPressAction action = nuki_lock::button_press_action_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, action] { return this->parent_->get_nuki_lock()->set_double_button_press_action(action); }, [this, action, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->doubleButtonPressAction = action;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockFobAction1Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action == 99) return;
    this->parent_->queue_nuki_command([this, action] { return this->parent_->get_nuki_lock()->set_fob_action(1, action); }, [this, action, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->fobAction1 = action;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockFobAction2Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action == 99) return;
    this->parent_->queue_nuki_command([this, action] { return this->parent_->get_nuki_lock()->set_fob_action(2, action); }, [this, action, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->fobAction2 = action;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockFobAction3Select::control(const std::string &value) {
    const uint8_t action = nuki_lock::fob_action_to_int(value.c_str());
    if (action == 99) return;
    this->parent_->queue_nuki_command([this, action] { return this->parent_->get_nuki_lock()->set_fob_action(3, action); }, [this, action, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->fobAction3 = action;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockTimeZoneSelect::control(const std::string &value) {
    TimeZoneId tzid = nuki_lock::timezone_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, tzid] { return this->parent_->get_nuki_lock()->set_time_zone_id(tzid); }, [this, tzid, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->timeZoneId = tzid;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockAdvertisingModeSelect::control(const std::string &value) {
    AdvertisingMode mode = nuki_lock::advertising_mode_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, mode] { return this->parent_->get_nuki_lock()->set_advertising_mode(mode); }, [this, mode, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->advertisingMode = mode;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockBatteryTypeSelect::control(const std::string &value) {
    if (this->parent_->get_nuki_lock()->is_lock_ultra()) {
        ESP_LOGE(TAG, "Battery Type is not supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
        return;
    }
    BatteryType type = nuki_lock::battery_type_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, type] { return this->parent_->get_nuki_lock()->set_battery_type(type); }, [this, type, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->batteryType = type;
            this->publish_state(value.c_str());
        }
    });
}

void NukiLockMotorSpeedSelect::control(const std::string &value) {
    if (!this->parent_->get_nuki_lock()->is_lock_ultra()) {
        ESP_LOGE(TAG, "Motor Speed is only supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
        return;
    }
    MotorSpeed speed = nuki_lock::motor_speed_to_enum(value.c_str());
    this->parent_->queue_nuki_command([this, speed] { return this->parent_->get_nuki_lock()->set_motor_speed(speed); }, [this, speed, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->motorSpeed = speed;
            this->publish_state(value.c_str());
        }
    });
}
#endif

#ifdef USE_SWITCH
void NukiLockPairingModeSwitch::write_state(bool state) {
    this->parent_->set_pairing_mode(state);
}

void NukiLockPairingEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_pairing(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->pairing_enabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockAutoUnlatchEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_auto_unlatch(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->autoUnlatch = state;
            this->publish_state(state);
        }
    });
}

void NukiLockButtonEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_button(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->buttonEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockLedEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_led_flash(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->ledEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockNightModeEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_night_mode(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->nightModeEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockNightModeAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_night_mode_auto_lock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->nightModeAutoLockEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockNightModeAutoUnlockDisabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->disable_night_mode_auto_unlock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->nightModeAutoUnlockDisabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockNightModeImmediateLockOnStartEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_night_mode_immediate_lock_on_start(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->nightModeImmediateLockOnStart = state;
            this->publish_state(state);
        }
    });
}

void NukiLockAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_auto_lock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->autoLockEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockAutoUnlockDisabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->disable_auto_unlock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->autoUnLockDisabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockImmediateAutoLockEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_immediate_auto_lock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->immediateAutoLockEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockAutoUpdateEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_auto_update(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->autoUpdateEnabled = state;
            this->publish_state(state);
        }
    });
}

void NukiLockSingleLockEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_single_lock(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->singleLock = state;
            this->publish_state(state);
        }
    });
}

void NukiLockDstModeEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_dst(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->dstMode = state;
            this->publish_state(state);
        }
    });
}

void NukiLockAutoBatteryTypeDetectionEnabledSwitch::write_state(bool state) {
    if (this->parent_->get_nuki_lock()->is_lock_ultra()) {
        ESP_LOGE(TAG, "Auto Battery Type Detection is not supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
        return;
    }
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_auto_battery_type_detection(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->automaticBatteryTypeDetection = state;
            this->publish_state(state);
        }
    });
}

void NukiLockSlowSpeedDuringNightModeEnabledSwitch::write_state(bool state) {
    if (!this->parent_->get_nuki_lock()->is_lock_ultra()) {
        ESP_LOGE(TAG, "Slow Speed During Night Mode is only supported for 5th Gen Smart Locks (Ultra / Go / Pro)");
        return;
    }
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_slow_speed_during_night_mode(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->enable_slow_speed_during_night_mode = state;
            this->publish_state(state);
        }
    });
}
void NukiLockDetachedCylinderEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_detached_cylinder(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->detachedCylinder = state;
            this->publish_state(state);
        }
    });
}
void NukiLockLoggingEnabledSwitch::write_state(bool state) {
    this->parent_->queue_nuki_command([this, state] { return this->parent_->get_nuki_lock()->enable_logging(state); }, [this, state](CmdResult result) {
        if (result == CmdResult::Success) {
            this->publish_state(state);
        }
    });
}
#endif

#ifdef USE_NUMBER
void NukiLockLedBrightnessNumber::control(float value) {
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_led_brightness(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->ledBrightness = value;
            this->publish_state(value);
        }
    });
}
void NukiLockTimeZoneOffsetNumber::control(float value) {
    if (value < -60 || value > 60) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_time_zone_offset(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_config()->timeZoneOffset = value;
            this->publish_state(value);
        }
    });
}
void NukiLockLockNGoTimeoutNumber::control(float value) {
    if (value < 5 || value > 60) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_lock_ngo_timeout(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->lockNgoTimeout = value;
            this->publish_state(value);
        }
    });
}
void NukiLockAutoLockTimeoutNumber::control(float value) {
    if (value < 30 || value > 1800) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_auto_lock_time_out(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->autoLockTimeOut = value;
            this->publish_state(value);
        }
    });
}
void NukiLockUnlatchDurationNumber::control(float value) {
    if (value < 1 || value > 30) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_unlatch_duration(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->unlatchDuration = value;
            this->publish_state(value);
        }
    });
}
void NukiLockUnlockedPositionOffsetDegreesNumber::control(float value) {
    if (value < -90 || value > 180) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_unlocked_position_offset_degrees(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->unlockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    });
}
void NukiLockLockedPositionOffsetDegreesNumber::control(float value) {
    if (value < -180 || value > 90) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_locked_position_offset_degrees(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->lockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    });
}
void NukiLockSingleLockedPositionOffsetDegreesNumber::control(float value) {
    if (value < -180 || value > 180) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_single_locked_position_offset_degrees(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->singleLockedPositionOffsetDegrees = value;
            this->publish_state(value);
        }
    });
}
void NukiLockUnlockedToLockedTransitionOffsetDegreesNumber::control(float value) {
    if (value < -180 || value > 180) return;
    this->parent_->queue_nuki_command([this, value] { return this->parent_->get_nuki_lock()->set_unlocked_to_locked_transition_offset_degrees(value); }, [this, value](CmdResult result) {
        if (result == CmdResult::Success) {
            this->parent_->get_nuki_lock_advanced_config()->unlockedToLockedTransitionOffsetDegrees = value;
            this->publish_state(value);
        }
    });
}
#endif

}  // namespace esphome::nuki_lock