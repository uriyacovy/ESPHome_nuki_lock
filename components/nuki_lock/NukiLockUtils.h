#pragma once

/**
 * @file NukiUtills.h
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

#include "NukiDataTypes.h"
#include "NukiLockConstants.h"
#include <bitset>
#include "esp_log.h"

namespace NukiLock {

void cmdResultToString(const CmdResult state, char* str);


void logLockErrorCode(uint8_t errorCode, bool debug = false);
void logConfig(Config config, bool debug = false);
void logNewConfig(NewConfig newConfig, bool debug = false);
void logNewKeypadEntry(NewKeypadEntry newKeypadEntry, bool debug = false);
void logKeypadEntry(KeypadEntry keypadEntry, bool debug = false);
void logUpdatedKeypadEntry(UpdatedKeypadEntry updatedKeypadEntry, bool debug = false);
void logAuthorizationEntry(AuthorizationEntry authorizationEntry, bool debug = false);
void logNewAuthorizationEntry(NewAuthorizationEntry newAuthorizationEntry, bool debug = false);
void logUpdatedAuthorizationEntry(UpdatedAuthorizationEntry updatedAuthorizationEntry, bool debug = false);
void logNewTimeControlEntry(NewTimeControlEntry newTimeControlEntry, bool debug = false);
void logTimeControlEntry(TimeControlEntry timeControlEntry, bool debug = false);
void logCompletionStatus(CompletionStatus completionStatus, bool debug = false);
void logNukiTrigger(Trigger nukiTrigger, bool debug = false);
void logLockAction(LockAction lockAction, bool debug = false);
void logKeyturnerState(KeyTurnerState keyTurnerState, bool debug = false);
void logBatteryReport(BatteryReport batteryReport, bool debug = false);
void logLogEntry(LogEntry logEntry, bool debug = false);
void logAdvancedConfig(AdvancedConfig advancedConfig, bool debug = false);
void logNewAdvancedConfig(NewAdvancedConfig newAdvancedConfig, bool debug = false);
void logMqttConfig(MqttConfig mqttConfig, bool debug = false);
void logMqttConfigForMigration(MqttConfigForMigration mqttConfigForMigration, bool debug = false);
void logWifiScanEntry(WifiScanEntry wifiScanEntry, bool debug = false);
void logKeypad2Config(Keypad2Config keypad2Config, bool debug = false);
void logDoorSensorConfig(DoorSensorConfig doorSensorConfig, bool debug = false);
void logWifiConfig(WifiConfig wifiConfig, bool debug = false);
void logWifiConfigForMigration(WifiConfigForMigration wifiConfigForMigration, bool debug = false);
void logGeneralStatistics(GeneralStatistics generalStatistics, bool debug = false);
void logDailyStatistics(DailyStatistics dailyStatistics, bool debug = false);
void logFingerprintEntry(FingerprintEntry fingerprintEntry, bool debug = false);
void logInternalLogEntry(InternalLogEntry internalLogEntry, bool debug = false);
void logAccessoryInfo(AccessoryInfo accessoryInfo, bool debug = false);

} // namespace Nuki