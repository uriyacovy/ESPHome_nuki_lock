#pragma once

/**
 * @file nuki_lock_protocol_utils.h
 * Implementation of generic/helper functions
 *
 * Created on: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include "nuki_ble_constants.h"
#include "nuki_lock_protocol_constants.h"
#include <bitset>
#include "esphome/core/log.h"

namespace esphome::nuki_lock {

void cmd_result_to_string(const CmdResult state, char* str);


void log_lock_error_code(uint8_t errorCode, bool debug = false);
void log_config(Config config, bool debug = false);
void log_new_config(NewConfig newConfig, bool debug = false);
void log_new_time_control_entry(NewTimeControlEntry newTimeControlEntry, bool debug = false);
void log_time_control_entry(TimeControlEntry timeControlEntry, bool debug = false);
void log_completion_status(CompletionStatus completionStatus, bool debug = false);
void log_nuki_trigger(NukiTrigger nukiTrigger, bool debug = false);
void log_lock_action(LockAction lock_action, bool debug = false);
void log_keyturner_state(KeyTurnerState keyTurnerState, bool debug = false);
void log_battery_report(BatteryReport batteryReport, bool debug = false);
void log_log_entry(LogEntry logEntry, bool debug = false);
void log_advanced_config(AdvancedConfig advancedConfig, bool debug = false);
void log_new_advanced_config(NewAdvancedConfig newAdvancedConfig, bool debug = false);
void log_mqtt_config(MqttConfig mqttConfig, bool debug = false);
void log_mqtt_config_for_migration(MqttConfigForMigration mqttConfigForMigration, bool debug = false);
void log_wifi_scan_entry(WifiScanEntry wifiScanEntry, bool debug = false);
void log_door_sensor_config(DoorSensorConfig doorSensorConfig, bool debug = false);
void log_wifi_config(WifiConfig wifiConfig, bool debug = false);
void log_wifi_config_for_migration(WifiConfigForMigration wifiConfigForMigration, bool debug = false);
void log_general_statistics(GeneralStatistics generalStatistics, bool debug = false);
void log_daily_statistics(DailyStatistics dailyStatistics, bool debug = false);
void log_internal_log_entry(InternalLogEntry internalLogEntry, bool debug = false);

}  // namespace esphome::nuki_lock