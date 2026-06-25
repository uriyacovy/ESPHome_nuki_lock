#pragma once

#include "esphome/components/lock/lock.h"
#include "nuki_lock_protocol.h"

namespace esphome::nuki_lock {
    enum PinState
    {
        NotSet = 0,
        Set = 1,
        Valid = 2,
        Invalid = 3
    };

    lock::LockState nuki_to_lock_state(LockState);
    
    bool nuki_doorsensor_to_binary(DoorSensorState);
    
    uint8_t fob_action_to_int(const char *str);
    void fob_action_to_string(const int action, char* str);
    
    BatteryType battery_type_to_enum(const char* str);
    void battery_type_to_string(const BatteryType battery_type, char* str);
    
    MotorSpeed motor_speed_to_enum(const char* str);
    void motor_speed_to_string(const MotorSpeed speed, char* str);
    
    ButtonPressAction button_press_action_to_enum(const char* str);
    void button_press_action_to_string(ButtonPressAction action, char* str);
    
    TimeZoneId timezone_to_enum(const char *str);
    void timezone_to_string(const TimeZoneId timeZoneId, char* str);
    
    AdvertisingMode advertising_mode_to_enum(const char *str);
    void advertising_mode_to_string(const AdvertisingMode mode, char* str);
    
    void homekit_status_to_string(const int status, char* str);
    void pin_state_to_string(const PinState value, char* str);

    // Decode the composite KeyturnerStates status bytes (Smart Lock 4th Generation/Ultra
    // only - all-zeros/default on older locks) into a short human-readable summary.
    void wifi_connection_status_to_string(const uint8_t status, char* str);
    void mqtt_connection_status_to_string(const uint8_t status, char* str);
    void thread_connection_status_to_string(const uint8_t status, char* str);
}  // namespace esphome::nuki_lock