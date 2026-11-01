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
    ESP_LOGD(TAG, "nukiId: %u", (unsigned int)config.nukiId);
    ESP_LOGD(TAG, "name: %s", (const char*)config.name);
    ESP_LOGD(TAG, "latitude: %f", (const float)config.latitude);
    ESP_LOGD(TAG, "longitude: %f", (const float)config.longitude);
    ESP_LOGD(TAG, "autoUnlatch: %u", (unsigned int)config.autoUnlatch);
    ESP_LOGD(TAG, "pairing_enabled: %u", (unsigned int)config.pairing_enabled);
    ESP_LOGD(TAG, "buttonEnabled: %u", (unsigned int)config.buttonEnabled);
    ESP_LOGD(TAG, "ledEnabled: %u", (unsigned int)config.ledEnabled);
    ESP_LOGD(TAG, "ledBrightness: %u", (unsigned int)config.ledBrightness);
    ESP_LOGD(TAG, "currentTime Year: %u", (unsigned int)config.currentTimeYear);
    ESP_LOGD(TAG, "currentTime Month: %u", (unsigned int)config.currentTimeMonth);
    ESP_LOGD(TAG, "currentTime Day: %u", (unsigned int)config.currentTimeDay);
    ESP_LOGD(TAG, "currentTime Hour: %u", (unsigned int)config.currentTimeHour);
    ESP_LOGD(TAG, "currentTime Minute: %u", (unsigned int)config.currentTimeMinute);
    ESP_LOGD(TAG, "currentTime Second: %u", (unsigned int)config.currentTimeSecond);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (int)config.timeZoneOffset);
    ESP_LOGD(TAG, "dstMode: %u", (unsigned int)config.dstMode);
    ESP_LOGD(TAG, "hasFob: %u", (unsigned int)config.hasFob);
    ESP_LOGD(TAG, "fobAction1: %u", (unsigned int)config.fobAction1);
    ESP_LOGD(TAG, "fobAction2: %u", (unsigned int)config.fobAction2);
    ESP_LOGD(TAG, "fobAction3: %u", (unsigned int)config.fobAction3);
    ESP_LOGD(TAG, "singleLock: %u", (unsigned int)config.singleLock);
    ESP_LOGD(TAG, "advertisingMode: %u", (unsigned int)config.advertisingMode);
    ESP_LOGD(TAG, "hasKeypad: %u", (unsigned int)config.hasKeypad);
    ESP_LOGD(TAG, "firmwareVersion: %d.%d.%d", config.firmwareVersion[0], config.firmwareVersion[1], config.firmwareVersion[2]);
    ESP_LOGD(TAG, "hardwareRevision: %d.%d", config.hardwareRevision[0], config.hardwareRevision[1]);
    ESP_LOGD(TAG, "homeKitStatus: %u", (unsigned int)config.homeKitStatus);
    ESP_LOGD(TAG, "timeZoneId: %u", (unsigned int)config.timeZoneId);
    ESP_LOGD(TAG, "deviceType: %u", (unsigned int)config.deviceType);
    ESP_LOGD(TAG, "wifiCapable: %u", (unsigned int)config.capabilities & 1);
    ESP_LOGD(TAG, "threadCapable: %u", (unsigned int)(((unsigned int)config.capabilities & 2) != 0 ? 1:  0));
    ESP_LOGD(TAG, "hasKeypadV2: %u", (unsigned int)config.hasKeypadV2);
    ESP_LOGD(TAG, "matterStatus: %u", (unsigned int)config.matterStatus);
    ESP_LOGD(TAG, "productVariant: %u", (unsigned int)config.productVariant);
  }
}

