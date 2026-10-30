/**
 * @file nuki_lock_protocol_utils.cpp
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

#include "nuki_lock_protocol_utils.h"

#include "esphome/core/log.h"
#include <cstring>
#include <cstdint>


namespace esphome::nuki_lock {

static const char *const TAG = "NukiBle.NukiLock";

void cmd_result_to_string(const CmdResult state, char* str) {
  switch (state) {
    case CmdResult::Success:
      strcpy(str, "success");
      break;
    case CmdResult::Failed:
      strcpy(str, "failed");
      break;
    case CmdResult::TimeOut:
      strcpy(str, "timeOut");
      break;
    case CmdResult::Working:
      strcpy(str, "working");
      break;
    case CmdResult::NotPaired:
      strcpy(str, "notPaired");
      break;
    case CmdResult::Error:
      strcpy(str, "error");
      break;
    default:
      strcpy(str, "undefined");
      break;
  }
}

void log_lock_error_code(uint8_t errorCode, bool debug) {
  if (debug) {
    switch (errorCode) {
      case (uint8_t)ErrorCode::ERROR_BAD_CRC :
        ESP_LOGE(TAG, "ERROR_BAD_CRC");
        break;
      case (uint8_t)ErrorCode::ERROR_BAD_LENGTH :
        ESP_LOGE(TAG, "ERROR_BAD_LENGTH");
        break;
      case (uint8_t)ErrorCode::ERROR_UNKNOWN :
        ESP_LOGE(TAG, "ERROR_UNKNOWN");
        break;
      case (uint8_t)ErrorCode::P_ERROR_NOT_PAIRING :
        ESP_LOGE(TAG, "P_ERROR_NOT_PAIRING");
        break;
      case (uint8_t)ErrorCode::P_ERROR_BAD_AUTHENTICATOR :
        ESP_LOGE(TAG, "P_ERROR_BAD_AUTHENTICATOR");
        break;
      case (uint8_t)ErrorCode::P_ERROR_BAD_PARAMETER :
        ESP_LOGE(TAG, "P_ERROR_BAD_PARAMETER");
        break;
      case (uint8_t)ErrorCode::P_ERROR_MAX_USER :
        ESP_LOGE(TAG, "P_ERROR_MAX_USER");
        break;
      case (uint8_t)ErrorCode::K_ERROR_AUTO_UNLOCK_TOO_RECENT :
        ESP_LOGE(TAG, "K_ERROR_AUTO_UNLOCK_TOO_RECENT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_NONCE :
        ESP_LOGE(TAG, "K_ERROR_BAD_NONCE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_PARAMETER :
        ESP_LOGE(TAG, "K_ERROR_BAD_PARAMETER");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_PIN :
        ESP_LOGE(TAG, "K_ERROR_BAD_PIN");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BUSY :
        ESP_LOGE(TAG, "K_ERROR_BUSY");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CANCELED :
        ESP_LOGE(TAG, "K_ERROR_CANCELED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CLUTCH_FAILURE :
        ESP_LOGE(TAG, "K_ERROR_CLUTCH_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CLUTCH_POWER_FAILURE :
        ESP_LOGE(TAG, "K_ERROR_CLUTCH_POWER_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_ALREADY_EXISTS :
        ESP_LOGE(TAG, "K_ERROR_CODE_ALREADY_EXISTS");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID :
        ESP_LOGE(TAG, "K_ERROR_CODE_INVALID");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_1 :
        ESP_LOGE(TAG, "K_ERROR_CODE_INVALID_TIMEOUT_1");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_2 :
        ESP_LOGE(TAG, "K_ERROR_CODE_INVALID_TIMEOUT_2");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_3 :
        ESP_LOGE(TAG, "K_ERROR_CODE_INVALID_TIMEOUT_3");
        break;
      case (uint8_t)ErrorCode::K_ERROR_DISABLED :
        ESP_LOGE(TAG, "K_ERROR_DISABLED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_FIRMWARE_UPDATE_NEEDED :
        ESP_LOGE(TAG, "K_ERROR_FIRMWARE_UPDATE_NEEDED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_INVALID_AUTH_ID :
        ESP_LOGE(TAG, "K_ERROR_INVALID_AUTH_ID");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_BLOCKED :
        ESP_LOGE(TAG, "K_ERROR_MOTOR_BLOCKED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_LOW_VOLTAGE :
        ESP_LOGE(TAG, "K_ERROR_MOTOR_LOW_VOLTAGE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_POSITION_LIMIT :
        ESP_LOGE(TAG, "K_ERROR_MOTOR_POSITION_LIMIT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_POWER_FAILURE :
        ESP_LOGE(TAG, "K_ERROR_MOTOR_POWER_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_TIMEOUT :
        ESP_LOGE(TAG, "K_ERROR_MOTOR_TIMEOUT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_NOT_AUTHORIZED :
        ESP_LOGE(TAG, "K_ERROR_NOT_AUTHORIZED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_NOT_CALIBRATED :
        ESP_LOGE(TAG, "K_ERROR_NOT_CALIBRATED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_POSITION_UNKNOWN :
        ESP_LOGE(TAG, "K_ERROR_POSITION_UNKNOWN");
        break;
      case (uint8_t)ErrorCode::K_ERROR_REMOTE_NOT_ALLOWED :
        ESP_LOGE(TAG, "K_ERROR_REMOTE_NOT_ALLOWED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TIME_NOT_ALLOWED :
        ESP_LOGE(TAG, "K_ERROR_TIME_NOT_ALLOWED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TOO_MANY_ENTRIES :
        ESP_LOGE(TAG, "K_ERROR_TOO_MANY_ENTRIES");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TOO_MANY_PIN_ATTEMPTS :
        ESP_LOGE(TAG, "K_ERROR_TOO_MANY_PIN_ATTEMPTS");
        break;
      case (uint8_t)ErrorCode::K_ERROR_VOLTAGE_TOO_LOW :
        ESP_LOGE(TAG, "K_ERROR_VOLTAGE_TOO_LOW");
        break;
      default:
        ESP_LOGE(TAG, "UNDEFINED ERROR");
    }
  }
}

void log_config(Config config, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "nukiId: %d", (unsigned int)config.nukiId);
    ESP_LOGD(TAG, "name: %s", (const char*)config.name);
    ESP_LOGD(TAG, "latitude: %f", (const float)config.latitude);
    ESP_LOGD(TAG, "longitude: %f", (const float)config.longitude);
    ESP_LOGD(TAG, "autoUnlatch: %d", (unsigned int)config.autoUnlatch);
    ESP_LOGD(TAG, "pairing_enabled: %d", (unsigned int)config.pairing_enabled);
    ESP_LOGD(TAG, "buttonEnabled: %d", (unsigned int)config.buttonEnabled);
    ESP_LOGD(TAG, "ledEnabled: %d", (unsigned int)config.ledEnabled);
    ESP_LOGD(TAG, "ledBrightness: %d", (unsigned int)config.ledBrightness);
    ESP_LOGD(TAG, "currentTime Year: %d", (unsigned int)config.currentTimeYear);
    ESP_LOGD(TAG, "currentTime Month: %d", (unsigned int)config.currentTimeMonth);
    ESP_LOGD(TAG, "currentTime Day: %d", (unsigned int)config.currentTimeDay);
    ESP_LOGD(TAG, "currentTime Hour: %d", (unsigned int)config.currentTimeHour);
    ESP_LOGD(TAG, "currentTime Minute: %d", (unsigned int)config.currentTimeMinute);
    ESP_LOGD(TAG, "currentTime Second: %d", (unsigned int)config.currentTimeSecond);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (unsigned int)config.timeZoneOffset);
    ESP_LOGD(TAG, "dstMode: %d", (unsigned int)config.dstMode);
    ESP_LOGD(TAG, "hasFob: %d", (unsigned int)config.hasFob);
    ESP_LOGD(TAG, "fobAction1: %d", (unsigned int)config.fobAction1);
    ESP_LOGD(TAG, "fobAction2: %d", (unsigned int)config.fobAction2);
    ESP_LOGD(TAG, "fobAction3: %d", (unsigned int)config.fobAction3);
    ESP_LOGD(TAG, "singleLock: %d", (unsigned int)config.singleLock);
    ESP_LOGD(TAG, "advertisingMode: %d", (unsigned int)config.advertisingMode);
    ESP_LOGD(TAG, "hasKeypad: %d", (unsigned int)config.hasKeypad);
    ESP_LOGD(TAG, "firmwareVersion: %d.%d.%d", config.firmwareVersion[0], config.firmwareVersion[1], config.firmwareVersion[2]);
    ESP_LOGD(TAG, "hardwareRevision: %d.%d", config.hardwareRevision[0], config.hardwareRevision[1]);
    ESP_LOGD(TAG, "homeKitStatus: %d", (unsigned int)config.homeKitStatus);
    ESP_LOGD(TAG, "timeZoneId: %d", (unsigned int)config.timeZoneId);
    ESP_LOGD(TAG, "deviceType: %d", (unsigned int)config.deviceType);
    ESP_LOGD(TAG, "wifiCapable: %d", (unsigned int)config.capabilities & 1);
    ESP_LOGD(TAG, "threadCapable: %d", (unsigned int)(((unsigned int)config.capabilities & 2) != 0 ? 1:  0));
    ESP_LOGD(TAG, "hasKeypadV2: %d", (unsigned int)config.hasKeypadV2);
    ESP_LOGD(TAG, "matterStatus: %d", (unsigned int)config.matterStatus);
    ESP_LOGD(TAG, "productVariant: %d", (unsigned int)config.productVariant);
  }
}

void log_new_config(NewConfig newConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "name: %s", (const char*)newConfig.name);
    ESP_LOGD(TAG, "latitude: %f", (const float)newConfig.latitude);
    ESP_LOGD(TAG, "longitude: %f", (const float)newConfig.longitude);
    ESP_LOGD(TAG, "autoUnlatch: %d", (unsigned int)newConfig.autoUnlatch);
    ESP_LOGD(TAG, "pairing_enabled: %d", (unsigned int)newConfig.pairing_enabled);
    ESP_LOGD(TAG, "buttonEnabled: %d", (unsigned int)newConfig.buttonEnabled);
    ESP_LOGD(TAG, "ledEnabled: %d", (unsigned int)newConfig.ledEnabled);
    ESP_LOGD(TAG, "ledBrightness: %d", (unsigned int)newConfig.ledBrightness);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (unsigned int)newConfig.timeZoneOffset);
    ESP_LOGD(TAG, "dstMode: %d", (unsigned int)newConfig.dstMode);
    ESP_LOGD(TAG, "fobAction1: %d", (unsigned int)newConfig.fobAction1);
    ESP_LOGD(TAG, "fobAction2: %d", (unsigned int)newConfig.fobAction2);
    ESP_LOGD(TAG, "fobAction3: %d", (unsigned int)newConfig.fobAction3);
    ESP_LOGD(TAG, "singleLock: %d", (unsigned int)newConfig.singleLock);
    ESP_LOGD(TAG, "advertisingMode: %d", (unsigned int)newConfig.advertisingMode);
    ESP_LOGD(TAG, "timeZoneId: %d", (unsigned int)newConfig.timeZoneId);
  }
}

void log_new_time_control_entry(NewTimeControlEntry newTimeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "weekdays:%d", (unsigned int)newTimeControlEntry.weekdays);
    ESP_LOGD(TAG, "time:%d:%d", (unsigned int)newTimeControlEntry.timeHour, newTimeControlEntry.timeMin);
    ESP_LOGD(TAG, "lock_action:%d", (unsigned int)newTimeControlEntry.lock_action);
  }
}

void log_time_control_entry(TimeControlEntry timeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "entryId:%d", (unsigned int)timeControlEntry.entryId);
    ESP_LOGD(TAG, "enabled:%d", (unsigned int)timeControlEntry.enabled);
    ESP_LOGD(TAG, "weekdays:%d", (unsigned int)timeControlEntry.weekdays);
    ESP_LOGD(TAG, "time:%d:%d", (unsigned int)timeControlEntry.timeHour, timeControlEntry.timeMin);
    ESP_LOGD(TAG, "lock_action:%d", (unsigned int)timeControlEntry.lock_action);
  }
}

void log_completion_status(CompletionStatus completionStatus, bool debug) {
  if (debug) {
    switch (completionStatus) {
      case CompletionStatus::Busy :
        ESP_LOGD(TAG, "Completion status: busy");
        break;
      case CompletionStatus::Canceled :
        ESP_LOGD(TAG, "Completion status: canceled");
        break;
      case CompletionStatus::ClutchFailure :
        ESP_LOGD(TAG, "Completion status: clutchFailure");
        break;
      case CompletionStatus::IncompleteFailure :
        ESP_LOGD(TAG, "Completion status: incompleteFailure");
        break;
      case CompletionStatus::LowMotorVoltage :
        ESP_LOGD(TAG, "Completion status: lowMotorVoltage");
        break;
      case CompletionStatus::MotorBlocked :
        ESP_LOGD(TAG, "Completion status: motorBlocked");
        break;
      case CompletionStatus::MotorPowerFailure :
        ESP_LOGD(TAG, "Completion status: motorPowerFailure");
        break;
      case CompletionStatus::OtherError :
        ESP_LOGD(TAG, "Completion status: otherError");
        break;
      case CompletionStatus::Success :
        ESP_LOGD(TAG, "Completion status: success");
        break;
      case CompletionStatus::TooRecent :
        ESP_LOGD(TAG, "Completion status: tooRecent");
        break;
      case CompletionStatus::InvalidCode :
        ESP_LOGD(TAG, "Completion status: invalid code");
        break;
      default:
        ESP_LOGW(TAG, "Completion status: unknown");
        break;
    }
  }
}

void log_nuki_trigger(NukiTrigger nukiTrigger, bool debug) {
  if (debug) {
    switch (nukiTrigger) {
      case NukiTrigger::AutoLock :
        ESP_LOGD(TAG, "Trigger: autoLock");
        break;
      case NukiTrigger::Automatic :
        ESP_LOGD(TAG, "Trigger: automatic");
        break;
      case NukiTrigger::Button :
        ESP_LOGD(TAG, "Trigger: button");
        break;
      case NukiTrigger::Manual :
        ESP_LOGD(TAG, "Trigger: manual");
        break;
      case NukiTrigger::System :
        ESP_LOGD(TAG, "Trigger: system");
        break;
      default:
        ESP_LOGW(TAG, "Trigger: unknown");
        break;
    }
  }
}

void log_lock_action(LockAction lock_action, bool debug) {
  if (debug) {
    switch (lock_action) {
      case LockAction::FobAction1 :
        ESP_LOGD(TAG, "action: autoLock");
        break;
      case LockAction::FobAction2 :
        ESP_LOGD(TAG, "action: automatic");
        break;
      case LockAction::FobAction3 :
        ESP_LOGD(TAG, "action: button");
        break;
      case LockAction::FullLock :
        ESP_LOGD(TAG, "action: manual");
        break;
      case LockAction::Lock :
        ESP_LOGD(TAG, "action: system");
        break;
      case LockAction::LockNgo :
        ESP_LOGD(TAG, "action: system");
        break;
      case LockAction::LockNgoUnlatch :
        ESP_LOGD(TAG, "action: system");
        break;
      case LockAction::Unlatch :
        ESP_LOGD(TAG, "action: system");
        break;
      case LockAction::Unlock :
        ESP_LOGD(TAG, "action: system");
        break;
      default:
        ESP_LOGW(TAG, "action: unknown");
        break;
    }
  }
}

void log_keyturner_state(KeyTurnerState keyTurnerState, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "nukiState: %02x", (unsigned int)keyTurnerState.nukiState);
    ESP_LOGD(TAG, "lockState: %d", (unsigned int)keyTurnerState.lockState);
    log_nuki_trigger(keyTurnerState.trigger, debug);
    ESP_LOGD(TAG, "currentTimeYear: %d", (unsigned int)keyTurnerState.currentTimeYear);
    ESP_LOGD(TAG, "currentTimeMonth: %d", (unsigned int)keyTurnerState.currentTimeMonth);
    ESP_LOGD(TAG, "currentTimeDay: %d", (unsigned int)keyTurnerState.currentTimeDay);
    ESP_LOGD(TAG, "currentTimeHour: %d", (unsigned int)keyTurnerState.currentTimeHour);
    ESP_LOGD(TAG, "currentTimeMinute: %d", (unsigned int)keyTurnerState.currentTimeMinute);
    ESP_LOGD(TAG, "currentTimeSecond: %d", (unsigned int)keyTurnerState.currentTimeSecond);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (unsigned int)keyTurnerState.timeZoneOffset);
    ESP_LOGD(TAG, "criticalBatteryState composed value: %d", (unsigned int)keyTurnerState.criticalBatteryState);
    ESP_LOGD(TAG, "criticalBatteryState: %d", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState) == 1 ? 1 : 0));
    ESP_LOGD(TAG, "batteryCharging: %d", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState & 2) == 2 ? 1 : 0));
    ESP_LOGD(TAG, "batteryPercent: %d", (unsigned int)((keyTurnerState.criticalBatteryState & 0b11111100) >> 1));
    ESP_LOGD(TAG, "configUpdateCount: %d", (unsigned int)keyTurnerState.configUpdateCount);
    ESP_LOGD(TAG, "lockNgoTimer: %d", (unsigned int)keyTurnerState.lockNgoTimer);
    log_lock_action((LockAction)keyTurnerState.lastLockAction, debug);
    ESP_LOGD(TAG, "lastLockActionTrigger: %d", (unsigned int)keyTurnerState.lastLockActionTrigger);
    log_completion_status(keyTurnerState.lastLockActionCompletionStatus, debug);
    ESP_LOGD(TAG, "doorSensorState: %d", (unsigned int)keyTurnerState.doorSensorState);
    ESP_LOGD(TAG, "nightModeActive: %d", (unsigned int)keyTurnerState.nightModeActive);
    ESP_LOGD(TAG, "accessoryBatteryState composed value: %d", (unsigned int)keyTurnerState.accessoryBatteryState);
    ESP_LOGD(TAG, "Keypad bat critical feature supported: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 1) == 1 ? 1 : 0));
    ESP_LOGD(TAG, "Keypad Battery Critical: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 3) == 3 ? 1 : 0));
    ESP_LOGD(TAG, "Doorsensor bat critical feature supported: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 4) == 4 ? 1 : 0));
    ESP_LOGD(TAG, "Doorsensor Battery Critical: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 12) == 12 ? 1 : 0));
    ESP_LOGD(TAG, "remoteAccessStatus composed value: %d", (unsigned int)keyTurnerState.remoteAccessStatus);
    ESP_LOGD(TAG, "remoteAccessEnabled: %d", (unsigned int)(((keyTurnerState.remoteAccessStatus & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "bridgePaired: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 1) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "sseConnectedViaWifi: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 2) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "sseConnectionEstablished: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 3) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "isSseConnectedViaThread: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 4) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "threadSseUplinkEnabledByUser: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 5) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "nat64AvailableViaThread: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 6) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "bleConnectionStrength: %d", (unsigned int)keyTurnerState.bleConnectionStrength);
    ESP_LOGD(TAG, "wifiConnectionStrength: %d", (unsigned int)keyTurnerState.wifiConnectionStrength);
    ESP_LOGD(TAG, "wifiConnectionStatus composed value: %d", (unsigned int)keyTurnerState.wifiConnectionStatus);
    ESP_LOGD(TAG, "wifiStatus: %d", (unsigned int)(keyTurnerState.wifiConnectionStatus & 3));
    ESP_LOGD(TAG, "sseStatus: %d", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 2) & 3));
    ESP_LOGD(TAG, "wifiQuality: %d", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 4) & 15));
    ESP_LOGD(TAG, "mqttConnectionStatus composed value: %d", (unsigned int)keyTurnerState.mqttConnectionStatus);
    ESP_LOGD(TAG, "mqttStatus: %d", (unsigned int)(keyTurnerState.mqttConnectionStatus & 3));
    ESP_LOGD(TAG, "mqttConnectionChannel: %d", (unsigned int)((keyTurnerState.mqttConnectionStatus >> 2) & 1));
    ESP_LOGD(TAG, "threadConnectionStatus composed value: %d", (unsigned int)keyTurnerState.threadConnectionStatus);
    ESP_LOGD(TAG, "threadConnectionStatus: %d", (unsigned int)(keyTurnerState.threadConnectionStatus & 3));
    ESP_LOGD(TAG, "threadSseStatus: %d", (unsigned int)((keyTurnerState.threadConnectionStatus >> 2) & 3));
    ESP_LOGD(TAG, "isCommissioningModeActive: %d", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 16) != 0 ? 1 : 0));
    ESP_LOGD(TAG, "isWifiDisabledBecauseOfThread: %d", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 32) != 0 ? 1 : 0));
  }
}

void log_battery_report(BatteryReport batteryReport, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "batteryDrain:%d", (unsigned int)batteryReport.batteryDrain);
    ESP_LOGD(TAG, "batteryVoltage:%d", (unsigned int)batteryReport.batteryVoltage);
    ESP_LOGD(TAG, "criticalBatteryState:%d", (unsigned int)batteryReport.criticalBatteryState);
    ESP_LOGD(TAG, "lock_action:%d", (unsigned int)batteryReport.lock_action);
    ESP_LOGD(TAG, "startVoltage:%d", (unsigned int)batteryReport.startVoltage);
    ESP_LOGD(TAG, "lowestVoltage:%d", (unsigned int)batteryReport.lowestVoltage);
    ESP_LOGD(TAG, "lockDistance:%d", (unsigned int)batteryReport.lockDistance);
    ESP_LOGD(TAG, "startTemperature:%d", (unsigned int)batteryReport.startTemperature);
    ESP_LOGD(TAG, "maxTurnCurrent:%d", (unsigned int)batteryReport.maxTurnCurrent);
    ESP_LOGD(TAG, "batteryResistance:%d", (unsigned int)batteryReport.batteryResistance);
  }
}

void log_log_entry(LogEntry logEntry, bool debug) {
  ESP_LOGD(TAG, "[%lu] type: %u authId: %lu name: %s %d-%d-%d %d:%d:%d ", logEntry.index, (uint8_t)logEntry.loggingType, logEntry.authId, logEntry.name, logEntry.timeStampYear, logEntry.timeStampMonth, logEntry.timeStampDay, logEntry.timeStampHour, logEntry.timeStampMinute, logEntry.timeStampSecond);


  switch (logEntry.loggingType) {
    case LoggingType::LoggingEnabled: {
      ESP_LOGD(TAG, "Logging enabled: %d", (unsigned int)logEntry.data[0]);
      break;
    }
    case LoggingType::LockAction:
    case LoggingType::Calibration:
    case LoggingType::InitializationRun: {
      log_lock_action((LockAction)logEntry.data[0], debug);
      log_nuki_trigger((NukiTrigger)logEntry.data[1], debug);
      ESP_LOGD(TAG, "Flags: %d", (unsigned int)logEntry.data[2]);
      log_completion_status((CompletionStatus)logEntry.data[3], debug);
      break;
    }
    case LoggingType::KeypadAction: {
      log_lock_action((LockAction)logEntry.data[0], debug);
      ESP_LOGD(TAG, "Source: %d", (unsigned int)logEntry.data[1]);
      log_completion_status((CompletionStatus)logEntry.data[2], debug);
      uint16_t codeId = 0;
      memcpy(&codeId, &logEntry.data[3], 2);
      ESP_LOGD(TAG, "Code id: %d", (unsigned int)codeId);
      break;
    }
    case LoggingType::DoorSensor: {
      if (logEntry.data[0] == 0x00) {
        ESP_LOGD(TAG, "Door opened") ;
      }
      if (logEntry.data[0] == 0x01) {
        ESP_LOGD(TAG, "Door closed") ;
      }
      if (logEntry.data[0] == 0x02) {
        ESP_LOGD(TAG, "Door sensor jammed") ;
      }
      break;
    }
    case LoggingType::DoorSensorLoggingEnabled: {
      ESP_LOGD(TAG, "Logging enabled: %d", (unsigned int)logEntry.data[0]);
      break;
    }
    default:
      ESP_LOGW(TAG, "Unknown logging type");
      break;
  }
}

void log_advanced_config(AdvancedConfig advancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "totalDegrees: %d", (unsigned int)advancedConfig.totalDegrees);
    ESP_LOGD(TAG, "unlockedPositionOffsetDegrees: %d", (unsigned int)advancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "lockedPositionOffsetDegrees: %f", (const float)advancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "singleLockedPositionOffsetDegrees: %f", (const float)advancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "unlockedToLockedTransitionOffsetDegrees: %d", (unsigned int)advancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD(TAG, "lockNgoTimeout: %d", (unsigned int)advancedConfig.lockNgoTimeout);
    ESP_LOGD(TAG, "singleButtonPressAction: %d", (unsigned int)advancedConfig.singleButtonPressAction);
    ESP_LOGD(TAG, "doubleButtonPressAction: %d", (unsigned int)advancedConfig.doubleButtonPressAction);
    ESP_LOGD(TAG, "detachedCylinder: %d", (unsigned int)advancedConfig.detachedCylinder);
    ESP_LOGD(TAG, "batteryType: %d", (unsigned int)advancedConfig.batteryType);
    ESP_LOGD(TAG, "automaticBatteryTypeDetection: %d", (unsigned int)advancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD(TAG, "unlatchDuration: %d", (unsigned int)advancedConfig.unlatchDuration);
    ESP_LOGD(TAG, "autoLockTimeOut: %d", (unsigned int)advancedConfig.autoLockTimeOut);
    ESP_LOGD(TAG, "autoUnLockDisabled: %d", (unsigned int)advancedConfig.autoUnLockDisabled);
    ESP_LOGD(TAG, "nightModeEnabled: %d", (unsigned int)advancedConfig.nightModeEnabled);
    ESP_LOGD(TAG, "nightModeStartTime Hour: %d", (unsigned int)advancedConfig.nightModeStartTime[0]);
    ESP_LOGD(TAG, "nightModeStartTime Minute: %d", (unsigned int)advancedConfig.nightModeStartTime[1]);
    ESP_LOGD(TAG, "nightModeEndTime Hour: %d", (unsigned int)advancedConfig.nightModeEndTime[0]);
    ESP_LOGD(TAG, "nightModeEndTime Minute: %d", (unsigned int)advancedConfig.nightModeEndTime[1]);
    ESP_LOGD(TAG, "nightModeAutoLockEnabled: %d", (unsigned int)advancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD(TAG, "nightModeAutoUnlockDisabled: %d", (unsigned int)advancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD(TAG, "nightModeImmediateLockOnStart: %d", (unsigned int)advancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD(TAG, "autoLockEnabled: %d", (unsigned int)advancedConfig.autoLockEnabled);
    ESP_LOGD(TAG, "immediateAutoLockEnabled: %d", (unsigned int)advancedConfig.immediateAutoLockEnabled);
    ESP_LOGD(TAG, "autoUpdateEnabled: %d", (unsigned int)advancedConfig.autoUpdateEnabled);
    ESP_LOGD(TAG, "motorSpeed: %d", (unsigned int)advancedConfig.motorSpeed);
    ESP_LOGD(TAG, "enable_slow_speed_during_night_mode: %d", (unsigned int)advancedConfig.enable_slow_speed_during_night_mode);
  }
}

void log_new_advanced_config(NewAdvancedConfig newAdvancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "unlockedPositionOffsetDegrees: %d", (unsigned int)newAdvancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "lockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "singleLockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "unlockedToLockedTransitionOffsetDegrees: %d", (unsigned int)newAdvancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD(TAG, "lockNgoTimeout: %d", (unsigned int)newAdvancedConfig.lockNgoTimeout);
    ESP_LOGD(TAG, "singleButtonPressAction: %d", (unsigned int)newAdvancedConfig.singleButtonPressAction);
    ESP_LOGD(TAG, "doubleButtonPressAction: %d", (unsigned int)newAdvancedConfig.doubleButtonPressAction);
    ESP_LOGD(TAG, "detachedCylinder: %d", (unsigned int)newAdvancedConfig.detachedCylinder);
    ESP_LOGD(TAG, "batteryType: %d", (unsigned int)newAdvancedConfig.batteryType);
    ESP_LOGD(TAG, "automaticBatteryTypeDetection: %d", (unsigned int)newAdvancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD(TAG, "unlatchDuration: %d", (unsigned int)newAdvancedConfig.unlatchDuration);
    ESP_LOGD(TAG, "autoUnLockTimeOut: %d", (unsigned int)newAdvancedConfig.autoLockTimeOut);
    ESP_LOGD(TAG, "autoUnLockDisabled: %d", (unsigned int)newAdvancedConfig.autoUnLockDisabled);
    ESP_LOGD(TAG, "nightModeEnabled: %d", (unsigned int)newAdvancedConfig.nightModeEnabled);
    ESP_LOGD(TAG, "nightModeStartTime Hour: %d", (unsigned int)newAdvancedConfig.nightModeStartTime[0]);
    ESP_LOGD(TAG, "nightModeStartTime Minute: %d", (unsigned int)newAdvancedConfig.nightModeStartTime[1]);
    ESP_LOGD(TAG, "nightModeEndTime Hour: %d", (unsigned int)newAdvancedConfig.nightModeEndTime[0]);
    ESP_LOGD(TAG, "nightModeEndTime Minute: %d", (unsigned int)newAdvancedConfig.nightModeEndTime[1]);
    ESP_LOGD(TAG, "nightModeAutoLockEnabled: %d", (unsigned int)newAdvancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD(TAG, "nightModeAutoUnlockDisabled: %d", (unsigned int)newAdvancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD(TAG, "nightModeImmediateLockOnStart: %d", (unsigned int)newAdvancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD(TAG, "autoLockEnabled: %d", (unsigned int)newAdvancedConfig.autoLockEnabled);
    ESP_LOGD(TAG, "immediateAutoLockEnabled: %d", (unsigned int)newAdvancedConfig.immediateAutoLockEnabled);
    ESP_LOGD(TAG, "autoUpdateEnabled: %d", (unsigned int)newAdvancedConfig.autoUpdateEnabled);
  }
}

void log_wifi_scan_entry(WifiScanEntry wifiScanEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "ssid: %s", (const char*)wifiScanEntry.ssid);
    ESP_LOGD(TAG, "type: %d", (unsigned int)wifiScanEntry.type);
    ESP_LOGD(TAG, "signal raw: %d", (unsigned int)wifiScanEntry.signal);
    ESP_LOGD(TAG, "signal: %d", (unsigned int)(wifiScanEntry.signal & 255));
  }
}

void log_mqtt_config(MqttConfig mqttConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %d", (unsigned int)mqttConfig.enabled);
    ESP_LOGD(TAG, "hostName: %s", (const char*)mqttConfig.hostName);
    ESP_LOGD(TAG, "userName: %s", (const char*)mqttConfig.userName);
    ESP_LOGD(TAG, "secureConnection: %d", (unsigned int)mqttConfig.secureConnection);
    ESP_LOGD(TAG, "autoDiscovery: %d", (unsigned int)mqttConfig.autoDiscovery);
    ESP_LOGD(TAG, "lockingEnabled: %d", (unsigned int)mqttConfig.lockingEnabled);
  }
}

void log_mqtt_config_for_migration(MqttConfigForMigration mqttConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %d", (unsigned int)mqttConfigForMigration.enabled);
    ESP_LOGD(TAG, "hostName: %s", (const char*)mqttConfigForMigration.hostName);
    ESP_LOGD(TAG, "userName: %s", (const char*)mqttConfigForMigration.userName);
    ESP_LOGD(TAG, "secureConnection: %d", (unsigned int)mqttConfigForMigration.secureConnection);
    ESP_LOGD(TAG, "autoDiscovery: %d", (unsigned int)mqttConfigForMigration.autoDiscovery);
    ESP_LOGD(TAG, "lockingEnabled: %d", (unsigned int)mqttConfigForMigration.lockingEnabled);
    ESP_LOGD(TAG, "passphrase: %s", (const char*)mqttConfigForMigration.passphrase);
  }
}

void log_wifi_config(WifiConfig wifiConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "serverBridgeId: %d", (unsigned int)wifiConfig.serverBridgeId);
    ESP_LOGD(TAG, "wifiEnabled: %d", (unsigned int)wifiConfig.wifiEnabled);
    ESP_LOGD(TAG, "wifiExpertSettings composed value: %d", (unsigned int)wifiConfig.wifiExpertSettings);
    ESP_LOGD(TAG, "expertSettingsMode: %d", (unsigned int)(wifiConfig.wifiExpertSettings & 3));
    ESP_LOGD(TAG, "broadcastFilterSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 2) & 3));
    ESP_LOGD(TAG, "dtimSkipSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 4) & 7));
    ESP_LOGD(TAG, "sseSkipSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 7) & 7));
    ESP_LOGD(TAG, "powersafeMode: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 10) & 3));
    ESP_LOGD(TAG, "activePingEnabled: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 12) & 1));
  }
}

void log_wifi_config_for_migration(WifiConfigForMigration wifiConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "ssid: %s", (const char*)wifiConfigForMigration.ssid);
    ESP_LOGD(TAG, "type: %d", (unsigned int)wifiConfigForMigration.type);
    ESP_LOGD(TAG, "passphrase: %s", (const char*)wifiConfigForMigration.passphrase);
  }
}

void log_door_sensor_config(DoorSensorConfig doorSensorConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %d", (unsigned int)doorSensorConfig.enabled);
    ESP_LOGD(TAG, "doorAjarTimeout: %d", (unsigned int)doorSensorConfig.doorAjarTimeout);
    ESP_LOGD(TAG, "doorAjarLoggingEnabled: %d", (unsigned int)doorSensorConfig.doorAjarLoggingEnabled);
    ESP_LOGD(TAG, "doorStatusMismatchLoggingEnabled: %d", (unsigned int)doorSensorConfig.doorStatusMismatchLoggingEnabled);
  }
}

void log_daily_statistics(DailyStatistics dailyStatistics, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "dateYear: %d", (unsigned int)dailyStatistics.dateYear);
    ESP_LOGD(TAG, "dateMonth: %d", (unsigned int)dailyStatistics.dateMonth);
    ESP_LOGD(TAG, "dateDay: %d", (unsigned int)dailyStatistics.dateDay);
    //ESP_LOGD(TAG, "dateHour: %d", (unsigned int)dailyStatistics.dateHour);
    //ESP_LOGD(TAG, "dateMinute: %d", (unsigned int)dailyStatistics.dateMinute);
    //ESP_LOGD(TAG, "dateSecond: %d", (unsigned int)dailyStatistics.dateSecond);
    ESP_LOGD(TAG, "version: %d", (unsigned int)dailyStatistics.version);
    ESP_LOGD(TAG, "countSuccessfulLockActions: %d", (unsigned int)dailyStatistics.countSuccessfulLockActions);
    ESP_LOGD(TAG, "countErroneousLockActions: %d", (unsigned int)dailyStatistics.countErroneousLockActions);
    ESP_LOGD(TAG, "avgCurrentConsumptionLock: %d", (unsigned int)dailyStatistics.avgCurrentConsumptionLock);
    ESP_LOGD(TAG, "maxCurrentConsumptionLock: %d", (unsigned int)dailyStatistics.maxCurrentConsumptionLock);
    ESP_LOGD(TAG, "batteryMinStartVoltageLock: %d", (unsigned int)dailyStatistics.batteryMinStartVoltageLock);
    ESP_LOGD(TAG, "countSuccessfulUnlatchActions: %d", (unsigned int)dailyStatistics.countSuccessfulUnlatchActions);
    ESP_LOGD(TAG, "countErroneousUnlatchActions: %d", (unsigned int)dailyStatistics.countErroneousUnlatchActions);
    ESP_LOGD(TAG, "avgCurrentConsumptionUnlatch: %d", (unsigned int)dailyStatistics.avgCurrentConsumptionUnlatch);
    ESP_LOGD(TAG, "maxCurrentConsumptionUnlatch: %d", (unsigned int)dailyStatistics.maxCurrentConsumptionUnlatch);
    ESP_LOGD(TAG, "batteryMinStartVoltageUnlatch: %d", (unsigned int)dailyStatistics.batteryMinStartVoltageUnlatch);
    ESP_LOGD(TAG, "incomingCommands: %d", (unsigned int)dailyStatistics.incomingCommands);
    ESP_LOGD(TAG, "outgoingCommands: %d", (unsigned int)dailyStatistics.outgoingCommands);
    ESP_LOGD(TAG, "maxTemperature: %d", (unsigned int)dailyStatistics.maxTemperature);
    ESP_LOGD(TAG, "minTemperature: %d", (unsigned int)dailyStatistics.minTemperature);
    ESP_LOGD(TAG, "avgTemperature: %d", (unsigned int)dailyStatistics.avgTemperature);
    ESP_LOGD(TAG, "numDoorSensorStatusChanges: %d", (unsigned int)dailyStatistics.numDoorSensorStatusChanges);
    ESP_LOGD(TAG, "maxBatteryPercentage: %d", (unsigned int)dailyStatistics.maxBatteryPercentage);
    ESP_LOGD(TAG, "minBatteryPercentage: %d", (unsigned int)dailyStatistics.minBatteryPercentage);
    ESP_LOGD(TAG, "idleTime: %d", (unsigned int)dailyStatistics.idleTime);
    ESP_LOGD(TAG, "connectionTime: %d", (unsigned int)dailyStatistics.connectionTime);
    ESP_LOGD(TAG, "actionTime: %d", (unsigned int)dailyStatistics.actionTime);
  }
}

void log_general_statistics(GeneralStatistics generalStatistics, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "version: %d", (unsigned int)generalStatistics.version);
    ESP_LOGD(TAG, "firstCalibrationYear: %d", (unsigned int)generalStatistics.firstCalibrationYear);
    ESP_LOGD(TAG, "firstCalibrationMonth: %d", (unsigned int)generalStatistics.firstCalibrationMonth);
    ESP_LOGD(TAG, "firstCalibrationDay: %d", (unsigned int)generalStatistics.firstCalibrationDay);
    ESP_LOGD(TAG, "calibrationCount: %d", (unsigned int)generalStatistics.calibrationCount);
    ESP_LOGD(TAG, "lockActionCount: %d", (unsigned int)generalStatistics.lockActionCount);
    ESP_LOGD(TAG, "unlatchCount: %d", (unsigned int)generalStatistics.unlatchCount);
    ESP_LOGD(TAG, "lastRebootDateYear: %d", (unsigned int)generalStatistics.lastRebootDateYear);
    ESP_LOGD(TAG, "lastRebootDateMonth: %d", (unsigned int)generalStatistics.lastRebootDateMonth);
    ESP_LOGD(TAG, "lastRebootDateDay: %d", (unsigned int)generalStatistics.lastRebootDateDay);
    ESP_LOGD(TAG, "lastRebootDateHour: %d", (unsigned int)generalStatistics.lastRebootDateHour);
    ESP_LOGD(TAG, "lastRebootDateMinute: %d", (unsigned int)generalStatistics.lastRebootDateMinute);
    ESP_LOGD(TAG, "lastRebootDateSecond: %d", (unsigned int)generalStatistics.lastRebootDateSecond);
    ESP_LOGD(TAG, "lastChargeDateYear: %d", (unsigned int)generalStatistics.lastChargeDateYear);
    ESP_LOGD(TAG, "lastChargeDateMonth: %d", (unsigned int)generalStatistics.lastChargeDateMonth);
    ESP_LOGD(TAG, "lastChargeDateDay: %d", (unsigned int)generalStatistics.lastChargeDateDay);
    ESP_LOGD(TAG, "lastChargeDateHour: %d", (unsigned int)generalStatistics.lastChargeDateHour);
    ESP_LOGD(TAG, "lastChargeDateMinute: %d", (unsigned int)generalStatistics.lastChargeDateMinute);
    ESP_LOGD(TAG, "lastChargeDateSecond: %d", (unsigned int)generalStatistics.lastChargeDateSecond);
    ESP_LOGD(TAG, "initialBatteryVoltage: %d", (unsigned int)generalStatistics.initialBatteryVoltage);
    ESP_LOGD(TAG, "numActionsDuringBatteryCycle: %d", (unsigned int)generalStatistics.numActionsDuringBatteryCycle);
    ESP_LOGD(TAG, "numUnexpectedReboots: %d", (unsigned int)generalStatistics.numUnexpectedReboots);
  }
}

void log_internal_log_entry(InternalLogEntry internalLogEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "[%d] type: %d authId: %d %d-%d-%d %d:%d:%d",
      (unsigned int)internalLogEntry.index,
      (unsigned int)internalLogEntry.loggingType,
      (unsigned int)internalLogEntry.authId,
      internalLogEntry.timeStampYear,
      internalLogEntry.timeStampMonth,
      internalLogEntry.timeStampDay,
      internalLogEntry.timeStampHour,
      internalLogEntry.timeStampMinute,
      internalLogEntry.timeStampSecond
    );
    ESP_LOGD(TAG, "data: %d", (unsigned int)internalLogEntry.data);
  }
}

}  // namespace esphome::nuki_lock