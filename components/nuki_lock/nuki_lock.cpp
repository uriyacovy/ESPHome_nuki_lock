#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/preferences.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#include <map>

#include "nuki_lock.h"

namespace esphome {
namespace nuki_lock {

uint32_t global_nuki_lock_id = 1912044075ULL;

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

bool NukiLockComponent::nuki_doorsensor_to_binary(Nuki::DoorSensorState nuki_door_sensor_state) {
    if (nuki_door_sensor_state == Nuki::DoorSensorState::DoorClosed) {
        return false;
    }
    return true;
}

NukiLock::ButtonPressAction NukiLockComponent::button_press_action_to_enum(const char* str)
{
    if (strcmp(str, "No Action") == 0) {
        return NukiLock::ButtonPressAction::NoAction;
    } else if (strcmp(str, "Intelligent") == 0) {
        return NukiLock::ButtonPressAction::Intelligent;
    } else if (strcmp(str, "Unlock") == 0) {
        return NukiLock::ButtonPressAction::Unlock;
    } else if (strcmp(str, "Lock") == 0) {
        return NukiLock::ButtonPressAction::Lock;
    } else if (strcmp(str, "Unlatch") == 0) {
        return NukiLock::ButtonPressAction::Unlatch;
    } else if (strcmp(str, "Lock n Go") == 0) {
        return NukiLock::ButtonPressAction::LockNgo;
    } else if (strcmp(str, "Show Status") == 0) {
        return NukiLock::ButtonPressAction::ShowStatus;
    }
    return NukiLock::ButtonPressAction::NoAction;
}

void NukiLockComponent::button_press_action_to_string(const NukiLock::ButtonPressAction action, char* str) {
    switch (action) {
        case NukiLock::ButtonPressAction::NoAction:
            strcpy(str, "No Action");
            break;
        case NukiLock::ButtonPressAction::Intelligent:
            strcpy(str, "Intelligent");
            break;
        case NukiLock::ButtonPressAction::Unlock:
            strcpy(str, "Unlock");
            break;
        case NukiLock::ButtonPressAction::Lock:
            strcpy(str, "Lock");
            break;
        case NukiLock::ButtonPressAction::Unlatch:
            strcpy(str, "Unlatch");
            break;
        case NukiLock::ButtonPressAction::LockNgo:
            strcpy(str, "Lock n Go");
            break;
        case NukiLock::ButtonPressAction::ShowStatus:
            strcpy(str, "Show Status");
            break;
        default:
            strcpy(str, "No Action");
            break;
    }
}

void NukiLockComponent::battery_type_to_string(const Nuki::BatteryType battery_type, char* str) {
    switch (battery_type) {
        case Nuki::BatteryType::Alkali:
            strcpy(str, "Alkali");
            break;
        case Nuki::BatteryType::Accumulators:
            strcpy(str, "Accumulators");
            break;
        case Nuki::BatteryType::Lithium:
            strcpy(str, "Lithium");
            break;
        default:
            strcpy(str, "undefined");
            break;
    }
}

Nuki::BatteryType NukiLockComponent::battery_type_to_enum(const char* str) {
    if(strcmp(str, "Alkali") == 0) {
        return Nuki::BatteryType::Alkali;
    } else if(strcmp(str, "Accumulators") == 0) {
        return Nuki::BatteryType::Accumulators;
    } else if(strcmp(str, "Lithium") == 0) {
        return Nuki::BatteryType::Lithium;
    }
    return (Nuki::BatteryType)0xff;
}

void NukiLockComponent::homekit_status_to_string(const int status, char* str) {
    switch (status) {
        case 0:
            strcpy(str, "Not Available");
            break;
        case 1:
            strcpy(str, "Disabled");
            break;
        case 2:
            strcpy(str, "Enabled");
            break;
        case 3:
            strcpy(str, "Enabled & Paired");
            break;
        default:
            strcpy(str, "undefined");
            break;
    }
}

void NukiLockComponent::motor_speed_to_string(const NukiLock::MotorSpeed speed, char* str) {
    switch (speed) {
        case NukiLock::MotorSpeed::Standard:
            strcpy(str, "Standard");
            break;
        case NukiLock::MotorSpeed::Insane:
            strcpy(str, "Insane");
            break;
        case NukiLock::MotorSpeed::Gentle:
            strcpy(str, "Gentle");
            break;
        default:
            strcpy(str, "undefined");
            break;
    }
}

NukiLock::MotorSpeed NukiLockComponent::motor_speed_to_enum(const char* str) {
    if(strcmp(str, "Standard") == 0) {
        return NukiLock::MotorSpeed::Standard;
    } else if(strcmp(str, "Insane") == 0) {
        return NukiLock::MotorSpeed::Insane;
    } else if(strcmp(str, "Gentle") == 0) {
        return NukiLock::MotorSpeed::Gentle;
    }
    return NukiLock::MotorSpeed::Standard;
}

uint8_t NukiLockComponent::fob_action_to_int(const char *str) {
    if(strcmp(str, "No Action") == 0) {
        return 0;
    } else if(strcmp(str, "Unlock") == 0) {
        return 1;
    } else if(strcmp(str, "Lock") == 0) {
        return 2;
    } else if(strcmp(str, "Lock n Go") == 0) {
        return 3;
    } else if(strcmp(str, "Intelligent") == 0) {
        return 4;
    }
    return 99;
}

void NukiLockComponent::fob_action_to_string(const int action, char* str) {
    switch (action) {
        case 0:
            strcpy(str, "No Action");
            break;
        case 1:
            strcpy(str, "Unlock");
            break;
        case 2:
            strcpy(str, "Lock");
            break;
        case 3:
            strcpy(str, "Lock n Go");
            break;
        case 4:
            strcpy(str, "Intelligent");
            break;
        default:
            strcpy(str, "No Action");
            break;
    }
}

Nuki::TimeZoneId NukiLockComponent::timezone_to_enum(const char *str) {
    if(strcmp(str, "Africa/Cairo") == 0) {
        return Nuki::TimeZoneId::Africa_Cairo;
    } else if(strcmp(str, "Africa/Lagos") == 0) {
        return Nuki::TimeZoneId::Africa_Lagos;
    } else if(strcmp(str, "Africa/Maputo") == 0) {
        return Nuki::TimeZoneId::Africa_Maputo;
    } else if(strcmp(str, "Africa/Nairobi") == 0) {
        return Nuki::TimeZoneId::Africa_Nairobi;
    } else if(strcmp(str, "America/Anchorage") == 0) {
        return Nuki::TimeZoneId::America_Anchorage;
    } else if(strcmp(str, "America/Argentina/Buenos_Aires") == 0) {
        return Nuki::TimeZoneId::America_Argentina_Buenos_Aires;
    } else if(strcmp(str, "America/Chicago") == 0) {
        return Nuki::TimeZoneId::America_Chicago;
    } else if(strcmp(str, "America/Denver") == 0) {
        return Nuki::TimeZoneId::America_Denver;
    } else if(strcmp(str, "America/Halifax") == 0) {
        return Nuki::TimeZoneId::America_Halifax;
    } else if(strcmp(str, "America/Los_Angeles") == 0) {
        return Nuki::TimeZoneId::America_Los_Angeles;
    } else if(strcmp(str, "America/Manaus") == 0) {
        return Nuki::TimeZoneId::America_Manaus;
    } else if(strcmp(str, "America/Mexico_City") == 0) {
        return Nuki::TimeZoneId::America_Mexico_City;
    } else if(strcmp(str, "America/New_York") == 0) {
        return Nuki::TimeZoneId::America_New_York;
    } else if(strcmp(str, "America/Phoenix") == 0) {
        return Nuki::TimeZoneId::America_Phoenix;
    } else if(strcmp(str, "America/Regina") == 0) {
        return Nuki::TimeZoneId::America_Regina;
    } else if(strcmp(str, "America/Santiago") == 0) {
        return Nuki::TimeZoneId::America_Santiago;
    } else if(strcmp(str, "America/Sao_Paulo") == 0) {
        return Nuki::TimeZoneId::America_Sao_Paulo;
    } else if(strcmp(str, "America/St_Johns") == 0) {
        return Nuki::TimeZoneId::America_St_Johns;
    } else if(strcmp(str, "Asia/Bangkok") == 0) {
        return Nuki::TimeZoneId::Asia_Bangkok;
    } else if(strcmp(str, "Asia/Dubai") == 0) {
        return Nuki::TimeZoneId::Asia_Dubai;
    } else if(strcmp(str, "Asia/Hong_Kong") == 0) {
        return Nuki::TimeZoneId::Asia_Hong_Kong;
    } else if(strcmp(str, "Asia/Jerusalem") == 0) {
        return Nuki::TimeZoneId::Asia_Jerusalem;
    } else if(strcmp(str, "Asia/Karachi") == 0) {
        return Nuki::TimeZoneId::Asia_Karachi;
    } else if(strcmp(str, "Asia/Kathmandu") == 0) {
        return Nuki::TimeZoneId::Asia_Kathmandu;
    } else if(strcmp(str, "Asia/Kolkata") == 0) {
        return Nuki::TimeZoneId::Asia_Kolkata;
    } else if(strcmp(str, "Asia/Riyadh") == 0) {
        return Nuki::TimeZoneId::Asia_Riyadh;
    } else if(strcmp(str, "Asia/Seoul") == 0) {
        return Nuki::TimeZoneId::Asia_Seoul;
    } else if(strcmp(str, "Asia/Shanghai") == 0) {
        return Nuki::TimeZoneId::Asia_Shanghai;
    } else if(strcmp(str, "Asia/Tehran") == 0) {
        return Nuki::TimeZoneId::Asia_Tehran;
    } else if(strcmp(str, "Asia/Tokyo") == 0) {
        return Nuki::TimeZoneId::Asia_Tokyo;
    } else if(strcmp(str, "Asia/Yangon") == 0) {
        return Nuki::TimeZoneId::Asia_Yangon;
    } else if(strcmp(str, "Australia/Adelaide") == 0) {
        return Nuki::TimeZoneId::Australia_Adelaide;
    } else if(strcmp(str, "Australia/Brisbane") == 0) {
        return Nuki::TimeZoneId::Australia_Brisbane;
    } else if(strcmp(str, "Australia/Darwin") == 0) {
        return Nuki::TimeZoneId::Australia_Darwin;
    } else if(strcmp(str, "Australia/Hobart") == 0) {
        return Nuki::TimeZoneId::Australia_Hobart;
    } else if(strcmp(str, "Australia/Perth") == 0) {
        return Nuki::TimeZoneId::Australia_Perth;
    } else if(strcmp(str, "Australia/Sydney") == 0) {
        return Nuki::TimeZoneId::Australia_Sydney;
    } else if(strcmp(str, "Europe/Berlin") == 0) {
        return Nuki::TimeZoneId::Europe_Berlin;
    } else if(strcmp(str, "Europe/Helsinki") == 0) {
        return Nuki::TimeZoneId::Europe_Helsinki;
    } else if(strcmp(str, "Europe/Istanbul") == 0) {
        return Nuki::TimeZoneId::Europe_Istanbul;
    } else if(strcmp(str, "Europe/London") == 0) {
        return Nuki::TimeZoneId::Europe_London;
    } else if(strcmp(str, "Europe/Moscow") == 0) {
        return Nuki::TimeZoneId::Europe_Moscow;
    } else if(strcmp(str, "Pacific/Auckland") == 0) {
        return Nuki::TimeZoneId::Pacific_Auckland;
    } else if(strcmp(str, "Pacific/Guam") == 0) {
        return Nuki::TimeZoneId::Pacific_Guam;
    } else if(strcmp(str, "Pacific/Honolulu") == 0) {
        return Nuki::TimeZoneId::Pacific_Honolulu;
    } else if(strcmp(str, "Pacific/Pago_Pago") == 0) {
        return Nuki::TimeZoneId::Pacific_Pago_Pago;
    } else if(strcmp(str, "None") == 0) {
        return Nuki::TimeZoneId::None;
    }
    return (Nuki::TimeZoneId)0xff;
}

void NukiLockComponent::timezone_to_string(const Nuki::TimeZoneId timeZoneId, char* str) {
    switch (timeZoneId) {
        case Nuki::TimeZoneId::Africa_Cairo:
            strcpy(str, "Africa/Cairo");
            break;
        case Nuki::TimeZoneId::Africa_Lagos:
            strcpy(str, "Africa/Lagos");
            break;
        case Nuki::TimeZoneId::Africa_Maputo:
            strcpy(str, "Africa/Maputo");
            break;
        case Nuki::TimeZoneId::Africa_Nairobi:
            strcpy(str, "Africa/Nairobi");
            break;
        case Nuki::TimeZoneId::America_Anchorage:
            strcpy(str, "America/Anchorage");
            break;
        case Nuki::TimeZoneId::America_Argentina_Buenos_Aires:
            strcpy(str, "America/Argentina/Buenos_Aires");
            break;
        case Nuki::TimeZoneId::America_Chicago:
            strcpy(str, "America/Chicago");
            break;
        case Nuki::TimeZoneId::America_Denver:
            strcpy(str, "America/Denver");
            break;
        case Nuki::TimeZoneId::America_Halifax:
            strcpy(str, "America/Halifax");
            break;
        case Nuki::TimeZoneId::America_Los_Angeles:
            strcpy(str, "America/Los_Angeles");
            break;
        case Nuki::TimeZoneId::America_Manaus:
            strcpy(str, "America/Manaus");
            break;
        case Nuki::TimeZoneId::America_Mexico_City:
            strcpy(str, "America/Mexico_City");
            break;
        case Nuki::TimeZoneId::America_New_York:
            strcpy(str, "America/New_York");
            break;
        case Nuki::TimeZoneId::America_Phoenix:
            strcpy(str, "America/Phoenix");
            break;
        case Nuki::TimeZoneId::America_Regina:
            strcpy(str, "America/Regina");
            break;
        case Nuki::TimeZoneId::America_Santiago:
            strcpy(str, "America/Santiago");
            break;
        case Nuki::TimeZoneId::America_Sao_Paulo:
            strcpy(str, "America/Sao_Paulo");
            break;
        case Nuki::TimeZoneId::America_St_Johns:
            strcpy(str, "America/St_Johns");
            break;
        case Nuki::TimeZoneId::Asia_Bangkok:
            strcpy(str, "Asia/Bangkok");
            break;
        case Nuki::TimeZoneId::Asia_Dubai:
            strcpy(str, "Asia/Dubai");
            break;
        case Nuki::TimeZoneId::Asia_Hong_Kong:
            strcpy(str, "Asia/Hong_Kong");
            break;
        case Nuki::TimeZoneId::Asia_Jerusalem:
            strcpy(str, "Asia/Jerusalem");
            break;
        case Nuki::TimeZoneId::Asia_Karachi:
            strcpy(str, "Asia/Karachi");
            break;
        case Nuki::TimeZoneId::Asia_Kathmandu:
            strcpy(str, "Asia/Kathmandu");
            break;
        case Nuki::TimeZoneId::Asia_Kolkata:
            strcpy(str, "Asia/Kolkata");
            break;
        case Nuki::TimeZoneId::Asia_Riyadh:
            strcpy(str, "Asia/Riyadh");
            break;
        case Nuki::TimeZoneId::Asia_Seoul:
            strcpy(str, "Asia/Seoul");
            break;
        case Nuki::TimeZoneId::Asia_Shanghai:
            strcpy(str, "Asia/Shanghai");
            break;
        case Nuki::TimeZoneId::Asia_Tehran:
            strcpy(str, "Asia/Tehran");
            break;
        case Nuki::TimeZoneId::Asia_Tokyo:
            strcpy(str, "Asia/Tokyo");
            break;
        case Nuki::TimeZoneId::Asia_Yangon:
            strcpy(str, "Asia/Yangon");
            break;
        case Nuki::TimeZoneId::Australia_Adelaide:
            strcpy(str, "Australia/Adelaide");
            break;
        case Nuki::TimeZoneId::Australia_Brisbane:
            strcpy(str, "Australia/Brisbane");
            break;
        case Nuki::TimeZoneId::Australia_Darwin:
            strcpy(str, "Australia/Darwin");
            break;
        case Nuki::TimeZoneId::Australia_Hobart:
            strcpy(str, "Australia/Hobart");
            break;
        case Nuki::TimeZoneId::Australia_Perth:
            strcpy(str, "Australia/Perth");
            break;
        case Nuki::TimeZoneId::Australia_Sydney:
            strcpy(str, "Australia/Sydney");
            break;
        case Nuki::TimeZoneId::Europe_Berlin:
            strcpy(str, "Europe/Berlin");
            break;
        case Nuki::TimeZoneId::Europe_Helsinki:
            strcpy(str, "Europe/Helsinki");
            break;
        case Nuki::TimeZoneId::Europe_Istanbul:
            strcpy(str, "Europe/Istanbul");
            break;
        case Nuki::TimeZoneId::Europe_London:
            strcpy(str, "Europe/London");
            break;
        case Nuki::TimeZoneId::Europe_Moscow:
            strcpy(str, "Europe/Moscow");
            break;
        case Nuki::TimeZoneId::Pacific_Auckland:
            strcpy(str, "Pacific/Auckland");
            break;
        case Nuki::TimeZoneId::Pacific_Guam:
            strcpy(str, "Pacific/Guam");
            break;
        case Nuki::TimeZoneId::Pacific_Honolulu:
            strcpy(str, "Pacific/Honolulu");
            break;
        case Nuki::TimeZoneId::Pacific_Pago_Pago:
            strcpy(str, "Pacific/Pago_Pago");
            break;
        case Nuki::TimeZoneId::None:
            strcpy(str, "None");
            break;
        default:
            strcpy(str, "None");
            break;
    }
}

Nuki::AdvertisingMode NukiLockComponent::advertising_mode_to_enum(const char *str) {
    if(strcmp(str, "Automatic") == 0) {
        return Nuki::AdvertisingMode::Automatic;
    } else if(strcmp(str, "Normal") == 0) {
        return Nuki::AdvertisingMode::Normal;
    } else if(strcmp(str, "Slow") == 0) {
        return Nuki::AdvertisingMode::Slow;
    } else if(strcmp(str, "Slowest") == 0) {
        return Nuki::AdvertisingMode::Slowest;
    }
    return (Nuki::AdvertisingMode)0xff;
}

void NukiLockComponent::advertising_mode_to_string(const Nuki::AdvertisingMode mode, char* str) {
    switch (mode) {
        case Nuki::AdvertisingMode::Automatic:
            strcpy(str, "Automatic");
            break;
        case Nuki::AdvertisingMode::Normal:
            strcpy(str, "Normal");
            break;
        case Nuki::AdvertisingMode::Slow:
            strcpy(str, "Slow");
            break;
        case Nuki::AdvertisingMode::Slowest:
            strcpy(str, "Slowest");
            break;
        default:
            strcpy(str, "Normal");
            break;
    }
}

void NukiLockComponent::pin_state_to_string(const PinState value, char* str)
{
    switch(value)
    {
        case PinState::NotSet:
            strcpy(str, "Not set");
            break;
        case PinState::Set:
            strcpy(str, "Validation pending");
            break;
        case PinState::Valid:
            strcpy(str, "Valid");
            break;
        case PinState::Invalid:
            strcpy(str, "Invalid");
            break;
        default:
            strcpy(str, "Unknown");
            break;
    }
}


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
        this->connection_state_ = true;

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