void log_new_config(NewConfig newConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "name: %s", (const char*)newConfig.name);
    ESP_LOGD(TAG, "latitude: %f", (const float)newConfig.latitude);
    ESP_LOGD(TAG, "longitude: %f", (const float)newConfig.longitude);
    ESP_LOGD(TAG, "autoUnlatch: %u", (unsigned int)newConfig.autoUnlatch);
    ESP_LOGD(TAG, "pairing_enabled: %u", (unsigned int)newConfig.pairing_enabled);
    ESP_LOGD(TAG, "buttonEnabled: %u", (unsigned int)newConfig.buttonEnabled);
    ESP_LOGD(TAG, "ledEnabled: %u", (unsigned int)newConfig.ledEnabled);
    ESP_LOGD(TAG, "ledBrightness: %u", (unsigned int)newConfig.ledBrightness);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (int)newConfig.timeZoneOffset);
    ESP_LOGD(TAG, "dstMode: %u", (unsigned int)newConfig.dstMode);
    ESP_LOGD(TAG, "fobAction1: %u", (unsigned int)newConfig.fobAction1);
    ESP_LOGD(TAG, "fobAction2: %u", (unsigned int)newConfig.fobAction2);
    ESP_LOGD(TAG, "fobAction3: %u", (unsigned int)newConfig.fobAction3);
    ESP_LOGD(TAG, "singleLock: %u", (unsigned int)newConfig.singleLock);
    ESP_LOGD(TAG, "advertisingMode: %u", (unsigned int)newConfig.advertisingMode);
    ESP_LOGD(TAG, "timeZoneId: %u", (unsigned int)newConfig.timeZoneId);
  }
}

void log_new_time_control_entry(NewTimeControlEntry newTimeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "weekdays:%u", (unsigned int)newTimeControlEntry.weekdays);
    ESP_LOGD(TAG, "time:%u:%u", (unsigned int)newTimeControlEntry.timeHour, (unsigned int)newTimeControlEntry.timeMin);
    ESP_LOGD(TAG, "lock_action:%u", (unsigned int)newTimeControlEntry.lock_action);
  }
}

