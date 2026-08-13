#pragma once

#include "esphome/components/lock/lock.h"
#include "NukiLock.h"

namespace esphome::nuki_lock {
    enum PinState
    {
        NotSet = 0,
        Set = 1,
        Valid = 2,
        Invalid = 3
    };

    lock::LockState nuki_to_lock_state(NukiLock::LockState);
    
    bool nuki_doorsensor_to_binary(Nuki::DoorSensorState);
    
    uint8_t fob_action_to_int(const char *str);
    void fob_action_to_string(const int action, char* str);
    
    Nuki::BatteryType battery_type_to_enum(const char* str);
    void battery_type_to_string(const Nuki::BatteryType battery_type, char* str);
    
    NukiLock::MotorSpeed motor_speed_to_enum(const char* str);
    void motor_speed_to_string(const NukiLock::MotorSpeed speed, char* str);
    
    NukiLock::ButtonPressAction button_press_action_to_enum(const char* str);
    void button_press_action_to_string(NukiLock::ButtonPressAction action, char* str);
    
    Nuki::TimeZoneId timezone_to_enum(const char *str);
    void timezone_to_string(const Nuki::TimeZoneId timeZoneId, char* str);
    
    Nuki::AdvertisingMode advertising_mode_to_enum(const char *str);
    void advertising_mode_to_string(const Nuki::AdvertisingMode mode, char* str);
    
    void homekit_status_to_string(const int status, char* str);
    void pin_state_to_string(const PinState value, char* str);
}