        this->publish_state(this->nuki_to_lock_state(this->retrieved_key_turner_state_.lockState));

        #ifdef USE_BINARY_SENSOR
        if (this->is_connected_binary_sensor_ != nullptr) {
            this->is_connected_binary_sensor_->publish_state(this->connection_state_);
        }
        
        if (this->battery_critical_binary_sensor_ != nullptr) {
            this->battery_critical_binary_sensor_->publish_state(this->nuki_lock_.isBatteryCritical());
        }

        // If pin needs validation, validate now
        if(this->pin_state_ == PinState::Set) {
            validatePin();
        }
        
        if (this->door_sensor_binary_sensor_ != nullptr) {
            Nuki::DoorSensorState door_sensor_state = this->retrieved_key_turner_state_.doorSensorState;
            if(door_sensor_state != Nuki::DoorSensorState::Unavailable) {
                this->door_sensor_binary_sensor_->publish_state(this->nuki_doorsensor_to_binary(door_sensor_state));
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
            this->connection_state_ = false;

            // Publish failed state only when having too many consecutive errors
            this->publish_state(lock::LOCK_STATE_NONE);

            #ifdef USE_BINARY_SENSOR
            if (this->is_connected_binary_sensor_ != nullptr) {
                this->is_connected_binary_sensor_->publish_state(this->connection_state_);
            }
            #endif
        }
    }
}

void NukiLockComponent::update_config() {
    this->config_update_ = false;

    char str[50] = {0};

    NukiLock::Config config;
    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestConfig(&config);
    NukiLock::cmdResultToString(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestConfig has resulted in %s (%d)", str, conf_req_result);

        keypad_paired_ = config.hasKeypad || config.hasKeypadV2;

        #ifdef USE_SWITCH
        if (this->auto_unlatch_enabled_switch_ != nullptr) {
            this->auto_unlatch_enabled_switch_->publish_state(config.autoUnlatch);
        }
        
        if (this->button_enabled_switch_ != nullptr) {
            this->button_enabled_switch_->publish_state(config.buttonEnabled);
        }
        
        if (this->led_enabled_switch_ != nullptr) {
            this->led_enabled_switch_->publish_state(config.ledEnabled);
        }
        
        if (this->single_lock_enabled_switch_ != nullptr) {
            this->single_lock_enabled_switch_->publish_state(config.singleLock);
        }
        
        if (this->dst_mode_enabled_switch_ != nullptr) {
            this->dst_mode_enabled_switch_->publish_state(config.dstMode);
        }
        #endif
        #ifdef USE_NUMBER
        if (this->led_brightness_number_ != nullptr) {
            this->led_brightness_number_->publish_state(config.ledBrightness);
        }
        
        if (this->timezone_offset_number_ != nullptr) {
            this->timezone_offset_number_->publish_state(config.timeZoneOffset);
        }
        #endif
        #ifdef USE_SELECT
        if (this->fob_action_1_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->fob_action_to_string(config.fobAction1, str);
            this->fob_action_1_select_->publish_state(str);
        }
        
        if (this->fob_action_2_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->fob_action_to_string(config.fobAction2, str);
            this->fob_action_2_select_->publish_state(str);
        }
        
        if (this->fob_action_3_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->fob_action_to_string(config.fobAction3, str);
            this->fob_action_3_select_->publish_state(str);
        }
        
        if (this->timezone_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->timezone_to_string(config.timeZoneId, str);
            this->timezone_select_->publish_state(str);
        }
        
        if (this->advertising_mode_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->advertising_mode_to_string(config.advertisingMode, str);
            this->advertising_mode_select_->publish_state(str);
        }
        #endif
        
        ESP_LOGD(TAG, "Device Type: %i", (config.deviceType == 255 ? 0 : config.deviceType));
        ESP_LOGD(TAG, "Product Variant: %i", (config.productVariant == 255 ? 0 : config.productVariant));

        ESP_LOGD(TAG, "Firmware: %i.%i.%i", config.firmwareVersion[0], config.firmwareVersion[1], config.firmwareVersion[2]);
        ESP_LOGD(TAG, "Hardware: %i.%i", config.hardwareRevision[0], config.hardwareRevision[1]);

        ESP_LOGD(TAG, "Has Wifi: %s", YESNO(config.capabilities == 255 ? 0 : config.capabilities & 1));
        ESP_LOGD(TAG, "Has Thread: %s", YESNO(config.capabilities == 255 ? 0 : ((config.capabilities & 2) != 0 ? 1 : 0)));

        ESP_LOGD(TAG, "Matter Status: %i", (config.matterStatus == 255 ? 0 : config.matterStatus));
        memset(str, 0, sizeof(str));
        this->homekit_status_to_string(config.homeKitStatus, str);
        ESP_LOGD(TAG, "Homekit Status: %s", str);
    } else {
        ESP_LOGE(TAG, "requestConfig has resulted in %s (%d)", str, conf_req_result);
        this->config_update_ = true;
    }
}

void NukiLockComponent::update_advanced_config() {
    this->advanced_config_update_ = false;

    char str[50] = {0};

    NukiLock::AdvancedConfig advanced_config;
    Nuki::CmdResult conf_req_result = this->nuki_lock_.requestAdvancedConfig(&advanced_config);
    NukiLock::cmdResultToString(conf_req_result, str);

    App.feed_wdt();

    if (conf_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "requestAdvancedConfig has resulted in %s (%d)", str, conf_req_result);

        #ifdef USE_SWITCH
        if (this->nightmode_enabled_switch_ != nullptr) {
            this->nightmode_enabled_switch_->publish_state(advanced_config.nightModeEnabled);
        }

        if (this->night_mode_auto_lock_enabled_switch_ != nullptr) {
            this->night_mode_auto_lock_enabled_switch_->publish_state(advanced_config.nightModeAutoLockEnabled);
        }

        if (this->night_mode_auto_unlock_disabled_switch_ != nullptr) {
            this->night_mode_auto_unlock_disabled_switch_->publish_state(advanced_config.nightModeAutoUnlockDisabled);
        }

        if (this->night_mode_immediate_lock_on_start_switch_ != nullptr) {
            this->night_mode_immediate_lock_on_start_switch_->publish_state(advanced_config.nightModeImmediateLockOnStart);
        }

        if (this->auto_lock_enabled_switch_ != nullptr) {
            this->auto_lock_enabled_switch_->publish_state(advanced_config.autoLockEnabled);
        }

        if (this->auto_unlock_disabled_switch_ != nullptr) {
            this->auto_unlock_disabled_switch_->publish_state(advanced_config.autoUnLockDisabled);
        }

        if (this->immediate_auto_lock_enabled_switch_ != nullptr) {
            this->immediate_auto_lock_enabled_switch_->publish_state(advanced_config.immediateAutoLockEnabled);
        }

        if (this->auto_update_enabled_switch_ != nullptr) {
            this->auto_update_enabled_switch_->publish_state(advanced_config.autoUpdateEnabled);
        }

        // Gen 1-4 only
        if (!this->nuki_lock_.isLockUltra() && this->auto_battery_type_detection_enabled_switch_ != nullptr) {
            this->auto_battery_type_detection_enabled_switch_->publish_state(advanced_config.automaticBatteryTypeDetection);
        }

        // Ultra only
        if (this->nuki_lock_.isLockUltra() && this->slow_speed_during_night_mode_enabled_switch_ != nullptr) {
            this->slow_speed_during_night_mode_enabled_switch_->publish_state(advanced_config.enableSlowSpeedDuringNightMode);
        }
        #endif

        #ifdef USE_NUMBER
        if (this->lock_n_go_timeout_number_ != nullptr) {
            this->lock_n_go_timeout_number_->publish_state(advanced_config.lockNgoTimeout);
        }
        #endif

        #ifdef USE_SELECT
        if (this->single_button_press_action_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->button_press_action_to_string(advanced_config.singleButtonPressAction, str);
            this->single_button_press_action_select_->publish_state(str);
        }

        if (this->double_button_press_action_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->button_press_action_to_string(advanced_config.doubleButtonPressAction, str);
            this->double_button_press_action_select_->publish_state(str);
        }

        // Gen 1-4 only
        if (!this->nuki_lock_.isLockUltra() && this->battery_type_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->battery_type_to_string(advanced_config.batteryType, str);
            this->battery_type_select_->publish_state(str);
        }

        // Ultra
        if (this->nuki_lock_.isLockUltra() && this->motor_speed_select_ != nullptr) {
            memset(str, 0, sizeof(str));
            this->motor_speed_to_string(advanced_config.motorSpeed, str);
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
    this->cancel_timeout("wait_for_auth_data");

    if(!is_pin_valid()) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    Nuki::CmdResult auth_data_req_result = this->nuki_lock_.retrieveAuthorizationEntries(0, MAX_AUTH_DATA_ENTRIES);
    char auth_data_req_result_as_string[30] = {0};
    NukiLock::cmdResultToString(auth_data_req_result, auth_data_req_result_as_string);

    App.feed_wdt();

    if (auth_data_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "retrieveAuthorizationEntries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);

        this->set_timeout("wait_for_auth_data", 5000, [this]() {

            std::list<NukiLock::AuthorizationEntry> authEntries;
            this->nuki_lock_.getAuthorizationEntries(&authEntries);
    
            if (!authEntries.empty()) {
                ESP_LOGD(TAG, "Authorization Entry Count: %d", authEntries.size());

                authEntries.sort([](const NukiLock::AuthorizationEntry& a, const NukiLock::AuthorizationEntry& b) {
                    return a.authId < b.authId;
                });
        
                if (authEntries.size() > MAX_AUTH_DATA_ENTRIES) {
                    authEntries.resize(MAX_AUTH_DATA_ENTRIES);
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
            } else {
                ESP_LOGW(TAG, "No auth entries!");
            }
        });
    } else {
        ESP_LOGE(TAG, "retrieveAuthorizationEntries has resulted in %s (%d)", auth_data_req_result_as_string, auth_data_req_result);
        this->auth_data_update_ = true;
    }
}

void NukiLockComponent::update_event_logs() {
    this->event_log_update_ = false;
    this->cancel_timeout("wait_for_log_entries");

    if(!is_pin_valid()) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    Nuki::CmdResult event_log_req_result = this->nuki_lock_.retrieveLogEntries(0, MAX_EVENT_LOG_ENTRIES, 1, false);
    char event_log_req_result_as_string[30] = {0};
    NukiLock::cmdResultToString(event_log_req_result, event_log_req_result_as_string);

    App.feed_wdt();

    if (event_log_req_result == Nuki::CmdResult::Success) {
        ESP_LOGD(TAG, "retrieveLogEntries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);

        this->set_timeout("wait_for_log_entries", 5000, [this]() {
            std::list<NukiLock::LogEntry> log;
            this->nuki_lock_.getLogEntries(&log);

            App.feed_wdt();

            if (!log.empty()) {
                ESP_LOGD(TAG, "Log Entry Count: %d", log.size());

                if (log.size() > MAX_EVENT_LOG_ENTRIES) {
                    log.resize(MAX_EVENT_LOG_ENTRIES);
                }
        
                log.sort([](const NukiLock::LogEntry& a, const NukiLock::LogEntry& b) {
                    return a.index < b.index;
                });

                this->process_log_entries(log);
            } else {
                ESP_LOGW(TAG, "No log entries!");
            }
        });
    } else {
        ESP_LOGE(TAG, "retrieveLogEntries has resulted in %s (%d)", event_log_req_result_as_string, event_log_req_result);
        this->event_log_update_ = true;
    }
}

void NukiLockComponent::process_log_entries(const std::list<NukiLock::LogEntry>& log_entries) {
    ESP_LOGD(TAG, "Process Event Log Entries");

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

    // Mark as set but needs validation
    this->pin_state_ = PinState::Set;
    this->save_settings();
    this->publish_pin_state();

    // Save pin
    const bool result = this->nuki_lock_.isLockUltra() ? this->nuki_lock_.saveUltraPincode(pin_to_use) : this->nuki_lock_.saveSecurityPincode(static_cast<uint16_t>(pin_to_use));

    if (result) {
        ESP_LOGI(TAG, "Successfully saved security pin");
    } else {
        ESP_LOGE(TAG, "Failed to save security pin");
        this->pin_state_ = PinState::Invalid;
        this->save_settings();
        this->publish_pin_state();
        return;
    }

    // Validate pin if lock is paired and connected
    if (this->nuki_lock_.isPairedWithLock() && this->connection_state_) {
        ESP_LOGD(TAG, "Validating new security pin");
        this->validatePin();
    } else {
        ESP_LOGD(TAG, "Skipping pin validation (not paired or not connected)");
    }
}

void NukiLockComponent::validatePin()
{
    ESP_LOGD(TAG, "Check if pin is valid and save state");

    cancel_retry("validate_pin");

    if(this->pin_state_ == PinState::NotSet) {
        ESP_LOGD(TAG, "Pin is not set, no validation needed!");
        return;
    }

    this->set_retry(
        "validate_pin", 100, 4,
        [this](const uint8_t remaining_attempts) {

            ESP_LOGD(TAG, "verifySecurityPin attempts left: %d", remaining_attempts);

            Nuki::CmdResult pin_result = this->nuki_lock_.verifySecurityPin();

            App.feed_wdt();

            if(pin_result == Nuki::CmdResult::Success) {
                ESP_LOGI(TAG, "Nuki Lock PIN is valid");

                if(this->pin_state_ != PinState::Valid) {
                    this->pin_state_ = PinState::Valid;
                    this->save_settings();
                    this->publish_pin_state();
                }
                return RetryResult::DONE;

            } else if (remaining_attempts == 0) {
                ESP_LOGD(TAG, "Nuki Lock PIN is invalid or not set");

                if(this->pin_state_ != PinState::Invalid) {
                    this->pin_state_ = PinState::Invalid;
                    this->save_settings();
                    this->publish_pin_state();
                }
            }
            ESP_LOGW(TAG, "verifySecurityPin: result %d, retry...", pin_result);
            return RetryResult::RETRY;
        },
        1.0f
    );
}

bool NukiLockComponent::is_pin_valid() {
    return this->pin_state_ == PinState::Valid;
}

void NukiLockComponent::setup() {
    ESP_LOGCONFIG(TAG, "Running setup");

    // Increase Watchdog Timeout
    // Fixes Pairing Crash
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 15000,
        .trigger_panic = false
    }; 
    esp_task_wdt_reconfigure(&wdt_config);

    // Restore settings from flash
    this->pref_ = global_preferences->make_preference<NukiLockSettings>(global_nuki_lock_id);

    NukiLockSettings recovered;
    if (!this->pref_.load(&recovered)) {
        recovered = {0};
    }

    this->pin_state_ = recovered.pin_state;
    this->security_pin_ = recovered.security_pin;

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
        ESP_LOGW(TAG, "The security pin is not set. The security pin is crucial to pair a Smart Lock Ultra.");
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

    this->nuki_lock_.setDebugConnect(false);
    this->nuki_lock_.setDebugCommunication(false);
    this->nuki_lock_.setDebugReadableData(false);
    this->nuki_lock_.setDebugHexData(false);
    this->nuki_lock_.setDebugCommand(false);

    this->nuki_lock_.initialize();
    this->nuki_lock_.registerBleScanner(&this->scanner_);
    this->nuki_lock_.setConnectTimeout(BLE_CONNECT_TIMEOUT_SEC);
    this->nuki_lock_.setConnectRetries(BLE_CONNECT_RETRIES);
    this->nuki_lock_.setDisconnectTimeout(BLE_DISCONNECT_TIMEOUT);
    this->nuki_lock_.setGeneralTimeout(this->ble_general_timeout_ * 1000);
    this->nuki_lock_.setCommandTimeout(this->ble_command_timeout_ * 1000);
    
    App.feed_wdt();

    if (this->nuki_lock_.isPairedWithLock()) {
        this->status_update_ = true;

        // First boot: Request config and auth data
        this->config_update_ = true;
        this->advanced_config_update_ = true;
        if (this->send_events_) {
            this->auth_data_update_ = true;
            this->event_log_update_ = true;
        }

        const char* pairing_type = this->pairing_as_app_ ? "App" : "Bridge";
        const char* lock_type = this->nuki_lock_.isLockUltra() ? "Ultra / Go / 5th Gen" : "1st - 4th Gen";
        ESP_LOGI(TAG, "This component is already paired as %s with a %s smart lock!", pairing_type, lock_type);

        #ifdef USE_BINARY_SENSOR
        if (this->is_paired_binary_sensor_ != nullptr)
        {
            this->is_paired_binary_sensor_->publish_initial_state(true);
        }
        #endif

        this->validatePin();

        this->setup_intervals();

    } else {
        ESP_LOGI(TAG, "This component is not paired yet. Enable the pairing mode to pair with your smart lock.");
        #ifdef USE_BINARY_SENSOR
        if (this->is_paired_binary_sensor_ != nullptr)
        {
            this->is_paired_binary_sensor_->publish_initial_state(false);
        }     
        #endif
    }

    this->publish_pin_state();

    this->publish_state(lock::LOCK_STATE_NONE);

    #ifdef USE_API
        #ifdef USE_API_SERVICES
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
}

void NukiLockComponent::publish_pin_state() {
    #ifdef USE_TEXT_SENSOR
    char pin_state_as_string[20] = {0};
    this->pin_state_to_string(this->pin_state_, pin_state_as_string);

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

void NukiLockComponent::update() {
    // Check for new advertisements
    this->scanner_.update();
    App.feed_wdt();
    delay(20);

    /*int64_t ts = millis();
    int64_t last_received_beacon_ts = this->nuki_lock_.getLastReceivedBeaconTs();

    if(ts > 60000 && last_received_beacon_ts > 0 && (ts - last_received_beacon_ts > 60 * 1000))
    {
        ESP_LOGW(TAG, "We received no BLE beacon for %d seconds!", (ts - last_received_beacon_ts) / 1000);
    }*/

    // Terminate stale Bluetooth connections
    this->nuki_lock_.updateConnectionState();

    App.feed_wdt();

    if (millis() - last_command_executed_time_ < command_cooldown_millis) {
        // Give the lock time to terminate the previous command
        uint64_t millisSinceLastExecution = millis() - last_command_executed_time_;
        uint64_t millisLeft = (millisSinceLastExecution < command_cooldown_millis) ? command_cooldown_millis - millisSinceLastExecution : 1;
        ESP_LOGV(TAG, "Cooldown period, %dms left", millisLeft);
        return;
    }

    if (this->nuki_lock_.isPairedWithLock()) {
        #ifdef USE_BINARY_SENSOR
        if (this->is_paired_binary_sensor_ != nullptr)
        {
            this->is_paired_binary_sensor_->publish_state(true);
        } 
        #endif

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
                this->connection_state_ = false;
                
                // Publish failed state only when no attempts are left
                this->publish_state(lock::LOCK_STATE_NONE);

                #ifdef USE_BINARY_SENSOR
                if (this->is_connected_binary_sensor_ != nullptr)
                {
                    this->is_connected_binary_sensor_->publish_state(this->connection_state_);
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

        last_command_executed_time_ = millis();

    } else {
        this->connection_state_ = false;

        #ifdef USE_BINARY_SENSOR
        if (this->is_paired_binary_sensor_ != nullptr) {
            this->is_paired_binary_sensor_->publish_state(false);
        }
        if (this->is_connected_binary_sensor_ != nullptr) {
            this->is_connected_binary_sensor_->publish_state(connection_state_);
        }
        #endif

        // Pairing Mode is active
        if (this->pairing_mode_) {
            // Pair Nuki
            Nuki::AuthorizationIdType type = this->pairing_as_app_ ? Nuki::AuthorizationIdType::App : Nuki::AuthorizationIdType::Bridge;
            
            App.feed_wdt();
            
            if (this->security_pin_ != 0) {
                ESP_LOGW(TAG, "Note: Using security pin override to pair, not yaml config pin!");
            } else if(this->security_pin_config_.value_or(0) != 0) {
                ESP_LOGD(TAG, "Using security pin from yaml config to pair.");
            } else {
                ESP_LOGW(TAG, "Note: The security pin is crucial to pair a Smart Lock Ultra but is currently not set.");
            }

            ESP_LOGD(TAG, "NVS pin value (gen 1-4): %d", this->nuki_lock_.getSecurityPincode());
            ESP_LOGD(TAG, "NVS pin value (ultra/go/gen5): %d", this->nuki_lock_.getUltraPincode());

            ESP_LOGD(TAG, "ESPHome pin value (gen 1-4): %d", this->security_pin_);
            ESP_LOGD(TAG, "ESPHome pin value (ultra/go/gen5): %d", this->security_pin_config_.value_or(0));

            bool paired = this->nuki_lock_.pairNuki(type) == Nuki::PairingResult::Success;

            App.feed_wdt();

            if (paired) {
                const char* pairing_type = this->pairing_as_app_ ? "App" : "Bridge";
                const char* lock_type = this->nuki_lock_.isLockUltra() ? "Ultra / Go / 5th Gen" : "1st - 4th Gen";
                ESP_LOGI(TAG, "Successfully paired as %s with a %s smart lock!", pairing_type, lock_type);

                this->update_status();
                this->paired_callback_.call();
                this->set_pairing_mode(false);

                // Save initial security pin after pairing
                // Pairing resets the security pin
                const uint32_t pin_to_use = this->security_pin_ != 0 ? this->security_pin_ : this->security_pin_config_.value_or(0);

                if (this->security_pin_ != 0) {
                    ESP_LOGW(TAG, "Using security pin override instead of YAML config");
                }

                if (pin_to_use == 0) {
                    ESP_LOGD(TAG, "No security pin configured, skipping pin setup");
                    this->pin_state_ = PinState::NotSet;
                    this->save_settings();
                    this->publish_pin_state();
                } else if (pin_to_use > 999999) {
                    ESP_LOGE(TAG, "Invalid security pin detected! Maximum is 6 digits (999999)");
                    this->pin_state_ = PinState::Invalid;
                    this->save_settings();
                    this->publish_pin_state();
                } else if (!this->nuki_lock_.isLockUltra() && pin_to_use > 65535) {
                    ESP_LOGE(TAG, "Security pin exceeds maximum of 65535 for 1st-4th gen locks", pin_to_use);
                    this->pin_state_ = PinState::Invalid;
                    this->save_settings();
                    this->publish_pin_state();
                } else {
                    const bool result = this->nuki_lock_.isLockUltra() ? this->nuki_lock_.saveUltraPincode(pin_to_use) : this->nuki_lock_.saveSecurityPincode(static_cast<uint16_t>(pin_to_use));

                    if (result) {
                        ESP_LOGI(TAG, "Successfully set security pin");
                    } else {
                        ESP_LOGE(TAG, "Failed to set security pin");
                    }

                    this->save_settings();
                    this->publish_pin_state();
                    this->validatePin();
                }

                this->setup_intervals();
            }

            #ifdef USE_BINARY_SENSOR
            if (this->is_paired_binary_sensor_ != nullptr) {
                this->is_paired_binary_sensor_->publish_state(paired);
            }
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
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(!is_pin_valid()) {
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
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(!is_pin_valid()) {
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
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(!is_pin_valid()) {
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
    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(!is_pin_valid()) {
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
        ESP_LOGCONFIG(TAG, "  Event: Disabled");
    }
    #else
    ESP_LOGCONFIG(TAG, "  Event: Disabled");
    #endif

    ESP_LOGCONFIG(TAG, "  Pairing Identity: %s",this->pairing_as_app_ ? "App" : "Bridge");

    ESP_LOGCONFIG(TAG, "  Pairing mode timeout: %us", this->pairing_mode_timeout_);
    ESP_LOGCONFIG(TAG, "  Configuration query interval: %us", this->query_interval_config_);
    ESP_LOGCONFIG(TAG, "  Auth Data query interval: %us", this->query_interval_auth_data_);
    ESP_LOGCONFIG(TAG, "  BLE general timeout: %us", this->ble_general_timeout_);
    ESP_LOGCONFIG(TAG, "  BLE command timeout: %us", this->ble_command_timeout_);

    char pin_state_as_string[30] = {0};
    this->pin_state_to_string(this->pin_state_, pin_state_as_string);
    ESP_LOGCONFIG(TAG, "  Last known security pin state: %s", pin_state_as_string);

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
    LOG_TEXT_SENSOR(TAG, "Last Lock Action", this->last_lock_action_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Last Lock Action Trigger", this->last_lock_action_trigger_text_sensor_);
    LOG_TEXT_SENSOR(TAG, "Pin Status", this->pin_state_text_sensor_);
    #endif
    #ifdef USE_SENSOR
    LOG_SENSOR(TAG, "Battery Level", this->battery_level_sensor_);
    LOG_SENSOR(TAG, "Bluetooth Signal", this->bt_signal_sensor_);
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
    LOG_SWITCH(TAG, "Single Lock Enabled", this->single_lock_enabled_switch_);
    LOG_SWITCH(TAG, "DST Mode Enabled", this->dst_mode_enabled_switch_);
    LOG_SWITCH(TAG, "Slow Speed During Night Mode Enabled", this->slow_speed_during_night_mode_enabled_switch_);
    #endif
    #ifdef USE_NUMBER
    LOG_NUMBER(TAG, "LED Brightness", this->led_brightness_number_);
    LOG_NUMBER(TAG, "Timezone Offset", this->timezone_offset_number_);
    LOG_NUMBER(TAG, "LockNGo Timeout", this->lock_n_go_timeout_number_);
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
        ESP_LOGW(TAG, "Nuki reported invalid security pin");
        ESP_LOGD(TAG, "NVS pin value (gen 1-4): %d", this->nuki_lock_.getSecurityPincode());
        ESP_LOGD(TAG, "NVS pin value (ultra/go/gen5): %d", this->nuki_lock_.getUltraPincode());

        this->pin_state_ = PinState::Invalid;
        this->save_settings();
        this->publish_pin_state();
    } else if(event_type == Nuki::EventType::KeyTurnerStatusUpdated) {
        ESP_LOGD(TAG, "KeyTurnerStatusUpdated");

        // Request status update
        this->status_update_ = true;
    
        // Request event logs
        if (this->send_events_) {
            this->event_log_update_ = true;
        }
    } else if(event_type == Nuki::EventType::BLE_ERROR_ON_DISCONNECT) {
        ESP_LOGE(TAG, "Failed to disconnect from Nuki. Restarting ESP...");
        delay(100);  // NOLINT
        App.safe_reboot();
    }
}

void NukiLockComponent::unpair() {
    if (this->nuki_lock_.isPairedWithLock()) {
        this->nuki_lock_.unPairNuki();

        this->connection_state_ = false;

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

    cancel_timeout("pairing_mode_timeout");

    if (enabled) {
        ESP_LOGI(TAG, "Pairing Mode turned on for %d seconds", this->pairing_mode_timeout_);
        this->pairing_mode_on_callback_.call();

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

#ifdef USE_SELECT
void NukiLockComponent::set_config_select(const char* config, const char* value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if (strcmp(config, "single_button_press_action") == 0) {
        NukiLock::ButtonPressAction action = this->button_press_action_to_enum(value);
        cmd_result = this->nuki_lock_.setSingleButtonPressAction(action);
        is_advanced = true;
    } else if (strcmp(config, "double_button_press_action") == 0) {
        NukiLock::ButtonPressAction action = this->button_press_action_to_enum(value);
        cmd_result = this->nuki_lock_.setDoubleButtonPressAction(action);
        is_advanced = true;
    } else if (strcmp(config, "fob_action_1") == 0) {
        const uint8_t action = this->fob_action_to_int(value);
        if (action != 99) {
            cmd_result = this->nuki_lock_.setFobAction(1, action);
        }
    } else if (strcmp(config, "fob_action_2") == 0) {
        const uint8_t action = this->fob_action_to_int(value);
        if (action != 99) {
            cmd_result = this->nuki_lock_.setFobAction(2, action);
        }
    } else if (strcmp(config, "fob_action_3") == 0) {
        const uint8_t action = this->fob_action_to_int(value);
        if (action != 99) {
            cmd_result = this->nuki_lock_.setFobAction(3, action);
        }
    } else if (strcmp(config, "timezone") == 0) {
        Nuki::TimeZoneId tzid = this->timezone_to_enum(value);
        cmd_result = this->nuki_lock_.setTimeZoneId(tzid);
    } else if (strcmp(config, "advertising_mode") == 0) {
        Nuki::AdvertisingMode mode = this->advertising_mode_to_enum(value);
        cmd_result = this->nuki_lock_.setAdvertisingMode(mode);
    } else if (!this->nuki_lock_.isLockUltra() && strcmp(config, "battery_type") == 0) {
        Nuki::BatteryType type = this->battery_type_to_enum(value);
        cmd_result = this->nuki_lock_.setBatteryType(type);
        is_advanced = true;
    } else if (this->nuki_lock_.isLockUltra() && strcmp(config, "motor_speed") == 0) {
        NukiLock::MotorSpeed speed = this->motor_speed_to_enum(value);
        cmd_result = this->nuki_lock_.setMotorSpeed(speed);
        is_advanced = true;
    }

    if (cmd_result == Nuki::CmdResult::Success) {
        if (strcmp(config, "single_button_press_action") == 0 && this->single_button_press_action_select_ != nullptr) {
            this->single_button_press_action_select_->publish_state(value);
        } else if (strcmp(config, "double_button_press_action") == 0 && this->double_button_press_action_select_ != nullptr) {
            this->double_button_press_action_select_->publish_state(value);
        } else if (strcmp(config, "fob_action_1") == 0 && this->fob_action_1_select_ != nullptr) {
            this->fob_action_1_select_->publish_state(value);
        } else if (strcmp(config, "fob_action_2") == 0 && this->fob_action_2_select_ != nullptr) {
            this->fob_action_2_select_->publish_state(value);
        } else if (strcmp(config, "fob_action_3") == 0 && this->fob_action_3_select_ != nullptr) {
            this->fob_action_3_select_->publish_state(value);
        } else if (strcmp(config, "timezone") == 0 && this->timezone_select_ != nullptr) {
            this->timezone_select_->publish_state(value);
        } else if (strcmp(config, "advertising_mode") == 0 && this->advertising_mode_select_ != nullptr) {
            this->advertising_mode_select_->publish_state(value);
        } else if (!this->nuki_lock_.isLockUltra() && strcmp(config, "battery_type") == 0 && this->battery_type_select_ != nullptr) {
            this->battery_type_select_->publish_state(value);
        } else if (this->nuki_lock_.isLockUltra() && strcmp(config, "motor_speed") == 0 && this->motor_speed_select_ != nullptr) {
            this->motor_speed_select_->publish_state(value);
        }
        
        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    } else {
        ESP_LOGE(TAG, "Saving setting %s failed (result %d)", config, cmd_result);
    }
}
#endif

#ifdef USE_SWITCH
void NukiLockComponent::set_config_switch(const char* config, bool value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if (strcmp(config, "auto_unlatch_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableAutoUnlatch(value);
    } else if (strcmp(config, "button_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableButton(value);
    } else if (strcmp(config, "led_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableLedFlash(value);
    } else if (strcmp(config, "nightmode_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableNightMode(value);
        is_advanced = true;
    } else if (strcmp(config, "night_mode_auto_lock_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableNightModeAutoLock(value);
        is_advanced = true;
    } else if (strcmp(config, "night_mode_auto_unlock_disabled") == 0) {
        cmd_result = this->nuki_lock_.disableNightModeAutoUnlock(value);
        is_advanced = true;
    } else if (strcmp(config, "night_mode_immediate_lock_on_start") == 0) {
        cmd_result = this->nuki_lock_.enableNightModeImmediateLockOnStart(value);
        is_advanced = true;
    } else if (strcmp(config, "auto_lock_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableAutoLock(value);
        is_advanced = true;
    } else if (strcmp(config, "auto_unlock_disabled") == 0) {
        cmd_result = this->nuki_lock_.disableAutoUnlock(value);
        is_advanced = true;
    } else if (strcmp(config, "immediate_auto_lock_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableImmediateAutoLock(value);
        is_advanced = true;
    } else if (strcmp(config, "auto_update_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableAutoUpdate(value);
        is_advanced = true;
    } else if (strcmp(config, "single_lock_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableSingleLock(value);
    } else if (strcmp(config, "dst_mode_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableDst(value);
    } else if (!this->nuki_lock_.isLockUltra() && strcmp(config, "auto_battery_type_detection_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableAutoBatteryTypeDetection(value);
    } else if (this->nuki_lock_.isLockUltra() && strcmp(config, "slow_speed_during_night_mode_enabled") == 0) {
        cmd_result = this->nuki_lock_.enableSlowSpeedDuringNightMode(value);
    }

    if (cmd_result == Nuki::CmdResult::Success)
    {
        if (strcmp(config, "auto_unlatch_enabled") == 0 && this->auto_unlatch_enabled_switch_ != nullptr) {
            this->auto_unlatch_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "button_enabled") == 0 && this->button_enabled_switch_ != nullptr) {
            this->button_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "led_enabled") == 0 && this->led_enabled_switch_ != nullptr) {
            this->led_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "nightmode_enabled") == 0 && this->nightmode_enabled_switch_ != nullptr) {
            this->nightmode_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "night_mode_auto_lock_enabled") == 0 && this->night_mode_auto_lock_enabled_switch_ != nullptr) {
            this->night_mode_auto_lock_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "night_mode_auto_unlock_disabled") == 0 && this->night_mode_auto_unlock_disabled_switch_ != nullptr) {
            this->night_mode_auto_unlock_disabled_switch_->publish_state(value);
        } else if (strcmp(config, "night_mode_immediate_lock_on_start") == 0 && this->night_mode_immediate_lock_on_start_switch_ != nullptr) {
            this->night_mode_immediate_lock_on_start_switch_->publish_state(value);
        } else if (strcmp(config, "auto_lock_enabled") == 0 && this->auto_lock_enabled_switch_ != nullptr) {
            this->auto_lock_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "auto_unlock_disabled") == 0 && this->auto_unlock_disabled_switch_ != nullptr) {
            this->auto_unlock_disabled_switch_->publish_state(value);
        } else if (strcmp(config, "immediate_auto_lock_enabled") == 0 && this->immediate_auto_lock_enabled_switch_ != nullptr) {
            this->immediate_auto_lock_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "auto_update_enabled") == 0 && this->auto_update_enabled_switch_ != nullptr) {
            this->auto_update_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "single_lock_enabled") == 0 && this->single_lock_enabled_switch_ != nullptr) {
            this->single_lock_enabled_switch_->publish_state(value);
        } else if (strcmp(config, "dst_mode_enabled") == 0 && this->dst_mode_enabled_switch_ != nullptr) {
            this->dst_mode_enabled_switch_->publish_state(value);
        } else if (!this->nuki_lock_.isLockUltra() && strcmp(config, "auto_battery_type_detection_enabled") == 0 && this->auto_battery_type_detection_enabled_switch_ != nullptr) {
            this->auto_battery_type_detection_enabled_switch_->publish_state(value);
        } else if (this->nuki_lock_.isLockUltra() && strcmp(config, "slow_speed_during_night_mode_enabled") == 0 && this->slow_speed_during_night_mode_enabled_switch_ != nullptr) {
            this->slow_speed_during_night_mode_enabled_switch_->publish_state(value);
        }

        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    } else {
        ESP_LOGE(TAG, "Saving setting %s failed (result %d)", config, cmd_result);
    }
}
#endif
#ifdef USE_NUMBER
void NukiLockComponent::set_config_number(const char* config, float value) {

    Nuki::CmdResult cmd_result = (Nuki::CmdResult)-1;
    bool is_advanced = false;

    // Update Config
    if (strcmp(config, "led_brightness") == 0) {
        cmd_result = this->nuki_lock_.setLedBrightness(value);
    } else if (strcmp(config, "timezone_offset") == 0) {
        if (value >= -60 && value <= 60) {
            cmd_result = this->nuki_lock_.setTimeZoneOffset(value);
        }
    } else if (strcmp(config, "lock_n_go_timeout") == 0) {
        if (value >= 5 && value <= 60) {
            cmd_result = this->nuki_lock_.setLockNgoTimeout(value);
            is_advanced = true;
        }
    }

    if (cmd_result == Nuki::CmdResult::Success) {
        if (strcmp(config, "led_brightness") == 0 && this->led_brightness_number_ != nullptr) {
            this->led_brightness_number_->publish_state(value);
        } else if (strcmp(config, "timezone_offset") == 0 && this->timezone_offset_number_ != nullptr) {
            this->timezone_offset_number_->publish_state(value);
        } else if (strcmp(config, "lock_n_go_timeout") == 0 && this->lock_n_go_timeout_number_ != nullptr) {
            this->lock_n_go_timeout_number_->publish_state(value);
        }
        
        this->config_update_ = !is_advanced;
        this->advanced_config_update_ = is_advanced;
    } else {
        ESP_LOGE(TAG, "Saving setting %s failed (result %d)", config, cmd_result);
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
    this->parent_->set_config_select("single_button_press_action", action.c_str());
}

void NukiLockDoubleButtonPressActionSelect::control(const std::string &action) {
    this->parent_->set_config_select("double_button_press_action", action.c_str());
}

void NukiLockFobAction1Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_1", action.c_str());
}

void NukiLockFobAction2Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_2", action.c_str());
}

void NukiLockFobAction3Select::control(const std::string &action) {
    this->parent_->set_config_select("fob_action_3", action.c_str());
}

void NukiLockTimeZoneSelect::control(const std::string &zone) {
    this->parent_->set_config_select("timezone", zone.c_str());
}

void NukiLockAdvertisingModeSelect::control(const std::string &mode) {
    this->parent_->set_config_select("advertising_mode", mode.c_str());
}

void NukiLockBatteryTypeSelect::control(const std::string &mode) {
    this->parent_->set_config_select("battery_type", mode.c_str());
}

void NukiLockMotorSpeedSelect::control(const std::string &mode) {
    this->parent_->set_config_select("motor_speed", mode.c_str());
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

void NukiLockSingleLockEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("single_lock_enabled", state);
}

void NukiLockDstModeEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("dst_mode_enabled", state);
}

void NukiLockAutoBatteryTypeDetectionEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("auto_battery_type_detection_enabled", state);
}

void NukiLockSlowSpeedDuringNightModeEnabledSwitch::write_state(bool state) {
    this->parent_->set_config_switch("slow_speed_during_night_mode_enabled", state);
}
#endif
#ifdef USE_NUMBER
void NukiLockLedBrightnessNumber::control(float value) {
    this->parent_->set_config_number("led_brightness", value);
}
void NukiLockTimeZoneOffsetNumber::control(float value) {
    this->parent_->set_config_number("timezone_offset", value);
}
void NukiLockLockNGoTimeoutNumber::control(float value) {
    this->parent_->set_config_number("lock_n_go_timeout", value);
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

} //namespace nuki_lock
} //namespace esphome