void log_time_control_entry(TimeControlEntry timeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "entryId:%u", (unsigned int)timeControlEntry.entryId);
    ESP_LOGD(TAG, "enabled:%u", (unsigned int)timeControlEntry.enabled);
    ESP_LOGD(TAG, "weekdays:%u", (unsigned int)timeControlEntry.weekdays);
    ESP_LOGD(TAG, "time:%u:%u", (unsigned int)timeControlEntry.timeHour, (unsigned int)timeControlEntry.timeMin);
    ESP_LOGD(TAG, "lock_action:%u", (unsigned int)timeControlEntry.lock_action);
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
    ESP_LOGD(TAG, "lockState: %u", (unsigned int)keyTurnerState.lockState);
    log_nuki_trigger(keyTurnerState.trigger, debug);
    ESP_LOGD(TAG, "currentTimeYear: %u", (unsigned int)keyTurnerState.currentTimeYear);
    ESP_LOGD(TAG, "currentTimeMonth: %u", (unsigned int)keyTurnerState.currentTimeMonth);
    ESP_LOGD(TAG, "currentTimeDay: %u", (unsigned int)keyTurnerState.currentTimeDay);
    ESP_LOGD(TAG, "currentTimeHour: %u", (unsigned int)keyTurnerState.currentTimeHour);
    ESP_LOGD(TAG, "currentTimeMinute: %u", (unsigned int)keyTurnerState.currentTimeMinute);
    ESP_LOGD(TAG, "currentTimeSecond: %u", (unsigned int)keyTurnerState.currentTimeSecond);
    ESP_LOGD(TAG, "timeZoneOffset: %d", (int)keyTurnerState.timeZoneOffset);
    ESP_LOGD(TAG, "criticalBatteryState composed value: %u", (unsigned int)keyTurnerState.criticalBatteryState);
    ESP_LOGD(TAG, "criticalBatteryState: %u", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState) == 1 ? 1 : 0));
    ESP_LOGD(TAG, "batteryCharging: %u", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState & 2) == 2 ? 1 : 0));
    ESP_LOGD(TAG, "batteryPercent: %u", (unsigned int)((keyTurnerState.criticalBatteryState & 0b11111100) >> 1));
    ESP_LOGD(TAG, "configUpdateCount: %u", (unsigned int)keyTurnerState.configUpdateCount);
    ESP_LOGD(TAG, "lockNgoTimer: %u", (unsigned int)keyTurnerState.lockNgoTimer);
    log_lock_action((LockAction)keyTurnerState.lastLockAction, debug);
    ESP_LOGD(TAG, "lastLockActionTrigger: %u", (unsigned int)keyTurnerState.lastLockActionTrigger);
    log_completion_status(keyTurnerState.lastLockActionCompletionStatus, debug);
    ESP_LOGD(TAG, "doorSensorState: %u", (unsigned int)keyTurnerState.doorSensorState);
    ESP_LOGD(TAG, "nightModeActive: %u", (unsigned int)keyTurnerState.nightModeActive);
    ESP_LOGD(TAG, "accessoryBatteryState composed value: %u", (unsigned int)keyTurnerState.accessoryBatteryState);
    ESP_LOGD(TAG, "Keypad bat critical feature supported: %u", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 1) == 1 ? 1 : 0));
    ESP_LOGD(TAG, "Keypad Battery Critical: %u", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 3) == 3 ? 1 : 0));
    ESP_LOGD(TAG, "Doorsensor bat critical feature supported: %u", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 4) == 4 ? 1 : 0));
    ESP_LOGD(TAG, "Doorsensor Battery Critical: %u", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 12) == 12 ? 1 : 0));
    ESP_LOGD(TAG, "remoteAccessStatus composed value: %u", (unsigned int)keyTurnerState.remoteAccessStatus);
    ESP_LOGD(TAG, "remoteAccessEnabled: %u", (unsigned int)(((keyTurnerState.remoteAccessStatus & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "bridgePaired: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 1) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "sseConnectedViaWifi: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 2) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "sseConnectionEstablished: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 3) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "isSseConnectedViaThread: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 4) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "threadSseUplinkEnabledByUser: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 5) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "nat64AvailableViaThread: %u", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 6) & 1) == 1) ? 1 : 0));
    ESP_LOGD(TAG, "bleConnectionStrength: %d", (int)keyTurnerState.bleConnectionStrength);
    ESP_LOGD(TAG, "wifiConnectionStrength: %d", (int)keyTurnerState.wifiConnectionStrength);
    ESP_LOGD(TAG, "wifiConnectionStatus composed value: %u", (unsigned int)keyTurnerState.wifiConnectionStatus);
    ESP_LOGD(TAG, "wifiStatus: %u", (unsigned int)(keyTurnerState.wifiConnectionStatus & 3));
    ESP_LOGD(TAG, "sseStatus: %u", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 2) & 3));
    ESP_LOGD(TAG, "wifiQuality: %u", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 4) & 15));
    ESP_LOGD(TAG, "mqttConnectionStatus composed value: %u", (unsigned int)keyTurnerState.mqttConnectionStatus);
    ESP_LOGD(TAG, "mqttStatus: %u", (unsigned int)(keyTurnerState.mqttConnectionStatus & 3));
    ESP_LOGD(TAG, "mqttConnectionChannel: %u", (unsigned int)((keyTurnerState.mqttConnectionStatus >> 2) & 1));
    ESP_LOGD(TAG, "threadConnectionStatus composed value: %u", (unsigned int)keyTurnerState.threadConnectionStatus);
    ESP_LOGD(TAG, "threadConnectionStatus: %u", (unsigned int)(keyTurnerState.threadConnectionStatus & 3));
    ESP_LOGD(TAG, "threadSseStatus: %u", (unsigned int)((keyTurnerState.threadConnectionStatus >> 2) & 3));
    ESP_LOGD(TAG, "isCommissioningModeActive: %u", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 16) != 0 ? 1 : 0));
    ESP_LOGD(TAG, "isWifiDisabledBecauseOfThread: %u", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 32) != 0 ? 1 : 0));
  }
}

