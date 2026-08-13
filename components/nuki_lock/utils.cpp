#include "utils.h"
#include <cstring>

#include "esphome/components/lock/lock.h"
#include "NukiLock.h"

namespace esphome::nuki_lock {
    lock::LockState nuki_to_lock_state(NukiLock::LockState nukiLockState) {
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

    bool nuki_doorsensor_to_binary(Nuki::DoorSensorState nuki_door_sensor_state) {
        if (nuki_door_sensor_state == Nuki::DoorSensorState::DoorClosed) {
            return false;
        }
        return true;
    }

    NukiLock::ButtonPressAction button_press_action_to_enum(const char* str)
    {
        if (strcmp(str, "No action") == 0) {
            return NukiLock::ButtonPressAction::NoAction;
        } else if (strcmp(str, "Intelligent") == 0) {
            return NukiLock::ButtonPressAction::Intelligent;
        } else if (strcmp(str, "Unlock") == 0) {
            return NukiLock::ButtonPressAction::Unlock;
        } else if (strcmp(str, "Lock") == 0) {
            return NukiLock::ButtonPressAction::Lock;
        } else if (strcmp(str, "Open door") == 0) {
            return NukiLock::ButtonPressAction::Unlatch;
        } else if (strcmp(str, "Lock 'n' Go") == 0) {
            return NukiLock::ButtonPressAction::LockNgo;
        } else if (strcmp(str, "Show state") == 0) {
            return NukiLock::ButtonPressAction::ShowStatus;
        }
        return NukiLock::ButtonPressAction::NoAction;
    }

    void button_press_action_to_string(const NukiLock::ButtonPressAction action, char* str) {
        switch (action) {
            case NukiLock::ButtonPressAction::NoAction:
                strcpy(str, "No action");
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
                strcpy(str, "Open door");
                break;
            case NukiLock::ButtonPressAction::LockNgo:
                strcpy(str, "Lock 'n' Go");
                break;
            case NukiLock::ButtonPressAction::ShowStatus:
                strcpy(str, "Show state");
                break;
            default:
                strcpy(str, "No action");
                break;
        }
    }

    void battery_type_to_string(const Nuki::BatteryType battery_type, char* str) {
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

    Nuki::BatteryType battery_type_to_enum(const char* str) {
        if(strcmp(str, "Alkali") == 0) {
            return Nuki::BatteryType::Alkali;
        } else if(strcmp(str, "Accumulators") == 0) {
            return Nuki::BatteryType::Accumulators;
        } else if(strcmp(str, "Lithium") == 0) {
            return Nuki::BatteryType::Lithium;
        }
        return (Nuki::BatteryType)0xff;
    }

    void homekit_status_to_string(const int status, char* str) {
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

    void motor_speed_to_string(const NukiLock::MotorSpeed speed, char* str) {
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

    NukiLock::MotorSpeed motor_speed_to_enum(const char* str) {
        if(strcmp(str, "Standard") == 0) {
            return NukiLock::MotorSpeed::Standard;
        } else if(strcmp(str, "Insane") == 0) {
            return NukiLock::MotorSpeed::Insane;
        } else if(strcmp(str, "Gentle") == 0) {
            return NukiLock::MotorSpeed::Gentle;
        }
        return NukiLock::MotorSpeed::Standard;
    }

    uint8_t fob_action_to_int(const char *str) {
        if(strcmp(str, "No action") == 0) {
            return 0;
        } else if(strcmp(str, "Unlock") == 0) {
            return 1;
        } else if(strcmp(str, "Lock") == 0) {
            return 2;
        } else if(strcmp(str, "Lock 'n' Go") == 0) {
            return 3;
        } else if(strcmp(str, "Intelligent") == 0) {
            return 4;
        }
        return 99;
    }

    void fob_action_to_string(const int action, char* str) {
        switch (action) {
            case 0:
                strcpy(str, "No action");
                break;
            case 1:
                strcpy(str, "Unlock");
                break;
            case 2:
                strcpy(str, "Lock");
                break;
            case 3:
                strcpy(str, "Lock 'n' Go");
                break;
            case 4:
                strcpy(str, "Intelligent");
                break;
            default:
                strcpy(str, "No action");
                break;
        }
    }

    Nuki::TimeZoneId timezone_to_enum(const char *str) {
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

    void timezone_to_string(const Nuki::TimeZoneId timeZoneId, char* str) {
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

    Nuki::AdvertisingMode advertising_mode_to_enum(const char *str) {
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

    void advertising_mode_to_string(const Nuki::AdvertisingMode mode, char* str) {
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

    void pin_state_to_string(const PinState value, char* str)
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
}