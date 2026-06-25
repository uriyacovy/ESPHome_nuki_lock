#include "nuki_lock_utils.h"
#include <cstdio>
#include <cstring>

#include "esphome/components/lock/lock.h"
#include "nuki_lock_protocol.h"

namespace esphome::nuki_lock {
    lock::LockState nuki_to_lock_state(LockState nukiLockState) {
        switch(nukiLockState) {
            case LockState::Locked:
                return lock::LOCK_STATE_LOCKED;
            case LockState::Unlocked:
            case LockState::Unlatched:
                return lock::LOCK_STATE_UNLOCKED;
            case LockState::MotorBlocked:
                return lock::LOCK_STATE_JAMMED;
            case LockState::Locking:
                return lock::LOCK_STATE_LOCKING;
            case LockState::Unlocking:
            case LockState::Unlatching:
                return lock::LOCK_STATE_UNLOCKING;
            default:
                return lock::LOCK_STATE_NONE;
        }
    }

    bool nuki_doorsensor_to_binary(DoorSensorState nuki_door_sensor_state) {
        if (nuki_door_sensor_state == DoorSensorState::DoorClosed) {
            return false;
        }
        return true;
    }

    ButtonPressAction button_press_action_to_enum(const char* str)
    {
        if (strcmp(str, "No action") == 0) {
            return ButtonPressAction::NoAction;
        } else if (strcmp(str, "Intelligent") == 0) {
            return ButtonPressAction::Intelligent;
        } else if (strcmp(str, "Unlock") == 0) {
            return ButtonPressAction::Unlock;
        } else if (strcmp(str, "Lock") == 0) {
            return ButtonPressAction::Lock;
        } else if (strcmp(str, "Open door") == 0) {
            return ButtonPressAction::Unlatch;
        } else if (strcmp(str, "Lock 'n' Go") == 0) {
            return ButtonPressAction::LockNgo;
        } else if (strcmp(str, "Show state") == 0) {
            return ButtonPressAction::ShowStatus;
        }
        return ButtonPressAction::NoAction;
    }

    void button_press_action_to_string(const ButtonPressAction action, char* str) {
        switch (action) {
            case ButtonPressAction::NoAction:
                strcpy(str, "No action");
                break;
            case ButtonPressAction::Intelligent:
                strcpy(str, "Intelligent");
                break;
            case ButtonPressAction::Unlock:
                strcpy(str, "Unlock");
                break;
            case ButtonPressAction::Lock:
                strcpy(str, "Lock");
                break;
            case ButtonPressAction::Unlatch:
                strcpy(str, "Open door");
                break;
            case ButtonPressAction::LockNgo:
                strcpy(str, "Lock 'n' Go");
                break;
            case ButtonPressAction::ShowStatus:
                strcpy(str, "Show state");
                break;
            default:
                strcpy(str, "No action");
                break;
        }
    }

    void battery_type_to_string(const BatteryType battery_type, char* str) {
        switch (battery_type) {
            case BatteryType::Alkali:
                strcpy(str, "Alkali");
                break;
            case BatteryType::Accumulators:
                strcpy(str, "Accumulators");
                break;
            case BatteryType::Lithium:
                strcpy(str, "Lithium");
                break;
            default:
                strcpy(str, "undefined");
                break;
        }
    }