void log_battery_report(BatteryReport batteryReport, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "batteryDrain:%u", (unsigned int)batteryReport.batteryDrain);
    ESP_LOGD(TAG, "batteryVoltage:%u", (unsigned int)batteryReport.batteryVoltage);
    ESP_LOGD(TAG, "criticalBatteryState:%u", (unsigned int)batteryReport.criticalBatteryState);
    ESP_LOGD(TAG, "lock_action:%u", (unsigned int)batteryReport.lock_action);
    ESP_LOGD(TAG, "startVoltage:%u", (unsigned int)batteryReport.startVoltage);
    ESP_LOGD(TAG, "lowestVoltage:%u", (unsigned int)batteryReport.lowestVoltage);
    ESP_LOGD(TAG, "lockDistance:%u", (unsigned int)batteryReport.lockDistance);
    ESP_LOGD(TAG, "startTemperature:%d", (int)batteryReport.startTemperature);
    ESP_LOGD(TAG, "maxTurnCurrent:%u", (unsigned int)batteryReport.maxTurnCurrent);
    ESP_LOGD(TAG, "batteryResistance:%u", (unsigned int)batteryReport.batteryResistance);
  }
}

void log_log_entry(LogEntry logEntry, bool debug) {
  ESP_LOGD(TAG, "[%lu] type: %u authId: %lu name: %s %d-%d-%d %d:%d:%d ", logEntry.index, (uint8_t)logEntry.loggingType, logEntry.authId, logEntry.name, logEntry.timeStampYear, logEntry.timeStampMonth, logEntry.timeStampDay, logEntry.timeStampHour, logEntry.timeStampMinute, logEntry.timeStampSecond);


  switch (logEntry.loggingType) {
    case LoggingType::LoggingEnabled: {
      ESP_LOGD(TAG, "Logging enabled: %u", (unsigned int)logEntry.data[0]);
      break;
    }
    case LoggingType::LockAction:
    case LoggingType::Calibration:
    case LoggingType::InitializationRun: {
      log_lock_action((LockAction)logEntry.data[0], debug);
      log_nuki_trigger((NukiTrigger)logEntry.data[1], debug);
      ESP_LOGD(TAG, "Flags: %u", (unsigned int)logEntry.data[2]);
      log_completion_status((CompletionStatus)logEntry.data[3], debug);
      break;
    }
    case LoggingType::KeypadAction: {
      log_lock_action((LockAction)logEntry.data[0], debug);
      ESP_LOGD(TAG, "Source: %u", (unsigned int)logEntry.data[1]);
      log_completion_status((CompletionStatus)logEntry.data[2], debug);
      uint16_t codeId = 0;
      memcpy(&codeId, &logEntry.data[3], 2);
      ESP_LOGD(TAG, "Code id: %u", (unsigned int)codeId);
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
      ESP_LOGD(TAG, "Logging enabled: %u", (unsigned int)logEntry.data[0]);
      break;
    }
    default:
      ESP_LOGW(TAG, "Unknown logging type");
      break;
  }
}

