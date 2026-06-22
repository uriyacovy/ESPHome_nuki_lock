/**
 * @file NukiUtills.cpp
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

#include "NukiLockUtils.h"

#include "esp_log.h"
#include <cstring>
#include <cstdint>


namespace NukiLock {
void cmdResultToString(const CmdResult state, char* str) {
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

void logLockErrorCode(uint8_t errorCode, bool debug) {
  if (debug) {
    switch (errorCode) {
      case (uint8_t)ErrorCode::ERROR_BAD_CRC :
        ESP_LOGE("NukiBle.NukiLock", "ERROR_BAD_CRC");
        break;
      case (uint8_t)ErrorCode::ERROR_BAD_LENGTH :
        ESP_LOGE("NukiBle.NukiLock", "ERROR_BAD_LENGTH");
        break;
      case (uint8_t)ErrorCode::ERROR_UNKNOWN :
        ESP_LOGE("NukiBle.NukiLock", "ERROR_UNKNOWN");
        break;
      case (uint8_t)ErrorCode::P_ERROR_NOT_PAIRING :
        ESP_LOGE("NukiBle.NukiLock", "P_ERROR_NOT_PAIRING");
        break;
      case (uint8_t)ErrorCode::P_ERROR_BAD_AUTHENTICATOR :
        ESP_LOGE("NukiBle.NukiLock", "P_ERROR_BAD_AUTHENTICATOR");
        break;
      case (uint8_t)ErrorCode::P_ERROR_BAD_PARAMETER :
        ESP_LOGE("NukiBle.NukiLock", "P_ERROR_BAD_PARAMETER");
        break;
      case (uint8_t)ErrorCode::P_ERROR_MAX_USER :
        ESP_LOGE("NukiBle.NukiLock", "P_ERROR_MAX_USER");
        break;
      case (uint8_t)ErrorCode::K_ERROR_AUTO_UNLOCK_TOO_RECENT :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_AUTO_UNLOCK_TOO_RECENT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_NONCE :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_BAD_NONCE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_PARAMETER :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_BAD_PARAMETER");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BAD_PIN :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_BAD_PIN");
        break;
      case (uint8_t)ErrorCode::K_ERROR_BUSY :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_BUSY");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CANCELED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CANCELED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CLUTCH_FAILURE :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CLUTCH_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CLUTCH_POWER_FAILURE :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CLUTCH_POWER_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_ALREADY_EXISTS :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CODE_ALREADY_EXISTS");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CODE_INVALID");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_1 :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CODE_INVALID_TIMEOUT_1");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_2 :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CODE_INVALID_TIMEOUT_2");
        break;
      case (uint8_t)ErrorCode::K_ERROR_CODE_INVALID_TIMEOUT_3 :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_CODE_INVALID_TIMEOUT_3");
        break;
      case (uint8_t)ErrorCode::K_ERROR_DISABLED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_DISABLED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_FIRMWARE_UPDATE_NEEDED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_FIRMWARE_UPDATE_NEEDED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_INVALID_AUTH_ID :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_INVALID_AUTH_ID");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_BLOCKED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_MOTOR_BLOCKED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_LOW_VOLTAGE :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_MOTOR_LOW_VOLTAGE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_POSITION_LIMIT :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_MOTOR_POSITION_LIMIT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_POWER_FAILURE :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_MOTOR_POWER_FAILURE");
        break;
      case (uint8_t)ErrorCode::K_ERROR_MOTOR_TIMEOUT :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_MOTOR_TIMEOUT");
        break;
      case (uint8_t)ErrorCode::K_ERROR_NOT_AUTHORIZED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_NOT_AUTHORIZED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_NOT_CALIBRATED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_NOT_CALIBRATED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_POSITION_UNKNOWN :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_POSITION_UNKNOWN");
        break;
      case (uint8_t)ErrorCode::K_ERROR_REMOTE_NOT_ALLOWED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_REMOTE_NOT_ALLOWED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TIME_NOT_ALLOWED :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_TIME_NOT_ALLOWED");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TOO_MANY_ENTRIES :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_TOO_MANY_ENTRIES");
        break;
      case (uint8_t)ErrorCode::K_ERROR_TOO_MANY_PIN_ATTEMPTS :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_TOO_MANY_PIN_ATTEMPTS");
        break;
      case (uint8_t)ErrorCode::K_ERROR_VOLTAGE_TOO_LOW :
        ESP_LOGE("NukiBle.NukiLock", "K_ERROR_VOLTAGE_TOO_LOW");
        break;
      default:
        ESP_LOGE("NukiBle.NukiLock", "UNDEFINED ERROR");
    }
  }
}

void logConfig(Config config, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "nukiId: %d", (unsigned int)config.nukiId);
    ESP_LOGD("NukiBle.NukiLock", "name: %s", (const char*)config.name);
    ESP_LOGD("NukiBle.NukiLock", "latitude: %f", (const float)config.latitude);
    ESP_LOGD("NukiBle.NukiLock", "longitude: %f", (const float)config.longitude);
    ESP_LOGD("NukiBle.NukiLock", "autoUnlatch: %d", (unsigned int)config.autoUnlatch);
    ESP_LOGD("NukiBle.NukiLock", "pairingEnabled: %d", (unsigned int)config.pairingEnabled);
    ESP_LOGD("NukiBle.NukiLock", "buttonEnabled: %d", (unsigned int)config.buttonEnabled);
    ESP_LOGD("NukiBle.NukiLock", "ledEnabled: %d", (unsigned int)config.ledEnabled);
    ESP_LOGD("NukiBle.NukiLock", "ledBrightness: %d", (unsigned int)config.ledBrightness);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Year: %d", (unsigned int)config.currentTimeYear);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Month: %d", (unsigned int)config.currentTimeMonth);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Day: %d", (unsigned int)config.currentTimeDay);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Hour: %d", (unsigned int)config.currentTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Minute: %d", (unsigned int)config.currentTimeMinute);
    ESP_LOGD("NukiBle.NukiLock", "currentTime Second: %d", (unsigned int)config.currentTimeSecond);
    ESP_LOGD("NukiBle.NukiLock", "timeZoneOffset: %d", (unsigned int)config.timeZoneOffset);
    ESP_LOGD("NukiBle.NukiLock", "dstMode: %d", (unsigned int)config.dstMode);
    ESP_LOGD("NukiBle.NukiLock", "hasFob: %d", (unsigned int)config.hasFob);
    ESP_LOGD("NukiBle.NukiLock", "fobAction1: %d", (unsigned int)config.fobAction1);
    ESP_LOGD("NukiBle.NukiLock", "fobAction2: %d", (unsigned int)config.fobAction2);
    ESP_LOGD("NukiBle.NukiLock", "fobAction3: %d", (unsigned int)config.fobAction3);
    ESP_LOGD("NukiBle.NukiLock", "singleLock: %d", (unsigned int)config.singleLock);
    ESP_LOGD("NukiBle.NukiLock", "advertisingMode: %d", (unsigned int)config.advertisingMode);
    ESP_LOGD("NukiBle.NukiLock", "hasKeypad: %d", (unsigned int)config.hasKeypad);
    ESP_LOGD("NukiBle.NukiLock", "firmwareVersion: %d.%d.%d", config.firmwareVersion[0], config.firmwareVersion[1], config.firmwareVersion[2]);
    ESP_LOGD("NukiBle.NukiLock", "hardwareRevision: %d.%d", config.hardwareRevision[0], config.hardwareRevision[1]);
    ESP_LOGD("NukiBle.NukiLock", "homeKitStatus: %d", (unsigned int)config.homeKitStatus);
    ESP_LOGD("NukiBle.NukiLock", "timeZoneId: %d", (unsigned int)config.timeZoneId);
    ESP_LOGD("NukiBle.NukiLock", "deviceType: %d", (unsigned int)config.deviceType);
    ESP_LOGD("NukiBle.NukiLock", "wifiCapable: %d", (unsigned int)config.capabilities & 1);
    ESP_LOGD("NukiBle.NukiLock", "threadCapable: %d", (unsigned int)(((unsigned int)config.capabilities & 2) != 0 ? 1:  0));
    ESP_LOGD("NukiBle.NukiLock", "hasKeypadV2: %d", (unsigned int)config.hasKeypadV2);
    ESP_LOGD("NukiBle.NukiLock", "matterStatus: %d", (unsigned int)config.matterStatus);
    ESP_LOGD("NukiBle.NukiLock", "productVariant: %d", (unsigned int)config.productVariant);
  }
}

void logNewConfig(NewConfig newConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "name: %s", (const char*)newConfig.name);
    ESP_LOGD("NukiBle.NukiLock", "latitude: %f", (const float)newConfig.latitude);
    ESP_LOGD("NukiBle.NukiLock", "longitude: %f", (const float)newConfig.longitude);
    ESP_LOGD("NukiBle.NukiLock", "autoUnlatch: %d", (unsigned int)newConfig.autoUnlatch);
    ESP_LOGD("NukiBle.NukiLock", "pairingEnabled: %d", (unsigned int)newConfig.pairingEnabled);
    ESP_LOGD("NukiBle.NukiLock", "buttonEnabled: %d", (unsigned int)newConfig.buttonEnabled);
    ESP_LOGD("NukiBle.NukiLock", "ledEnabled: %d", (unsigned int)newConfig.ledEnabled);
    ESP_LOGD("NukiBle.NukiLock", "ledBrightness: %d", (unsigned int)newConfig.ledBrightness);
    ESP_LOGD("NukiBle.NukiLock", "timeZoneOffset: %d", (unsigned int)newConfig.timeZoneOffset);
    ESP_LOGD("NukiBle.NukiLock", "dstMode: %d", (unsigned int)newConfig.dstMode);
    ESP_LOGD("NukiBle.NukiLock", "fobAction1: %d", (unsigned int)newConfig.fobAction1);
    ESP_LOGD("NukiBle.NukiLock", "fobAction2: %d", (unsigned int)newConfig.fobAction2);
    ESP_LOGD("NukiBle.NukiLock", "fobAction3: %d", (unsigned int)newConfig.fobAction3);
    ESP_LOGD("NukiBle.NukiLock", "singleLock: %d", (unsigned int)newConfig.singleLock);
    ESP_LOGD("NukiBle.NukiLock", "advertisingMode: %d", (unsigned int)newConfig.advertisingMode);
    ESP_LOGD("NukiBle.NukiLock", "timeZoneId: %d", (unsigned int)newConfig.timeZoneId);
  }
}

void logNewKeypadEntry(NewKeypadEntry newKeypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "code:%d", (unsigned int)newKeypadEntry.code);
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)newKeypadEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)newKeypadEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)newKeypadEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)newKeypadEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)newKeypadEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)newKeypadEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)newKeypadEntry.allowedFromMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)newKeypadEntry.allowedFromSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)newKeypadEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)newKeypadEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)newKeypadEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)newKeypadEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)newKeypadEntry.allowedUntilMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)newKeypadEntry.allowedUntilSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)newKeypadEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)newKeypadEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)newKeypadEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)newKeypadEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)newKeypadEntry.allowedUntilTimeMin);
  }
}

void logKeypadEntry(KeypadEntry keypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "codeId:%d", (unsigned int)keypadEntry.codeId);
    ESP_LOGD("NukiBle.NukiLock", "code:%d", (unsigned int)keypadEntry.code);
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)keypadEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "enabled:%d", (unsigned int)keypadEntry.enabled);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedYear:%d", (unsigned int)keypadEntry.dateCreatedYear);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedMonth:%d", (unsigned int)keypadEntry.dateCreatedMonth);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedDay:%d", (unsigned int)keypadEntry.dateCreatedDay);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedHour:%d", (unsigned int)keypadEntry.dateCreatedHour);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedMin:%d", (unsigned int)keypadEntry.dateCreatedMin);
    ESP_LOGD("NukiBle.NukiLock", "dateCreatedSec:%d", (unsigned int)keypadEntry.dateCreatedSec);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveYear:%d", (unsigned int)keypadEntry.dateLastActiveYear);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveMonth:%d", (unsigned int)keypadEntry.dateLastActiveMonth);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveDay:%d", (unsigned int)keypadEntry.dateLastActiveDay);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveHour:%d", (unsigned int)keypadEntry.dateLastActiveHour);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveMin:%d", (unsigned int)keypadEntry.dateLastActiveMin);
    ESP_LOGD("NukiBle.NukiLock", "dateLastActiveSec:%d", (unsigned int)keypadEntry.dateLastActiveSec);
    ESP_LOGD("NukiBle.NukiLock", "lockCount:%d", (unsigned int)keypadEntry.lockCount);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)keypadEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)keypadEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)keypadEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)keypadEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)keypadEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)keypadEntry.allowedFromMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)keypadEntry.allowedFromSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)keypadEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)keypadEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)keypadEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)keypadEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)keypadEntry.allowedUntilMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)keypadEntry.allowedUntilSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)keypadEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)keypadEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)keypadEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)keypadEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)keypadEntry.allowedUntilTimeMin);
  }
}

void logUpdatedKeypadEntry(UpdatedKeypadEntry updatedKeypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "codeId:%d", (unsigned int)updatedKeypadEntry.codeId);
    ESP_LOGD("NukiBle.NukiLock", "code:%d", (unsigned int)updatedKeypadEntry.code);
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)updatedKeypadEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "enabled:%d", (unsigned int)updatedKeypadEntry.enabled);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)updatedKeypadEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)updatedKeypadEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)updatedKeypadEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)updatedKeypadEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)updatedKeypadEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)updatedKeypadEntry.allowedFromMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)updatedKeypadEntry.allowedFromSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)updatedKeypadEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)updatedKeypadEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)updatedKeypadEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)updatedKeypadEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)updatedKeypadEntry.allowedUntilMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)updatedKeypadEntry.allowedUntilSec);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)updatedKeypadEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)updatedKeypadEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)updatedKeypadEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)updatedKeypadEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)updatedKeypadEntry.allowedUntilTimeMin);
  }
}

void logAuthorizationEntry(AuthorizationEntry authorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "id:%d", (unsigned int)authorizationEntry.authId);
    ESP_LOGD("NukiBle.NukiLock", "idType:%d", (unsigned int)authorizationEntry.idType);
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)authorizationEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "enabled:%d", (unsigned int)authorizationEntry.enabled);
    ESP_LOGD("NukiBle.NukiLock", "remoteAllowed:%d", (unsigned int)authorizationEntry.remoteAllowed);
    ESP_LOGD("NukiBle.NukiLock", "createdYear:%d", (unsigned int)authorizationEntry.createdYear);
    ESP_LOGD("NukiBle.NukiLock", "createdMonth:%d", (unsigned int)authorizationEntry.createdMonth);
    ESP_LOGD("NukiBle.NukiLock", "createdDay:%d", (unsigned int)authorizationEntry.createdDay);
    ESP_LOGD("NukiBle.NukiLock", "createdHour:%d", (unsigned int)authorizationEntry.createdHour);
    ESP_LOGD("NukiBle.NukiLock", "createdMin:%d", (unsigned int)authorizationEntry.createdMinute);
    ESP_LOGD("NukiBle.NukiLock", "createdSec:%d", (unsigned int)authorizationEntry.createdSecond);
    ESP_LOGD("NukiBle.NukiLock", "lastactYear:%d", (unsigned int)authorizationEntry.lastActYear);
    ESP_LOGD("NukiBle.NukiLock", "lastactMonth:%d", (unsigned int)authorizationEntry.lastActMonth);
    ESP_LOGD("NukiBle.NukiLock", "lastactDay:%d", (unsigned int)authorizationEntry.lastActDay);
    ESP_LOGD("NukiBle.NukiLock", "lastactHour:%d", (unsigned int)authorizationEntry.lastActHour);
    ESP_LOGD("NukiBle.NukiLock", "lastactMin:%d", (unsigned int)authorizationEntry.lastActMinute);
    ESP_LOGD("NukiBle.NukiLock", "lastactSec:%d", (unsigned int)authorizationEntry.lastActSecond);
    ESP_LOGD("NukiBle.NukiLock", "lockCount:%d", (unsigned int)authorizationEntry.lockCount);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)authorizationEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)authorizationEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)authorizationEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)authorizationEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)authorizationEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)authorizationEntry.allowedFromMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)authorizationEntry.allowedFromSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)authorizationEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)authorizationEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)authorizationEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)authorizationEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)authorizationEntry.allowedUntilMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)authorizationEntry.allowedUntilSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)authorizationEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)authorizationEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)authorizationEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)authorizationEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)authorizationEntry.allowedUntilTimeMin);
  }
}

void logNewAuthorizationEntry(NewAuthorizationEntry newAuthorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)newAuthorizationEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "idType:%d", (unsigned int)newAuthorizationEntry.idType);
    ESP_LOGD("NukiBle.NukiLock", "remoteAllowed:%d", (unsigned int)newAuthorizationEntry.remoteAllowed);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)newAuthorizationEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)newAuthorizationEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)newAuthorizationEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)newAuthorizationEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)newAuthorizationEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)newAuthorizationEntry.allowedFromMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)newAuthorizationEntry.allowedFromSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)newAuthorizationEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)newAuthorizationEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)newAuthorizationEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)newAuthorizationEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)newAuthorizationEntry.allowedUntilMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)newAuthorizationEntry.allowedUntilSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)newAuthorizationEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)newAuthorizationEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)newAuthorizationEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)newAuthorizationEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)newAuthorizationEntry.allowedUntilTimeMin);
  }
}

void logUpdatedAuthorizationEntry(UpdatedAuthorizationEntry updatedAuthorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "id:%d", (unsigned int)updatedAuthorizationEntry.authId);
    ESP_LOGD("NukiBle.NukiLock", "name:%s", (const char*)updatedAuthorizationEntry.name);
    ESP_LOGD("NukiBle.NukiLock", "enabled:%d", (unsigned int)updatedAuthorizationEntry.enabled);
    ESP_LOGD("NukiBle.NukiLock", "remoteAllowed:%d", (unsigned int)updatedAuthorizationEntry.remoteAllowed);
    ESP_LOGD("NukiBle.NukiLock", "timeLimited:%d", (unsigned int)updatedAuthorizationEntry.timeLimited);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromYear:%d", (unsigned int)updatedAuthorizationEntry.allowedFromYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMonth:%d", (unsigned int)updatedAuthorizationEntry.allowedFromMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromDay:%d", (unsigned int)updatedAuthorizationEntry.allowedFromDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromHour:%d", (unsigned int)updatedAuthorizationEntry.allowedFromHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromMin:%d", (unsigned int)updatedAuthorizationEntry.allowedFromMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromSec:%d", (unsigned int)updatedAuthorizationEntry.allowedFromSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilYear:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilYear);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMonth:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilMonth);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilDay:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilDay);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilHour:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilMin:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilMinute);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilSec:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilSecond);
    ESP_LOGD("NukiBle.NukiLock", "allowedWeekdays:%d", (unsigned int)updatedAuthorizationEntry.allowedWeekdays);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeHour:%d", (unsigned int)updatedAuthorizationEntry.allowedFromTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedFromTimeMin:%d", (unsigned int)updatedAuthorizationEntry.allowedFromTimeMin);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeHour:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "allowedUntilTimeMin:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilTimeMin);
  }
}

void logNewTimeControlEntry(NewTimeControlEntry newTimeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "weekdays:%d", (unsigned int)newTimeControlEntry.weekdays);
    ESP_LOGD("NukiBle.NukiLock", "time:%d:%d", (unsigned int)newTimeControlEntry.timeHour, newTimeControlEntry.timeMin);
    ESP_LOGD("NukiBle.NukiLock", "lockAction:%d", (unsigned int)newTimeControlEntry.lockAction);
  }
}

void logTimeControlEntry(TimeControlEntry timeControlEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "entryId:%d", (unsigned int)timeControlEntry.entryId);
    ESP_LOGD("NukiBle.NukiLock", "enabled:%d", (unsigned int)timeControlEntry.enabled);
    ESP_LOGD("NukiBle.NukiLock", "weekdays:%d", (unsigned int)timeControlEntry.weekdays);
    ESP_LOGD("NukiBle.NukiLock", "time:%d:%d", (unsigned int)timeControlEntry.timeHour, timeControlEntry.timeMin);
    ESP_LOGD("NukiBle.NukiLock", "lockAction:%d", (unsigned int)timeControlEntry.lockAction);
  }
}

void logCompletionStatus(CompletionStatus completionStatus, bool debug) {
  if (debug) {
    switch (completionStatus) {
      case CompletionStatus::Busy :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: busy");
        break;
      case CompletionStatus::Canceled :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: canceled");
        break;
      case CompletionStatus::ClutchFailure :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: clutchFailure");
        break;
      case CompletionStatus::IncompleteFailure :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: incompleteFailure");
        break;
      case CompletionStatus::LowMotorVoltage :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: lowMotorVoltage");
        break;
      case CompletionStatus::MotorBlocked :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: motorBlocked");
        break;
      case CompletionStatus::MotorPowerFailure :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: motorPowerFailure");
        break;
      case CompletionStatus::OtherError :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: otherError");
        break;
      case CompletionStatus::Success :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: success");
        break;
      case CompletionStatus::TooRecent :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: tooRecent");
        break;
      case CompletionStatus::InvalidCode :
        ESP_LOGD("NukiBle.NukiLock", "Completion status: invalid code");
        break;
      default:
        ESP_LOGW("NukiBle.NukiLock", "Completion status: unknown");
        break;
    }
  }
}

void logNukiTrigger(Trigger nukiTrigger, bool debug) {
  if (debug) {
    switch (nukiTrigger) {
      case Trigger::AutoLock :
        ESP_LOGD("NukiBle.NukiLock", "Trigger: autoLock");
        break;
      case Trigger::Automatic :
        ESP_LOGD("NukiBle.NukiLock", "Trigger: automatic");
        break;
      case Trigger::Button :
        ESP_LOGD("NukiBle.NukiLock", "Trigger: button");
        break;
      case Trigger::Manual :
        ESP_LOGD("NukiBle.NukiLock", "Trigger: manual");
        break;
      case Trigger::System :
        ESP_LOGD("NukiBle.NukiLock", "Trigger: system");
        break;
      default:
        ESP_LOGW("NukiBle.NukiLock", "Trigger: unknown");
        break;
    }
  }
}

void logLockAction(LockAction lockAction, bool debug) {
  if (debug) {
    switch (lockAction) {
      case LockAction::FobAction1 :
        ESP_LOGD("NukiBle.NukiLock", "action: autoLock");
        break;
      case LockAction::FobAction2 :
        ESP_LOGD("NukiBle.NukiLock", "action: automatic");
        break;
      case LockAction::FobAction3 :
        ESP_LOGD("NukiBle.NukiLock", "action: button");
        break;
      case LockAction::FullLock :
        ESP_LOGD("NukiBle.NukiLock", "action: manual");
        break;
      case LockAction::Lock :
        ESP_LOGD("NukiBle.NukiLock", "action: system");
        break;
      case LockAction::LockNgo :
        ESP_LOGD("NukiBle.NukiLock", "action: system");
        break;
      case LockAction::LockNgoUnlatch :
        ESP_LOGD("NukiBle.NukiLock", "action: system");
        break;
      case LockAction::Unlatch :
        ESP_LOGD("NukiBle.NukiLock", "action: system");
        break;
      case LockAction::Unlock :
        ESP_LOGD("NukiBle.NukiLock", "action: system");
        break;
      default:
        ESP_LOGW("NukiBle.NukiLock", "action: unknown");
        break;
    }
  }
}

void logKeyturnerState(KeyTurnerState keyTurnerState, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "nukiState: %02x", (unsigned int)keyTurnerState.nukiState);
    ESP_LOGD("NukiBle.NukiLock", "lockState: %d", (unsigned int)keyTurnerState.lockState);
    logNukiTrigger(keyTurnerState.trigger, debug);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeYear: %d", (unsigned int)keyTurnerState.currentTimeYear);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeMonth: %d", (unsigned int)keyTurnerState.currentTimeMonth);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeDay: %d", (unsigned int)keyTurnerState.currentTimeDay);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeHour: %d", (unsigned int)keyTurnerState.currentTimeHour);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeMinute: %d", (unsigned int)keyTurnerState.currentTimeMinute);
    ESP_LOGD("NukiBle.NukiLock", "currentTimeSecond: %d", (unsigned int)keyTurnerState.currentTimeSecond);
    ESP_LOGD("NukiBle.NukiLock", "timeZoneOffset: %d", (unsigned int)keyTurnerState.timeZoneOffset);
    ESP_LOGD("NukiBle.NukiLock", "criticalBatteryState composed value: %d", (unsigned int)keyTurnerState.criticalBatteryState);
    ESP_LOGD("NukiBle.NukiLock", "criticalBatteryState: %d", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState) == 1 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "batteryCharging: %d", (unsigned int)(((unsigned int)keyTurnerState.criticalBatteryState & 2) == 2 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "batteryPercent: %d", (unsigned int)((keyTurnerState.criticalBatteryState & 0b11111100) >> 1));
    ESP_LOGD("NukiBle.NukiLock", "configUpdateCount: %d", (unsigned int)keyTurnerState.configUpdateCount);
    ESP_LOGD("NukiBle.NukiLock", "lockNgoTimer: %d", (unsigned int)keyTurnerState.lockNgoTimer);
    logLockAction((LockAction)keyTurnerState.lastLockAction, debug);
    ESP_LOGD("NukiBle.NukiLock", "lastLockActionTrigger: %d", (unsigned int)keyTurnerState.lastLockActionTrigger);
    logCompletionStatus(keyTurnerState.lastLockActionCompletionStatus, debug);
    ESP_LOGD("NukiBle.NukiLock", "doorSensorState: %d", (unsigned int)keyTurnerState.doorSensorState);
    ESP_LOGD("NukiBle.NukiLock", "nightModeActive: %d", (unsigned int)keyTurnerState.nightModeActive);
    ESP_LOGD("NukiBle.NukiLock", "accessoryBatteryState composed value: %d", (unsigned int)keyTurnerState.accessoryBatteryState);
    ESP_LOGD("NukiBle.NukiLock", "Keypad bat critical feature supported: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 1) == 1 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "Keypad Battery Critical: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 3) == 3 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "Doorsensor bat critical feature supported: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 4) == 4 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "Doorsensor Battery Critical: %d", (unsigned int)(((unsigned int)keyTurnerState.accessoryBatteryState & 12) == 12 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "remoteAccessStatus composed value: %d", (unsigned int)keyTurnerState.remoteAccessStatus);
    ESP_LOGD("NukiBle.NukiLock", "remoteAccessEnabled: %d", (unsigned int)(((keyTurnerState.remoteAccessStatus & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "bridgePaired: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 1) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "sseConnectedViaWifi: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 2) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "sseConnectionEstablished: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 3) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "isSseConnectedViaThread: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 4) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "threadSseUplinkEnabledByUser: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 5) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "nat64AvailableViaThread: %d", (unsigned int)((((keyTurnerState.remoteAccessStatus >> 6) & 1) == 1) ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "bleConnectionStrength: %d", (unsigned int)keyTurnerState.bleConnectionStrength);
    ESP_LOGD("NukiBle.NukiLock", "wifiConnectionStrength: %d", (unsigned int)keyTurnerState.wifiConnectionStrength);
    ESP_LOGD("NukiBle.NukiLock", "wifiConnectionStatus composed value: %d", (unsigned int)keyTurnerState.wifiConnectionStatus);
    ESP_LOGD("NukiBle.NukiLock", "wifiStatus: %d", (unsigned int)(keyTurnerState.wifiConnectionStatus & 3));
    ESP_LOGD("NukiBle.NukiLock", "sseStatus: %d", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 2) & 3));
    ESP_LOGD("NukiBle.NukiLock", "wifiQuality: %d", (unsigned int)((keyTurnerState.wifiConnectionStatus >> 4) & 15));
    ESP_LOGD("NukiBle.NukiLock", "mqttConnectionStatus composed value: %d", (unsigned int)keyTurnerState.mqttConnectionStatus);
    ESP_LOGD("NukiBle.NukiLock", "mqttStatus: %d", (unsigned int)(keyTurnerState.mqttConnectionStatus & 3));
    ESP_LOGD("NukiBle.NukiLock", "mqttConnectionChannel: %d", (unsigned int)((keyTurnerState.mqttConnectionStatus >> 2) & 1));
    ESP_LOGD("NukiBle.NukiLock", "threadConnectionStatus composed value: %d", (unsigned int)keyTurnerState.threadConnectionStatus);
    ESP_LOGD("NukiBle.NukiLock", "threadConnectionStatus: %d", (unsigned int)(keyTurnerState.threadConnectionStatus & 3));
    ESP_LOGD("NukiBle.NukiLock", "threadSseStatus: %d", (unsigned int)((keyTurnerState.threadConnectionStatus >> 2) & 3));
    ESP_LOGD("NukiBle.NukiLock", "isCommissioningModeActive: %d", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 16) != 0 ? 1 : 0));
    ESP_LOGD("NukiBle.NukiLock", "isWifiDisabledBecauseOfThread: %d", (unsigned int)(((unsigned int)keyTurnerState.threadConnectionStatus & 32) != 0 ? 1 : 0));
  }
}

void logBatteryReport(BatteryReport batteryReport, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "batteryDrain:%d", (unsigned int)batteryReport.batteryDrain);
    ESP_LOGD("NukiBle.NukiLock", "batteryVoltage:%d", (unsigned int)batteryReport.batteryVoltage);
    ESP_LOGD("NukiBle.NukiLock", "criticalBatteryState:%d", (unsigned int)batteryReport.criticalBatteryState);
    ESP_LOGD("NukiBle.NukiLock", "lockAction:%d", (unsigned int)batteryReport.lockAction);
    ESP_LOGD("NukiBle.NukiLock", "startVoltage:%d", (unsigned int)batteryReport.startVoltage);
    ESP_LOGD("NukiBle.NukiLock", "lowestVoltage:%d", (unsigned int)batteryReport.lowestVoltage);
    ESP_LOGD("NukiBle.NukiLock", "lockDistance:%d", (unsigned int)batteryReport.lockDistance);
    ESP_LOGD("NukiBle.NukiLock", "startTemperature:%d", (unsigned int)batteryReport.startTemperature);
    ESP_LOGD("NukiBle.NukiLock", "maxTurnCurrent:%d", (unsigned int)batteryReport.maxTurnCurrent);
    ESP_LOGD("NukiBle.NukiLock", "batteryResistance:%d", (unsigned int)batteryReport.batteryResistance);
  }
}

void logLogEntry(LogEntry logEntry, bool debug) {
  ESP_LOGD("NukiBle.NukiLock", "[%lu] type: %u authId: %lu name: %s %d-%d-%d %d:%d:%d ", logEntry.index, (uint8_t)logEntry.loggingType, logEntry.authId, logEntry.name, logEntry.timeStampYear, logEntry.timeStampMonth, logEntry.timeStampDay, logEntry.timeStampHour, logEntry.timeStampMinute, logEntry.timeStampSecond);


  switch (logEntry.loggingType) {
    case LoggingType::LoggingEnabled: {
      ESP_LOGD("NukiBle.NukiLock", "Logging enabled: %d", (unsigned int)logEntry.data[0]);
      break;
    }
    case LoggingType::LockAction:
    case LoggingType::Calibration:
    case LoggingType::InitializationRun: {
      logLockAction((LockAction)logEntry.data[0], debug);
      logNukiTrigger((Trigger)logEntry.data[1], debug);
      ESP_LOGD("NukiBle.NukiLock", "Flags: %d", (unsigned int)logEntry.data[2]);
      logCompletionStatus((CompletionStatus)logEntry.data[3], debug);
      break;
    }
    case LoggingType::KeypadAction: {
      logLockAction((LockAction)logEntry.data[0], debug);
      ESP_LOGD("NukiBle.NukiLock", "Source: %d", (unsigned int)logEntry.data[1]);
      logCompletionStatus((CompletionStatus)logEntry.data[2], debug);
      uint16_t codeId = 0;
      memcpy(&codeId, &logEntry.data[3], 2);
      ESP_LOGD("NukiBle.NukiLock", "Code id: %d", (unsigned int)codeId);
      break;
    }
    case LoggingType::DoorSensor: {
      if (logEntry.data[0] == 0x00) {
        ESP_LOGD("NukiBle.NukiLock", "Door opened") ;
      }
      if (logEntry.data[0] == 0x01) {
        ESP_LOGD("NukiBle.NukiLock", "Door closed") ;
      }
      if (logEntry.data[0] == 0x02) {
        ESP_LOGD("NukiBle.NukiLock", "Door sensor jammed") ;
      }
      break;
    }
    case LoggingType::DoorSensorLoggingEnabled: {
      ESP_LOGD("NukiBle.NukiLock", "Logging enabled: %d", (unsigned int)logEntry.data[0]);
      break;
    }
    default:
      ESP_LOGW("NukiBle.NukiLock", "Unknown logging type");
      break;
  }
}

void logAdvancedConfig(AdvancedConfig advancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "totalDegrees: %d", (unsigned int)advancedConfig.totalDegrees);
    ESP_LOGD("NukiBle.NukiLock", "unlockedPositionOffsetDegrees: %d", (unsigned int)advancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "lockedPositionOffsetDegrees: %f", (const float)advancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "singleLockedPositionOffsetDegrees: %f", (const float)advancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "unlockedToLockedTransitionOffsetDegrees: %d", (unsigned int)advancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "lockNgoTimeout: %d", (unsigned int)advancedConfig.lockNgoTimeout);
    ESP_LOGD("NukiBle.NukiLock", "singleButtonPressAction: %d", (unsigned int)advancedConfig.singleButtonPressAction);
    ESP_LOGD("NukiBle.NukiLock", "doubleButtonPressAction: %d", (unsigned int)advancedConfig.doubleButtonPressAction);
    ESP_LOGD("NukiBle.NukiLock", "detachedCylinder: %d", (unsigned int)advancedConfig.detachedCylinder);
    ESP_LOGD("NukiBle.NukiLock", "batteryType: %d", (unsigned int)advancedConfig.batteryType);
    ESP_LOGD("NukiBle.NukiLock", "automaticBatteryTypeDetection: %d", (unsigned int)advancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD("NukiBle.NukiLock", "unlatchDuration: %d", (unsigned int)advancedConfig.unlatchDuration);
    ESP_LOGD("NukiBle.NukiLock", "autoLockTimeOut: %d", (unsigned int)advancedConfig.autoLockTimeOut);
    ESP_LOGD("NukiBle.NukiLock", "autoUnLockDisabled: %d", (unsigned int)advancedConfig.autoUnLockDisabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEnabled: %d", (unsigned int)advancedConfig.nightModeEnabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeStartTime Hour: %d", (unsigned int)advancedConfig.nightModeStartTime[0]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeStartTime Minute: %d", (unsigned int)advancedConfig.nightModeStartTime[1]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEndTime Hour: %d", (unsigned int)advancedConfig.nightModeEndTime[0]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEndTime Minute: %d", (unsigned int)advancedConfig.nightModeEndTime[1]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeAutoLockEnabled: %d", (unsigned int)advancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeAutoUnlockDisabled: %d", (unsigned int)advancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeImmediateLockOnStart: %d", (unsigned int)advancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD("NukiBle.NukiLock", "autoLockEnabled: %d", (unsigned int)advancedConfig.autoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "immediateAutoLockEnabled: %d", (unsigned int)advancedConfig.immediateAutoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "autoUpdateEnabled: %d", (unsigned int)advancedConfig.autoUpdateEnabled);
    ESP_LOGD("NukiBle.NukiLock", "motorSpeed: %d", (unsigned int)advancedConfig.motorSpeed);
    ESP_LOGD("NukiBle.NukiLock", "enableSlowSpeedDuringNightMode: %d", (unsigned int)advancedConfig.enableSlowSpeedDuringNightMode);
  }
}

void logNewAdvancedConfig(NewAdvancedConfig newAdvancedConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "unlockedPositionOffsetDegrees: %d", (unsigned int)newAdvancedConfig.unlockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "lockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.lockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "singleLockedPositionOffsetDegrees: %f", (const float)newAdvancedConfig.singleLockedPositionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "unlockedToLockedTransitionOffsetDegrees: %d", (unsigned int)newAdvancedConfig.unlockedToLockedTransitionOffsetDegrees);
    ESP_LOGD("NukiBle.NukiLock", "lockNgoTimeout: %d", (unsigned int)newAdvancedConfig.lockNgoTimeout);
    ESP_LOGD("NukiBle.NukiLock", "singleButtonPressAction: %d", (unsigned int)newAdvancedConfig.singleButtonPressAction);
    ESP_LOGD("NukiBle.NukiLock", "doubleButtonPressAction: %d", (unsigned int)newAdvancedConfig.doubleButtonPressAction);
    ESP_LOGD("NukiBle.NukiLock", "detachedCylinder: %d", (unsigned int)newAdvancedConfig.detachedCylinder);
    ESP_LOGD("NukiBle.NukiLock", "batteryType: %d", (unsigned int)newAdvancedConfig.batteryType);
    ESP_LOGD("NukiBle.NukiLock", "automaticBatteryTypeDetection: %d", (unsigned int)newAdvancedConfig.automaticBatteryTypeDetection);
    ESP_LOGD("NukiBle.NukiLock", "unlatchDuration: %d", (unsigned int)newAdvancedConfig.unlatchDuration);
    ESP_LOGD("NukiBle.NukiLock", "autoUnLockTimeOut: %d", (unsigned int)newAdvancedConfig.autoLockTimeOut);
    ESP_LOGD("NukiBle.NukiLock", "autoUnLockDisabled: %d", (unsigned int)newAdvancedConfig.autoUnLockDisabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEnabled: %d", (unsigned int)newAdvancedConfig.nightModeEnabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeStartTime Hour: %d", (unsigned int)newAdvancedConfig.nightModeStartTime[0]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeStartTime Minute: %d", (unsigned int)newAdvancedConfig.nightModeStartTime[1]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEndTime Hour: %d", (unsigned int)newAdvancedConfig.nightModeEndTime[0]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeEndTime Minute: %d", (unsigned int)newAdvancedConfig.nightModeEndTime[1]);
    ESP_LOGD("NukiBle.NukiLock", "nightModeAutoLockEnabled: %d", (unsigned int)newAdvancedConfig.nightModeAutoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeAutoUnlockDisabled: %d", (unsigned int)newAdvancedConfig.nightModeAutoUnlockDisabled);
    ESP_LOGD("NukiBle.NukiLock", "nightModeImmediateLockOnStart: %d", (unsigned int)newAdvancedConfig.nightModeImmediateLockOnStart);
    ESP_LOGD("NukiBle.NukiLock", "autoLockEnabled: %d", (unsigned int)newAdvancedConfig.autoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "immediateAutoLockEnabled: %d", (unsigned int)newAdvancedConfig.immediateAutoLockEnabled);
    ESP_LOGD("NukiBle.NukiLock", "autoUpdateEnabled: %d", (unsigned int)newAdvancedConfig.autoUpdateEnabled);
  }
}

void logWifiScanEntry(WifiScanEntry wifiScanEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "ssid: %s", (const char*)wifiScanEntry.ssid);
    ESP_LOGD("NukiBle.NukiLock", "type: %d", (unsigned int)wifiScanEntry.type);
    ESP_LOGD("NukiBle.NukiLock", "signal raw: %d", (unsigned int)wifiScanEntry.signal);
    ESP_LOGD("NukiBle.NukiLock", "signal: %d", (unsigned int)(wifiScanEntry.signal & 255));
  }
}

void logMqttConfig(MqttConfig mqttConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "enabled: %d", (unsigned int)mqttConfig.enabled);
    ESP_LOGD("NukiBle.NukiLock", "hostName: %s", (const char*)mqttConfig.hostName);
    ESP_LOGD("NukiBle.NukiLock", "userName: %s", (const char*)mqttConfig.userName);
    ESP_LOGD("NukiBle.NukiLock", "secureConnection: %d", (unsigned int)mqttConfig.secureConnection);
    ESP_LOGD("NukiBle.NukiLock", "autoDiscovery: %d", (unsigned int)mqttConfig.autoDiscovery);
    ESP_LOGD("NukiBle.NukiLock", "lockingEnabled: %d", (unsigned int)mqttConfig.lockingEnabled);
  }
}

void logMqttConfigForMigration(MqttConfigForMigration mqttConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "enabled: %d", (unsigned int)mqttConfigForMigration.enabled);
    ESP_LOGD("NukiBle.NukiLock", "hostName: %s", (const char*)mqttConfigForMigration.hostName);
    ESP_LOGD("NukiBle.NukiLock", "userName: %s", (const char*)mqttConfigForMigration.userName);
    ESP_LOGD("NukiBle.NukiLock", "secureConnection: %d", (unsigned int)mqttConfigForMigration.secureConnection);
    ESP_LOGD("NukiBle.NukiLock", "autoDiscovery: %d", (unsigned int)mqttConfigForMigration.autoDiscovery);
    ESP_LOGD("NukiBle.NukiLock", "lockingEnabled: %d", (unsigned int)mqttConfigForMigration.lockingEnabled);
    ESP_LOGD("NukiBle.NukiLock", "passphrase: %s", (const char*)mqttConfigForMigration.passphrase);
  }
}

void logWifiConfig(WifiConfig wifiConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "serverBridgeId: %d", (unsigned int)wifiConfig.serverBridgeId);
    ESP_LOGD("NukiBle.NukiLock", "wifiEnabled: %d", (unsigned int)wifiConfig.wifiEnabled);
    ESP_LOGD("NukiBle.NukiLock", "wifiExpertSettings composed value: %d", (unsigned int)wifiConfig.wifiExpertSettings);
    ESP_LOGD("NukiBle.NukiLock", "expertSettingsMode: %d", (unsigned int)(wifiConfig.wifiExpertSettings & 3));
    ESP_LOGD("NukiBle.NukiLock", "broadcastFilterSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 2) & 3));
    ESP_LOGD("NukiBle.NukiLock", "dtimSkipSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 4) & 7));
    ESP_LOGD("NukiBle.NukiLock", "sseSkipSettings: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 7) & 7));
    ESP_LOGD("NukiBle.NukiLock", "powersafeMode: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 10) & 3));
    ESP_LOGD("NukiBle.NukiLock", "activePingEnabled: %d", (unsigned int)((wifiConfig.wifiExpertSettings >> 12) & 1));
  }
}

void logWifiConfigForMigration(WifiConfigForMigration wifiConfigForMigration, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "ssid: %s", (const char*)wifiConfigForMigration.ssid);
    ESP_LOGD("NukiBle.NukiLock", "type: %d", (unsigned int)wifiConfigForMigration.type);
    ESP_LOGD("NukiBle.NukiLock", "passphrase: %s", (const char*)wifiConfigForMigration.passphrase);
  }
}

void logKeypad2Config(Keypad2Config keypad2Config, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "updatePending: %d", (unsigned int)keypad2Config.updatePending);
    ESP_LOGD("NukiBle.NukiLock", "ledBrightness: %d", (unsigned int)keypad2Config.ledBrightness);
    ESP_LOGD("NukiBle.NukiLock", "batteryType: %d", (unsigned int)keypad2Config.batteryType);
    ESP_LOGD("NukiBle.NukiLock", "buttonMode: %d", (unsigned int)keypad2Config.buttonMode);
    ESP_LOGD("NukiBle.NukiLock", "lockAction: %d", (unsigned int)keypad2Config.lockAction);
  }
}

void logDoorSensorConfig(DoorSensorConfig doorSensorConfig, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "enabled: %d", (unsigned int)doorSensorConfig.enabled);
    ESP_LOGD("NukiBle.NukiLock", "doorAjarTimeout: %d", (unsigned int)doorSensorConfig.doorAjarTimeout);
    ESP_LOGD("NukiBle.NukiLock", "doorAjarLoggingEnabled: %d", (unsigned int)doorSensorConfig.doorAjarLoggingEnabled);
    ESP_LOGD("NukiBle.NukiLock", "doorStatusMismatchLoggingEnabled: %d", (unsigned int)doorSensorConfig.doorStatusMismatchLoggingEnabled);
  }
}

void logFingerprintEntry(FingerprintEntry fingerprintEntry, bool debug) {
  if (debug) {
    char hexString[65]; // 32 bytes * 2 chars + 1 null terminator
    for (size_t i = 0; i < 32; i++) {
        sprintf(&hexString[i*2], "%02x", fingerprintEntry.fingerprintId[i]);
    }
    hexString[64] = '\0';

    ESP_LOGD("NukiBle.NukiLock", "fingerprintId: %s", hexString);
    ESP_LOGD("NukiBle.NukiLock", "keypadCodeId: %d", (unsigned int)fingerprintEntry.keypadCodeId);
    ESP_LOGD("NukiBle.NukiLock", "name: %s", fingerprintEntry.name);
  }
}

void logAccessoryInfo(AccessoryInfo accessoryInfo, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "dateYear: %d", (unsigned int)accessoryInfo.dateYear);
    ESP_LOGD("NukiBle.NukiLock", "dateMonth: %d", (unsigned int)accessoryInfo.dateMonth);
    ESP_LOGD("NukiBle.NukiLock", "dateDay: %d", (unsigned int)accessoryInfo.dateDay);
    ESP_LOGD("NukiBle.NukiLock", "dateHour: %d", (unsigned int)accessoryInfo.dateHour);
    ESP_LOGD("NukiBle.NukiLock", "dateMinute: %d", (unsigned int)accessoryInfo.dateMinute);
    ESP_LOGD("NukiBle.NukiLock", "dateSecond: %d", (unsigned int)accessoryInfo.dateSecond);
    ESP_LOGD("NukiBle.NukiLock", "accessoryNukiId: %d", (unsigned int)accessoryInfo.accessoryNukiId);
    ESP_LOGD("NukiBle.NukiLock", "accessoryType: %d", (unsigned int)accessoryInfo.accessoryType);
    ESP_LOGD("NukiBle.NukiLock", "firmwareVersion: %d.%d.%d", accessoryInfo.firmwareVersion[0], accessoryInfo.firmwareVersion[1], accessoryInfo.firmwareVersion[2]);
    ESP_LOGD("NukiBle.NukiLock", "hardwareRevision: %d.%d", accessoryInfo.hardwareRevision[0], accessoryInfo.hardwareRevision[1]);
    ESP_LOGD("NukiBle.NukiLock", "productVariantDifferentiator: %d", (unsigned int)accessoryInfo.productVariantDifferentiator);
    ESP_LOGD("NukiBle.NukiLock", "mostRecentBatteryVoltage: %d", (unsigned int)accessoryInfo.mostRecentBatteryVoltage);
    ESP_LOGD("NukiBle.NukiLock", "mostRecentTemperature: %d", (unsigned int)accessoryInfo.mostRecentTemperature);
  }
}

void logDailyStatistics(DailyStatistics dailyStatistics, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "dateYear: %d", (unsigned int)dailyStatistics.dateYear);
    ESP_LOGD("NukiBle.NukiLock", "dateMonth: %d", (unsigned int)dailyStatistics.dateMonth);
    ESP_LOGD("NukiBle.NukiLock", "dateDay: %d", (unsigned int)dailyStatistics.dateDay);
    //ESP_LOGD("NukiBle.NukiLock", "dateHour: %d", (unsigned int)dailyStatistics.dateHour);
    //ESP_LOGD("NukiBle.NukiLock", "dateMinute: %d", (unsigned int)dailyStatistics.dateMinute);
    //ESP_LOGD("NukiBle.NukiLock", "dateSecond: %d", (unsigned int)dailyStatistics.dateSecond);
    ESP_LOGD("NukiBle.NukiLock", "version: %d", (unsigned int)dailyStatistics.version);
    ESP_LOGD("NukiBle.NukiLock", "countSuccessfulLockActions: %d", (unsigned int)dailyStatistics.countSuccessfulLockActions);
    ESP_LOGD("NukiBle.NukiLock", "countErroneousLockActions: %d", (unsigned int)dailyStatistics.countErroneousLockActions);
    ESP_LOGD("NukiBle.NukiLock", "avgCurrentConsumptionLock: %d", (unsigned int)dailyStatistics.avgCurrentConsumptionLock);
    ESP_LOGD("NukiBle.NukiLock", "maxCurrentConsumptionLock: %d", (unsigned int)dailyStatistics.maxCurrentConsumptionLock);
    ESP_LOGD("NukiBle.NukiLock", "batteryMinStartVoltageLock: %d", (unsigned int)dailyStatistics.batteryMinStartVoltageLock);
    ESP_LOGD("NukiBle.NukiLock", "countSuccessfulUnlatchActions: %d", (unsigned int)dailyStatistics.countSuccessfulUnlatchActions);
    ESP_LOGD("NukiBle.NukiLock", "countErroneousUnlatchActions: %d", (unsigned int)dailyStatistics.countErroneousUnlatchActions);
    ESP_LOGD("NukiBle.NukiLock", "avgCurrentConsumptionUnlatch: %d", (unsigned int)dailyStatistics.avgCurrentConsumptionUnlatch);
    ESP_LOGD("NukiBle.NukiLock", "maxCurrentConsumptionUnlatch: %d", (unsigned int)dailyStatistics.maxCurrentConsumptionUnlatch);
    ESP_LOGD("NukiBle.NukiLock", "batteryMinStartVoltageUnlatch: %d", (unsigned int)dailyStatistics.batteryMinStartVoltageUnlatch);
    ESP_LOGD("NukiBle.NukiLock", "incomingCommands: %d", (unsigned int)dailyStatistics.incomingCommands);
    ESP_LOGD("NukiBle.NukiLock", "outgoingCommands: %d", (unsigned int)dailyStatistics.outgoingCommands);
    ESP_LOGD("NukiBle.NukiLock", "maxTemperature: %d", (unsigned int)dailyStatistics.maxTemperature);
    ESP_LOGD("NukiBle.NukiLock", "minTemperature: %d", (unsigned int)dailyStatistics.minTemperature);
    ESP_LOGD("NukiBle.NukiLock", "avgTemperature: %d", (unsigned int)dailyStatistics.avgTemperature);
    ESP_LOGD("NukiBle.NukiLock", "numDoorSensorStatusChanges: %d", (unsigned int)dailyStatistics.numDoorSensorStatusChanges);
    ESP_LOGD("NukiBle.NukiLock", "maxBatteryPercentage: %d", (unsigned int)dailyStatistics.maxBatteryPercentage);
    ESP_LOGD("NukiBle.NukiLock", "minBatteryPercentage: %d", (unsigned int)dailyStatistics.minBatteryPercentage);
    ESP_LOGD("NukiBle.NukiLock", "idleTime: %d", (unsigned int)dailyStatistics.idleTime);
    ESP_LOGD("NukiBle.NukiLock", "connectionTime: %d", (unsigned int)dailyStatistics.connectionTime);
    ESP_LOGD("NukiBle.NukiLock", "actionTime: %d", (unsigned int)dailyStatistics.actionTime);
  }
}

void logGeneralStatistics(GeneralStatistics generalStatistics, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "version: %d", (unsigned int)generalStatistics.version);
    ESP_LOGD("NukiBle.NukiLock", "firstCalibrationYear: %d", (unsigned int)generalStatistics.firstCalibrationYear);
    ESP_LOGD("NukiBle.NukiLock", "firstCalibrationMonth: %d", (unsigned int)generalStatistics.firstCalibrationMonth);
    ESP_LOGD("NukiBle.NukiLock", "firstCalibrationDay: %d", (unsigned int)generalStatistics.firstCalibrationDay);
    ESP_LOGD("NukiBle.NukiLock", "calibrationCount: %d", (unsigned int)generalStatistics.calibrationCount);
    ESP_LOGD("NukiBle.NukiLock", "lockActionCount: %d", (unsigned int)generalStatistics.lockActionCount);
    ESP_LOGD("NukiBle.NukiLock", "unlatchCount: %d", (unsigned int)generalStatistics.unlatchCount);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateYear: %d", (unsigned int)generalStatistics.lastRebootDateYear);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateMonth: %d", (unsigned int)generalStatistics.lastRebootDateMonth);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateDay: %d", (unsigned int)generalStatistics.lastRebootDateDay);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateHour: %d", (unsigned int)generalStatistics.lastRebootDateHour);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateMinute: %d", (unsigned int)generalStatistics.lastRebootDateMinute);
    ESP_LOGD("NukiBle.NukiLock", "lastRebootDateSecond: %d", (unsigned int)generalStatistics.lastRebootDateSecond);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateYear: %d", (unsigned int)generalStatistics.lastChargeDateYear);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateMonth: %d", (unsigned int)generalStatistics.lastChargeDateMonth);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateDay: %d", (unsigned int)generalStatistics.lastChargeDateDay);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateHour: %d", (unsigned int)generalStatistics.lastChargeDateHour);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateMinute: %d", (unsigned int)generalStatistics.lastChargeDateMinute);
    ESP_LOGD("NukiBle.NukiLock", "lastChargeDateSecond: %d", (unsigned int)generalStatistics.lastChargeDateSecond);
    ESP_LOGD("NukiBle.NukiLock", "initialBatteryVoltage: %d", (unsigned int)generalStatistics.initialBatteryVoltage);
    ESP_LOGD("NukiBle.NukiLock", "numActionsDuringBatteryCycle: %d", (unsigned int)generalStatistics.numActionsDuringBatteryCycle);
    ESP_LOGD("NukiBle.NukiLock", "numUnexpectedReboots: %d", (unsigned int)generalStatistics.numUnexpectedReboots);
  }
}

void logInternalLogEntry(InternalLogEntry internalLogEntry, bool debug) {
  if (debug) {
    ESP_LOGD("NukiBle.NukiLock", "[%d] type: %d authId: %d %d-%d-%d %d:%d:%d",
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
    ESP_LOGD("NukiBle.NukiLock", "data: %d", (unsigned int)internalLogEntry.data);
  }
}

} // namespace Nuki