    BatteryType battery_type_to_enum(const char* str) {
        if(strcmp(str, "Alkali") == 0) {
            return BatteryType::Alkali;
        } else if(strcmp(str, "Accumulators") == 0) {
            return BatteryType::Accumulators;
        } else if(strcmp(str, "Lithium") == 0) {
            return BatteryType::Lithium;
        }
        return (BatteryType)0xff;
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

    void motor_speed_to_string(const MotorSpeed speed, char* str) {
        switch (speed) {
            case MotorSpeed::Standard:
                strcpy(str, "Standard");
                break;
            case MotorSpeed::Insane:
                strcpy(str, "Insane");
                break;
            case MotorSpeed::Gentle:
                strcpy(str, "Gentle");
                break;
            default:
                strcpy(str, "undefined");
                break;
        }
    }

    MotorSpeed motor_speed_to_enum(const char* str) {
        if(strcmp(str, "Standard") == 0) {
            return MotorSpeed::Standard;
        } else if(strcmp(str, "Insane") == 0) {
            return MotorSpeed::Insane;
        } else if(strcmp(str, "Gentle") == 0) {
            return MotorSpeed::Gentle;
        }
        return MotorSpeed::Standard;
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

    TimeZoneId timezone_to_enum(const char *str) {
        if(strcmp(str, "Africa/Cairo") == 0) {
            return TimeZoneId::Africa_Cairo;
        } else if(strcmp(str, "Africa/Lagos") == 0) {
            return TimeZoneId::Africa_Lagos;
        } else if(strcmp(str, "Africa/Maputo") == 0) {
            return TimeZoneId::Africa_Maputo;
        } else if(strcmp(str, "Africa/Nairobi") == 0) {
            return TimeZoneId::Africa_Nairobi;
        } else if(strcmp(str, "America/Anchorage") == 0) {
            return TimeZoneId::America_Anchorage;
        } else if(strcmp(str, "America/Argentina/Buenos_Aires") == 0) {
            return TimeZoneId::America_Argentina_Buenos_Aires;
        } else if(strcmp(str, "America/Chicago") == 0) {
            return TimeZoneId::America_Chicago;
        } else if(strcmp(str, "America/Denver") == 0) {
            return TimeZoneId::America_Denver;
        } else if(strcmp(str, "America/Halifax") == 0) {
            return TimeZoneId::America_Halifax;
        } else if(strcmp(str, "America/Los_Angeles") == 0) {
            return TimeZoneId::America_Los_Angeles;
        } else if(strcmp(str, "America/Manaus") == 0) {
            return TimeZoneId::America_Manaus;
        } else if(strcmp(str, "America/Mexico_City") == 0) {
            return TimeZoneId::America_Mexico_City;
        } else if(strcmp(str, "America/New_York") == 0) {
            return TimeZoneId::America_New_York;
        } else if(strcmp(str, "America/Phoenix") == 0) {
            return TimeZoneId::America_Phoenix;
        } else if(strcmp(str, "America/Regina") == 0) {
            return TimeZoneId::America_Regina;
        } else if(strcmp(str, "America/Santiago") == 0) {
            return TimeZoneId::America_Santiago;
        } else if(strcmp(str, "America/Sao_Paulo") == 0) {
            return TimeZoneId::America_Sao_Paulo;
        } else if(strcmp(str, "America/St_Johns") == 0) {
            return TimeZoneId::America_St_Johns;
        } else if(strcmp(str, "Asia/Bangkok") == 0) {
            return TimeZoneId::Asia_Bangkok;
        } else if(strcmp(str, "Asia/Dubai") == 0) {
            return TimeZoneId::Asia_Dubai;
        } else if(strcmp(str, "Asia/Hong_Kong") == 0) {
            return TimeZoneId::Asia_Hong_Kong;
        } else if(strcmp(str, "Asia/Jerusalem") == 0) {
            return TimeZoneId::Asia_Jerusalem;
        } else if(strcmp(str, "Asia/Karachi") == 0) {
            return TimeZoneId::Asia_Karachi;
        } else if(strcmp(str, "Asia/Kathmandu") == 0) {
            return TimeZoneId::Asia_Kathmandu;
        } else if(strcmp(str, "Asia/Kolkata") == 0) {
            return TimeZoneId::Asia_Kolkata;
        } else if(strcmp(str, "Asia/Riyadh") == 0) {
            return TimeZoneId::Asia_Riyadh;
        } else if(strcmp(str, "Asia/Seoul") == 0) {
            return TimeZoneId::Asia_Seoul;
        } else if(strcmp(str, "Asia/Shanghai") == 0) {
            return TimeZoneId::Asia_Shanghai;
        } else if(strcmp(str, "Asia/Tehran") == 0) {
            return TimeZoneId::Asia_Tehran;
        } else if(strcmp(str, "Asia/Tokyo") == 0) {
            return TimeZoneId::Asia_Tokyo;
        } else if(strcmp(str, "Asia/Yangon") == 0) {
            return TimeZoneId::Asia_Yangon;
        } else if(strcmp(str, "Australia/Adelaide") == 0) {
            return TimeZoneId::Australia_Adelaide;
        } else if(strcmp(str, "Australia/Brisbane") == 0) {
            return TimeZoneId::Australia_Brisbane;
        } else if(strcmp(str, "Australia/Darwin") == 0) {
            return TimeZoneId::Australia_Darwin;
        } else if(strcmp(str, "Australia/Hobart") == 0) {
            return TimeZoneId::Australia_Hobart;
        } else if(strcmp(str, "Australia/Perth") == 0) {
            return TimeZoneId::Australia_Perth;
        } else if(strcmp(str, "Australia/Sydney") == 0) {
            return TimeZoneId::Australia_Sydney;
        } else if(strcmp(str, "Europe/Berlin") == 0) {
            return TimeZoneId::Europe_Berlin;
        } else if(strcmp(str, "Europe/Helsinki") == 0) {
            return TimeZoneId::Europe_Helsinki;
        } else if(strcmp(str, "Europe/Istanbul") == 0) {
            return TimeZoneId::Europe_Istanbul;
        } else if(strcmp(str, "Europe/London") == 0) {
            return TimeZoneId::Europe_London;
        } else if(strcmp(str, "Europe/Moscow") == 0) {
            return TimeZoneId::Europe_Moscow;
        } else if(strcmp(str, "Pacific/Auckland") == 0) {
            return TimeZoneId::Pacific_Auckland;
        } else if(strcmp(str, "Pacific/Guam") == 0) {
            return TimeZoneId::Pacific_Guam;
        } else if(strcmp(str, "Pacific/Honolulu") == 0) {
            return TimeZoneId::Pacific_Honolulu;
        } else if(strcmp(str, "Pacific/Pago_Pago") == 0) {
            return TimeZoneId::Pacific_Pago_Pago;
        } else if(strcmp(str, "None") == 0) {
            return TimeZoneId::None;
        }
        return (TimeZoneId)0xff;
    }

    void timezone_to_string(const TimeZoneId timeZoneId, char* str) {
        switch (timeZoneId) {
            case TimeZoneId::Africa_Cairo:
                strcpy(str, "Africa/Cairo");
                break;
            case TimeZoneId::Africa_Lagos:
                strcpy(str, "Africa/Lagos");
                break;
            case TimeZoneId::Africa_Maputo:
                strcpy(str, "Africa/Maputo");
                break;
            case TimeZoneId::Africa_Nairobi:
                strcpy(str, "Africa/Nairobi");
                break;
            case TimeZoneId::America_Anchorage:
                strcpy(str, "America/Anchorage");
                break;
            case TimeZoneId::America_Argentina_Buenos_Aires:
                strcpy(str, "America/Argentina/Buenos_Aires");
                break;
            case TimeZoneId::America_Chicago:
                strcpy(str, "America/Chicago");
                break;
            case TimeZoneId::America_Denver:
                strcpy(str, "America/Denver");
                break;
            case TimeZoneId::America_Halifax:
                strcpy(str, "America/Halifax");
                break;
            case TimeZoneId::America_Los_Angeles:
                strcpy(str, "America/Los_Angeles");
                break;
            case TimeZoneId::America_Manaus:
                strcpy(str, "America/Manaus");
                break;
            case TimeZoneId::America_Mexico_City:
                strcpy(str, "America/Mexico_City");
                break;
            case TimeZoneId::America_New_York:
                strcpy(str, "America/New_York");
                break;
            case TimeZoneId::America_Phoenix:
                strcpy(str, "America/Phoenix");
                break;
            case TimeZoneId::America_Regina:
                strcpy(str, "America/Regina");
                break;
            case TimeZoneId::America_Santiago:
                strcpy(str, "America/Santiago");
                break;
            case TimeZoneId::America_Sao_Paulo:
                strcpy(str, "America/Sao_Paulo");
                break;
            case TimeZoneId::America_St_Johns:
                strcpy(str, "America/St_Johns");
                break;
            case TimeZoneId::Asia_Bangkok:
                strcpy(str, "Asia/Bangkok");
                break;
            case TimeZoneId::Asia_Dubai:
                strcpy(str, "Asia/Dubai");
                break;
            case TimeZoneId::Asia_Hong_Kong:
                strcpy(str, "Asia/Hong_Kong");
                break;
            case TimeZoneId::Asia_Jerusalem:
                strcpy(str, "Asia/Jerusalem");
                break;
            case TimeZoneId::Asia_Karachi:
                strcpy(str, "Asia/Karachi");
                break;
            case TimeZoneId::Asia_Kathmandu:
                strcpy(str, "Asia/Kathmandu");
                break;
            case TimeZoneId::Asia_Kolkata:
                strcpy(str, "Asia/Kolkata");
                break;
            case TimeZoneId::Asia_Riyadh:
                strcpy(str, "Asia/Riyadh");
                break;
            case TimeZoneId::Asia_Seoul:
                strcpy(str, "Asia/Seoul");
                break;
            case TimeZoneId::Asia_Shanghai:
                strcpy(str, "Asia/Shanghai");
                break;
            case TimeZoneId::Asia_Tehran:
                strcpy(str, "Asia/Tehran");
                break;
            case TimeZoneId::Asia_Tokyo:
                strcpy(str, "Asia/Tokyo");
                break;
            case TimeZoneId::Asia_Yangon:
                strcpy(str, "Asia/Yangon");
                break;
            case TimeZoneId::Australia_Adelaide:
                strcpy(str, "Australia/Adelaide");
                break;
            case TimeZoneId::Australia_Brisbane:
                strcpy(str, "Australia/Brisbane");
                break;
            case TimeZoneId::Australia_Darwin:
                strcpy(str, "Australia/Darwin");
                break;
            case TimeZoneId::Australia_Hobart:
                strcpy(str, "Australia/Hobart");
                break;
            case TimeZoneId::Australia_Perth:
                strcpy(str, "Australia/Perth");
                break;
            case TimeZoneId::Australia_Sydney:
                strcpy(str, "Australia/Sydney");
                break;
            case TimeZoneId::Europe_Berlin:
                strcpy(str, "Europe/Berlin");
                break;
            case TimeZoneId::Europe_Helsinki:
                strcpy(str, "Europe/Helsinki");
                break;
            case TimeZoneId::Europe_Istanbul:
                strcpy(str, "Europe/Istanbul");
                break;
            case TimeZoneId::Europe_London:
                strcpy(str, "Europe/London");
                break;
            case TimeZoneId::Europe_Moscow:
                strcpy(str, "Europe/Moscow");
                break;
            case TimeZoneId::Pacific_Auckland:
                strcpy(str, "Pacific/Auckland");
                break;
            case TimeZoneId::Pacific_Guam:
                strcpy(str, "Pacific/Guam");
                break;
            case TimeZoneId::Pacific_Honolulu:
                strcpy(str, "Pacific/Honolulu");
                break;
            case TimeZoneId::Pacific_Pago_Pago:
                strcpy(str, "Pacific/Pago_Pago");
                break;
            case TimeZoneId::None:
                strcpy(str, "None");
                break;
            default:
                strcpy(str, "None");
                break;
        }
    }

    AdvertisingMode advertising_mode_to_enum(const char *str) {
        if(strcmp(str, "Automatic") == 0) {
            return AdvertisingMode::Automatic;
        } else if(strcmp(str, "Normal") == 0) {
            return AdvertisingMode::Normal;
        } else if(strcmp(str, "Slow") == 0) {
            return AdvertisingMode::Slow;
        } else if(strcmp(str, "Slowest") == 0) {
            return AdvertisingMode::Slowest;
        }
        return (AdvertisingMode)0xff;
    }

    void advertising_mode_to_string(const AdvertisingMode mode, char* str) {
        switch (mode) {
            case AdvertisingMode::Automatic:
                strcpy(str, "Automatic");
                break;
            case AdvertisingMode::Normal:
                strcpy(str, "Normal");
                break;
            case AdvertisingMode::Slow:
                strcpy(str, "Slow");
                break;
            case AdvertisingMode::Slowest:
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

    // Shared by wifi/mqtt/thread_connection_status_to_string below - bits 0-1 of all three
    // use this same 4-state connection lifecycle.
    static const char* connection_status_to_string(const uint8_t status) {
        switch (status & 3) {
            case 0: return "disabled";
            case 1: return "disconnected";
            case 2: return "connecting";
            default: return "connected";
        }
    }

    void wifi_connection_status_to_string(const uint8_t status, char* str) {
        const char* sse_status;
        switch ((status >> 2) & 3) {
            case 0: sse_status = "suspended"; break;
            case 1: sse_status = "not reachable"; break;
            case 2: sse_status = "connecting"; break;
            default: sse_status = "connected"; break;
        }
        sprintf(str, "WiFi %s, SSE %s, quality %u%%", connection_status_to_string(status), sse_status, (unsigned)((status >> 4) & 15) * 100 / 15);
    }

    void mqtt_connection_status_to_string(const uint8_t status, char* str) {
        sprintf(str, "MQTT %s (via %s)", connection_status_to_string(status), ((status >> 2) & 1) ? "Thread" : "WiFi");
    }

    void thread_connection_status_to_string(const uint8_t status, char* str) {
        const char* sse_status;
        switch ((status >> 2) & 3) {
            case 0: sse_status = "suspended"; break;
            case 1: sse_status = "not reachable"; break;
            case 2: sse_status = "connecting"; break;
            default: sse_status = "connected"; break;
        }
        sprintf(str, "Thread %s, SSE %s%s%s", connection_status_to_string(status), sse_status,
                (status & 16) ? ", commissioning active" : "", (status & 32) ? ", WiFi suspended" : "");
    }
}  // namespace esphome::nuki_lock