void log_advanced_config(AdvancedConfig advancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "totalDegrees: %u", (unsigned int)advancedConfig.totalDegrees);
    ESP_LOGD(TAG, "unlockedPositionOffsetDegrees: %d", (int)advancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "lockedPositionOffsetDegrees: %f", (const float)advancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "singleLockedPositionOffsetDegrees: %f", (const float)advancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "unlockedToLockedTransitionOffsetDegrees: %d", (int)advancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD(TAG, "lockNgoTimeout: %u", (unsigned int)advancedConfig.lockNgoTimeout);
    ESP_LOGD(TAG, "singleButtonPressAction: %u", (unsigned int)advancedConfig.singleButtonPressAction);
    ESP_LOGD(TAG, "doubleButtonPressAction: %u", (unsigned int)advancedConfig.doubleButtonPressAction);
    ESP_LOGD(TAG, "detachedCylinder: %u", (unsigned int)advancedConfig.detachedCylinder);
    ESP_LOGD(TAG, "batteryType: %u", (unsigned int)advancedConfig.batteryType);
    ESP_LOGD(TAG, "automaticBatteryTypeDetection: %u", (unsigned int)advancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD(TAG, "unlatchDuration: %u", (unsigned int)advancedConfig.unlatchDuration);
    ESP_LOGD(TAG, "autoLockTimeOut: %u", (unsigned int)advancedConfig.autoLockTimeOut);
    ESP_LOGD(TAG, "autoUnLockDisabled: %u", (unsigned int)advancedConfig.autoUnLockDisabled);
    ESP_LOGD(TAG, "nightModeEnabled: %u", (unsigned int)advancedConfig.nightModeEnabled);
    ESP_LOGD(TAG, "nightModeStartTime Hour: %u", (unsigned int)advancedConfig.nightModeStartTime[0]);
    ESP_LOGD(TAG, "nightModeStartTime Minute: %u", (unsigned int)advancedConfig.nightModeStartTime[1]);
    ESP_LOGD(TAG, "nightModeEndTime Hour: %u", (unsigned int)advancedConfig.nightModeEndTime[0]);
    ESP_LOGD(TAG, "nightModeEndTime Minute: %u", (unsigned int)advancedConfig.nightModeEndTime[1]);
    ESP_LOGD(TAG, "nightModeAutoLockEnabled: %u", (unsigned int)advancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD(TAG, "nightModeAutoUnlockDisabled: %u", (unsigned int)advancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD(TAG, "nightModeImmediateLockOnStart: %u", (unsigned int)advancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD(TAG, "autoLockEnabled: %u", (unsigned int)advancedConfig.autoLockEnabled);
    ESP_LOGD(TAG, "immediateAutoLockEnabled: %u", (unsigned int)advancedConfig.immediateAutoLockEnabled);
    ESP_LOGD(TAG, "autoUpdateEnabled: %u", (unsigned int)advancedConfig.autoUpdateEnabled);
    ESP_LOGD(TAG, "motorSpeed: %u", (unsigned int)advancedConfig.motorSpeed);
    ESP_LOGD(TAG, "enable_slow_speed_during_night_mode: %u", (unsigned int)advancedConfig.enable_slow_speed_during_night_mode);
  }
}

void log_new_advanced_config(NewAdvancedConfig newAdvancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "unlockedPositionOffsetDegrees: %d", (int)newAdvancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "lockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "singleLockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD(TAG, "unlockedToLockedTransitionOffsetDegrees: %d", (int)newAdvancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD(TAG, "lockNgoTimeout: %u", (unsigned int)newAdvancedConfig.lockNgoTimeout);
    ESP_LOGD(TAG, "singleButtonPressAction: %u", (unsigned int)newAdvancedConfig.singleButtonPressAction);
    ESP_LOGD(TAG, "doubleButtonPressAction: %u", (unsigned int)newAdvancedConfig.doubleButtonPressAction);
    ESP_LOGD(TAG, "detachedCylinder: %u", (unsigned int)newAdvancedConfig.detachedCylinder);
    ESP_LOGD(TAG, "batteryType: %u", (unsigned int)newAdvancedConfig.batteryType);
    ESP_LOGD(TAG, "automaticBatteryTypeDetection: %u", (unsigned int)newAdvancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD(TAG, "unlatchDuration: %u", (unsigned int)newAdvancedConfig.unlatchDuration);
    ESP_LOGD(TAG, "autoUnLockTimeOut: %u", (unsigned int)newAdvancedConfig.autoLockTimeOut);
    ESP_LOGD(TAG, "autoUnLockDisabled: %u", (unsigned int)newAdvancedConfig.autoUnLockDisabled);
    ESP_LOGD(TAG, "nightModeEnabled: %u", (unsigned int)newAdvancedConfig.nightModeEnabled);
    ESP_LOGD(TAG, "nightModeStartTime Hour: %u", (unsigned int)newAdvancedConfig.nightModeStartTime[0]);
    ESP_LOGD(TAG, "nightModeStartTime Minute: %u", (unsigned int)newAdvancedConfig.nightModeStartTime[1]);
    ESP_LOGD(TAG, "nightModeEndTime Hour: %u", (unsigned int)newAdvancedConfig.nightModeEndTime[0]);
    ESP_LOGD(TAG, "nightModeEndTime Minute: %u", (unsigned int)newAdvancedConfig.nightModeEndTime[1]);
    ESP_LOGD(TAG, "nightModeAutoLockEnabled: %u", (unsigned int)newAdvancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD(TAG, "nightModeAutoUnlockDisabled: %u", (unsigned int)newAdvancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD(TAG, "nightModeImmediateLockOnStart: %u", (unsigned int)newAdvancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD(TAG, "autoLockEnabled: %u", (unsigned int)newAdvancedConfig.autoLockEnabled);
    ESP_LOGD(TAG, "immediateAutoLockEnabled: %u", (unsigned int)newAdvancedConfig.immediateAutoLockEnabled);
    ESP_LOGD(TAG, "autoUpdateEnabled: %u", (unsigned int)newAdvancedConfig.autoUpdateEnabled);
  }
}

void log_wifi_scan_entry(WifiScanEntry wifiScanEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "ssid: %s", (const char*)wifiScanEntry.ssid);
    ESP_LOGD(TAG, "type: %u", (unsigned int)wifiScanEntry.type);
    ESP_LOGD(TAG, "signal raw: %u", (unsigned int)wifiScanEntry.signal);
    ESP_LOGD(TAG, "signal: %u", (unsigned int)(wifiScanEntry.signal & 255));
  }
}

void log_mqtt_config(MqttConfig mqttConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %u", (unsigned int)mqttConfig.enabled);
    ESP_LOGD(TAG, "hostName: %s", (const char*)mqttConfig.hostName);
    ESP_LOGD(TAG, "userName: %s", (const char*)mqttConfig.userName);
    ESP_LOGD(TAG, "secureConnection: %u", (unsigned int)mqttConfig.secureConnection);
    ESP_LOGD(TAG, "autoDiscovery: %u", (unsigned int)mqttConfig.autoDiscovery);
    ESP_LOGD(TAG, "lockingEnabled: %u", (unsigned int)mqttConfig.lockingEnabled);
  }
}

void log_mqtt_config_for_migration(MqttConfigForMigration mqttConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %u", (unsigned int)mqttConfigForMigration.enabled);
    ESP_LOGD(TAG, "hostName: %s", (const char*)mqttConfigForMigration.hostName);
    ESP_LOGD(TAG, "userName: %s", (const char*)mqttConfigForMigration.userName);
    ESP_LOGD(TAG, "secureConnection: %u", (unsigned int)mqttConfigForMigration.secureConnection);
    ESP_LOGD(TAG, "autoDiscovery: %u", (unsigned int)mqttConfigForMigration.autoDiscovery);
    ESP_LOGD(TAG, "lockingEnabled: %u", (unsigned int)mqttConfigForMigration.lockingEnabled);
    ESP_LOGD(TAG, "passphrase: %s", (const char*)mqttConfigForMigration.passphrase);
  }
}

void log_wifi_config(WifiConfig wifiConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "serverBridgeId: %u", (unsigned int)wifiConfig.serverBridgeId);
    ESP_LOGD(TAG, "wifiEnabled: %u", (unsigned int)wifiConfig.wifiEnabled);
    ESP_LOGD(TAG, "wifiExpertSettings composed value: %u", (unsigned int)wifiConfig.wifiExpertSettings);
    ESP_LOGD(TAG, "expertSettingsMode: %u", (unsigned int)(wifiConfig.wifiExpertSettings & 3));
    ESP_LOGD(TAG, "broadcastFilterSettings: %u", (unsigned int)((wifiConfig.wifiExpertSettings >> 2) & 3));
    ESP_LOGD(TAG, "dtimSkipSettings: %u", (unsigned int)((wifiConfig.wifiExpertSettings >> 4) & 7));
    ESP_LOGD(TAG, "sseSkipSettings: %u", (unsigned int)((wifiConfig.wifiExpertSettings >> 7) & 7));
    ESP_LOGD(TAG, "powersafeMode: %u", (unsigned int)((wifiConfig.wifiExpertSettings >> 10) & 3));
    ESP_LOGD(TAG, "activePingEnabled: %u", (unsigned int)((wifiConfig.wifiExpertSettings >> 12) & 1));
  }
}

void log_wifi_config_for_migration(WifiConfigForMigration wifiConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "ssid: %s", (const char*)wifiConfigForMigration.ssid);
    ESP_LOGD(TAG, "type: %u", (unsigned int)wifiConfigForMigration.type);
    ESP_LOGD(TAG, "passphrase: %s", (const char*)wifiConfigForMigration.passphrase);
  }
}

void log_door_sensor_config(DoorSensorConfig doorSensorConfig, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "enabled: %u", (unsigned int)doorSensorConfig.enabled);
    ESP_LOGD(TAG, "doorAjarTimeout: %u", (unsigned int)doorSensorConfig.doorAjarTimeout);
    ESP_LOGD(TAG, "doorAjarLoggingEnabled: %u", (unsigned int)doorSensorConfig.doorAjarLoggingEnabled);
    ESP_LOGD(TAG, "doorStatusMismatchLoggingEnabled: %u", (unsigned int)doorSensorConfig.doorStatusMismatchLoggingEnabled);
  }
}

void log_daily_statistics(DailyStatistics dailyStatistics, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "dateYear: %u", (unsigned int)dailyStatistics.dateYear);
    ESP_LOGD(TAG, "dateMonth: %u", (unsigned int)dailyStatistics.dateMonth);
    ESP_LOGD(TAG, "dateDay: %u", (unsigned int)dailyStatistics.dateDay);
    //ESP_LOGD(TAG, "dateHour: %u", (unsigned int)dailyStatistics.dateHour);
    //ESP_LOGD(TAG, "dateMinute: %u", (unsigned int)dailyStatistics.dateMinute);
    //ESP_LOGD(TAG, "dateSecond: %u", (unsigned int)dailyStatistics.dateSecond);
    ESP_LOGD(TAG, "version: %u", (unsigned int)dailyStatistics.version);
    ESP_LOGD(TAG, "countSuccessfulLockActions: %u", (unsigned int)dailyStatistics.countSuccessfulLockActions);
    ESP_LOGD(TAG, "countErroneousLockActions: %u", (unsigned int)dailyStatistics.countErroneousLockActions);
    ESP_LOGD(TAG, "avgCurrentConsumptionLock: %u", (unsigned int)dailyStatistics.avgCurrentConsumptionLock);
    ESP_LOGD(TAG, "maxCurrentConsumptionLock: %u", (unsigned int)dailyStatistics.maxCurrentConsumptionLock);
    ESP_LOGD(TAG, "batteryMinStartVoltageLock: %u", (unsigned int)dailyStatistics.batteryMinStartVoltageLock);
    ESP_LOGD(TAG, "countSuccessfulUnlatchActions: %u", (unsigned int)dailyStatistics.countSuccessfulUnlatchActions);
    ESP_LOGD(TAG, "countErroneousUnlatchActions: %u", (unsigned int)dailyStatistics.countErroneousUnlatchActions);
    ESP_LOGD(TAG, "avgCurrentConsumptionUnlatch: %u", (unsigned int)dailyStatistics.avgCurrentConsumptionUnlatch);
    ESP_LOGD(TAG, "maxCurrentConsumptionUnlatch: %u", (unsigned int)dailyStatistics.maxCurrentConsumptionUnlatch);
    ESP_LOGD(TAG, "batteryMinStartVoltageUnlatch: %u", (unsigned int)dailyStatistics.batteryMinStartVoltageUnlatch);
    ESP_LOGD(TAG, "incomingCommands: %u", (unsigned int)dailyStatistics.incomingCommands);
    ESP_LOGD(TAG, "outgoingCommands: %u", (unsigned int)dailyStatistics.outgoingCommands);
    ESP_LOGD(TAG, "maxTemperature: %u", (unsigned int)dailyStatistics.maxTemperature);
    ESP_LOGD(TAG, "minTemperature: %u", (unsigned int)dailyStatistics.minTemperature);
    ESP_LOGD(TAG, "avgTemperature: %u", (unsigned int)dailyStatistics.avgTemperature);
    ESP_LOGD(TAG, "numDoorSensorStatusChanges: %u", (unsigned int)dailyStatistics.numDoorSensorStatusChanges);
    ESP_LOGD(TAG, "maxBatteryPercentage: %u", (unsigned int)dailyStatistics.maxBatteryPercentage);
    ESP_LOGD(TAG, "minBatteryPercentage: %u", (unsigned int)dailyStatistics.minBatteryPercentage);
    ESP_LOGD(TAG, "idleTime: %u", (unsigned int)dailyStatistics.idleTime);
    ESP_LOGD(TAG, "connectionTime: %u", (unsigned int)dailyStatistics.connectionTime);
    ESP_LOGD(TAG, "actionTime: %u", (unsigned int)dailyStatistics.actionTime);
  }
}

void log_general_statistics(GeneralStatistics generalStatistics, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "version: %u", (unsigned int)generalStatistics.version);
    ESP_LOGD(TAG, "firstCalibrationYear: %u", (unsigned int)generalStatistics.firstCalibrationYear);
    ESP_LOGD(TAG, "firstCalibrationMonth: %u", (unsigned int)generalStatistics.firstCalibrationMonth);
    ESP_LOGD(TAG, "firstCalibrationDay: %u", (unsigned int)generalStatistics.firstCalibrationDay);
    ESP_LOGD(TAG, "calibrationCount: %u", (unsigned int)generalStatistics.calibrationCount);
    ESP_LOGD(TAG, "lockActionCount: %u", (unsigned int)generalStatistics.lockActionCount);
    ESP_LOGD(TAG, "unlatchCount: %u", (unsigned int)generalStatistics.unlatchCount);
    ESP_LOGD(TAG, "lastRebootDateYear: %u", (unsigned int)generalStatistics.lastRebootDateYear);
    ESP_LOGD(TAG, "lastRebootDateMonth: %u", (unsigned int)generalStatistics.lastRebootDateMonth);
    ESP_LOGD(TAG, "lastRebootDateDay: %u", (unsigned int)generalStatistics.lastRebootDateDay);
    ESP_LOGD(TAG, "lastRebootDateHour: %u", (unsigned int)generalStatistics.lastRebootDateHour);
    ESP_LOGD(TAG, "lastRebootDateMinute: %u", (unsigned int)generalStatistics.lastRebootDateMinute);
    ESP_LOGD(TAG, "lastRebootDateSecond: %u", (unsigned int)generalStatistics.lastRebootDateSecond);
    ESP_LOGD(TAG, "lastChargeDateYear: %u", (unsigned int)generalStatistics.lastChargeDateYear);
    ESP_LOGD(TAG, "lastChargeDateMonth: %u", (unsigned int)generalStatistics.lastChargeDateMonth);
    ESP_LOGD(TAG, "lastChargeDateDay: %u", (unsigned int)generalStatistics.lastChargeDateDay);
    ESP_LOGD(TAG, "lastChargeDateHour: %u", (unsigned int)generalStatistics.lastChargeDateHour);
    ESP_LOGD(TAG, "lastChargeDateMinute: %u", (unsigned int)generalStatistics.lastChargeDateMinute);
    ESP_LOGD(TAG, "lastChargeDateSecond: %u", (unsigned int)generalStatistics.lastChargeDateSecond);
    ESP_LOGD(TAG, "initialBatteryVoltage: %u", (unsigned int)generalStatistics.initialBatteryVoltage);
    ESP_LOGD(TAG, "numActionsDuringBatteryCycle: %u", (unsigned int)generalStatistics.numActionsDuringBatteryCycle);
    ESP_LOGD(TAG, "numUnexpectedReboots: %u", (unsigned int)generalStatistics.numUnexpectedReboots);
  }
}

void log_internal_log_entry(InternalLogEntry internalLogEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "[%u] type: %u authId: %u %d-%d-%d %d:%d:%d",
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
    ESP_LOGD(TAG, "data: %u", (unsigned int)internalLogEntry.data);
  }
}

}  // namespace esphome::nuki_lock