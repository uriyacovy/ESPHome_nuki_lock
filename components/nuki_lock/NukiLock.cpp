#include "NukiLockUtils.h"
#include "NukiLock.h"
#include "NukiUtils.h"

#include <list>
#include <cstdint>
#include <cstring>
#include <string>

#include "esp_log.h"


namespace NukiLock {
NukiLock::NukiLock(const std::string& deviceName, const uint32_t deviceId)
  : NukiBle(deviceName,
            deviceId,
            keyturnerPairingServiceUUID,
            keyturnerPairingServiceUltraUUID,
            keyturnerServiceUUID,
            keyturnerGdioUUID,
            keyturnerGdioUltraUUID,
            keyturnerUserDataUUID,
            deviceName) {
    errorCode = (uint8_t)ErrorCode::ERROR_UNKNOWN;
}

Nuki::CmdResult NukiLock::lockAction(const LockAction lockAction, const uint32_t nukiAppId, const uint8_t flags, const char* nameSuffix, const uint8_t nameSuffixLen) {
  Action action{};
  unsigned char payload[sizeof(LockAction) + 4 + 1 + 20] = {0};
  memcpy(payload, &lockAction, sizeof(LockAction));
  memcpy(&payload[sizeof(LockAction)], &nukiAppId, 4);
  memcpy(&payload[sizeof(LockAction) + 4], &flags, 1);
  uint8_t payloadLen = 0;
  if (nameSuffix) {
    //If nameSuffixLen is between 1 & 18, use it, else use 19 (keep 1 for ending '\0')
    uint8_t len = nameSuffixLen>0 && nameSuffixLen<19 ? nameSuffixLen : 19;
    strncpy((char*)&payload[sizeof(LockAction) + 4 + 1], nameSuffix, len);
    payload[sizeof(LockAction) + 4 + 1 + len] = '\0'; //In any case, add '\0' at end of string
    payloadLen = sizeof(LockAction) + 4 + 1 + 20;
  } else {
    payloadLen = sizeof(LockAction) + 4 + 1;
  }

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndAccept;
  action.command = Command::LockAction;
  memcpy(action.payload, &payload, payloadLen);
  action.payloadLen = payloadLen;

  return executeAction(action);
}

Nuki::CmdResult NukiLock::keypadAction(KeypadActionSource source, uint32_t code, KeypadAction keypadAction) {
  Action action{};
  unsigned char payload[6] = {(unsigned char)source};
  memcpy(&payload[1], &code, sizeof(code));
  memcpy(&payload[1+sizeof(code)], &keypadAction, sizeof(KeypadAction));
  uint8_t payloadLen = 6;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndAccept;
  action.command = Command::KeypadAction;
  memcpy(action.payload, &payload, payloadLen);
  action.payloadLen = payloadLen;

  return executeAction(action);
}

Nuki::CmdResult NukiLock::requestKeyTurnerState(KeyTurnerState* retrievedKeyTurnerState) {
  Action action{};
  uint16_t payload = (uint16_t)Command::KeyturnerStates;

  action.cmdType = Nuki::CommandType::Command;
  action.command = Command::RequestData;
  memcpy(&action.payload[0], &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    // printBuffer((uint8_t*)&retrievedKeyTurnerState, sizeof(retrievedKeyTurnerState), false, "retrieved Keyturner state", debugNukiHexData);
    memcpy(retrievedKeyTurnerState, &keyTurnerState, sizeof(KeyTurnerState));
  }
  return result;
}

void NukiLock::retrieveKeyTunerState(KeyTurnerState* retrievedKeyTurnerState) {
  memcpy(retrievedKeyTurnerState, &keyTurnerState, sizeof(KeyTurnerState));
}


Nuki::CmdResult NukiLock::requestBatteryReport(BatteryReport* retrievedBatteryReport) {
  Action action{};
  uint16_t payload = (uint16_t)Command::BatteryReport;

  action.cmdType = Nuki::CommandType::Command;
  action.command = Command::RequestData;
  memcpy(&action.payload[0], &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    memcpy(retrievedBatteryReport, &batteryReport, sizeof(batteryReport));
  }
  return result;
}


Nuki::CmdResult NukiLock::requestConfig(Config* retrievedConfig) {
  Action action{};
  action.cmdType = Nuki::CommandType::CommandWithChallenge;
  action.command = Command::RequestConfig;

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    memcpy(retrievedConfig, &config, sizeof(Config));
  }
  return result;
}

Nuki::CmdResult NukiLock::requestAdvancedConfig(AdvancedConfig* retrievedAdvancedConfig) {
  Action action{};
  action.cmdType = Nuki::CommandType::CommandWithChallenge;
  action.command = Command::RequestAdvancedConfig;

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    memcpy(retrievedAdvancedConfig, &advancedConfig, sizeof(AdvancedConfig));
  }
  return result;
}


//basic config change methods
Nuki::CmdResult NukiLock::setName(const std::string& name) {

  if (name.length() <= 32) {
    Config oldConfig;
    Nuki::CmdResult result = requestConfig(&oldConfig);
    if (result == Nuki::CmdResult::Success) {
      memset(oldConfig.name, 0, sizeof(oldConfig.name));
      memcpy(oldConfig.name, name.c_str(), name.length());
      result = setFromConfig(oldConfig);
    }
    return result;
  } else {
    ESP_LOGW("NukiBle.NukiLock", "setName, too long (max32)");
    return Nuki::CmdResult::Failed;
  }
}

Nuki::CmdResult NukiLock::setLatitude(const float degrees) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.latitude = degrees;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setLongitude(const float degrees) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.longitude = degrees;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableAutoUnlatch(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.autoUnlatch = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setFobAction(const uint8_t fobActionNr, const uint8_t fobAction) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    switch (fobActionNr) {
      case 1:
        oldConfig.fobAction1 = fobAction;
        break;
      case 2:
        oldConfig.fobAction2 = fobAction;
        break;
      case 3:
        oldConfig.fobAction3 = fobAction;
        break;
      default:
        return Nuki::CmdResult::Error;
    }
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::retrieveInternalLogEntries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount) {
  Action action;
  unsigned char payload[8] = {0};
  memcpy(payload, &startIndex, 4);
  memcpy(&payload[4], &count, 2);
  memcpy(&payload[6], &sortOrder, 1);
  memcpy(&payload[7], &totalCount, 1);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestInternalLogEntries;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  listOfInternalLogEntries.clear();

  return executeAction(action);
}

Nuki::CmdResult NukiLock::enableDst(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.dstMode = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setTimeZoneOffset(const int16_t minutes) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.timeZoneOffset = minutes;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setTimeZoneId(const TimeZoneId timeZoneId) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.timeZoneId = timeZoneId;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableButton(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.buttonEnabled = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}


//advanced config change methods
Nuki::CmdResult NukiLock::setUnlockedPositionOffsetDegrees(const int16_t degrees) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.unlockedPositionOffsetDegrees = degrees;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setLockedPositionOffsetDegrees(const int16_t degrees) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.lockedPositionOffsetDegrees = degrees;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setSingleLockedPositionOffsetDegrees(const int16_t degrees) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.singleLockedPositionOffsetDegrees = degrees;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setUnlockedToLockedTransitionOffsetDegrees(const int16_t degrees) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.unlockedToLockedTransitionOffsetDegrees = degrees;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setLockNgoTimeout(const uint8_t timeout) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.lockNgoTimeout = timeout;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableDetachedCylinder(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.detachedCylinder = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setUnlatchDuration(const uint8_t duration) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.unlatchDuration = duration;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setAutoLockTimeOut(const uint8_t timeout) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.autoLockTimeOut = timeout;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableNightMode(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeEnabled = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setNightModeStartTime(unsigned char starttime[2]) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeStartTime[0] = starttime[0];
    oldConfig.nightModeStartTime[1] = starttime[1];    
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setNightModeEndTime(unsigned char endtime[2]) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeEndTime[0] = endtime[0];
    oldConfig.nightModeEndTime[1] = endtime[1];    
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableNightModeAutoLock(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeAutoLockEnabled = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::disableNightModeAutoUnlock(const bool disable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeAutoUnlockDisabled = disable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableNightModeImmediateLockOnStart(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.nightModeImmediateLockOnStart = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setSingleButtonPressAction(const ButtonPressAction action) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.singleButtonPressAction = action;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setDoubleButtonPressAction(const ButtonPressAction action) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.doubleButtonPressAction = action;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setBatteryType(const BatteryType type) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.batteryType = type;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableAutoBatteryTypeDetection(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.automaticBatteryTypeDetection = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::disableAutoUnlock(const bool disable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.autoUnLockDisabled = disable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableAutoLock(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.autoLockEnabled = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableImmediateAutoLock(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.immediateAutoLockEnabled = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableAutoUpdate(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.autoUpdateEnabled = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setMotorSpeed(const MotorSpeed speed) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.motorSpeed = speed;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableSlowSpeedDuringNightMode(const bool enable) {
  AdvancedConfig oldConfig;
  Nuki::CmdResult result = requestAdvancedConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.enableSlowSpeedDuringNightMode = enable;
    result = setFromAdvancedConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enablePairing(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.pairingEnabled = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}

bool NukiLock::pairingEnabled() {
  Config config;
  Nuki::CmdResult result = requestConfig(&config);
  if (result == Nuki::CmdResult::Success) {
    return config.pairingEnabled;
  }
  return false;
}

Nuki::CmdResult NukiLock::enableLedFlash(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.ledEnabled = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setLedBrightness(const uint8_t level) {
  //level is from 0 (off) to 5(max)
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.ledBrightness = level > 5 ? 5 : level;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::enableSingleLock(const bool enable) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.singleLock = enable;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::setAdvertisingMode(const AdvertisingMode mode) {
  Config oldConfig;
  Nuki::CmdResult result = requestConfig(&oldConfig);
  if (result == Nuki::CmdResult::Success) {
    oldConfig.advertisingMode = mode;
    result = setFromConfig(oldConfig);
  }
  return result;
}

Nuki::CmdResult NukiLock::scanWifi(uint8_t scanDurationSeconds) {
  Action action;
  unsigned char payload[1] = {0};
  memcpy(payload, &scanDurationSeconds, 1);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::ScanWifi;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  listOfWifiScanEntries.clear();

  Nuki::CmdResult result = executeAction(action);
  return result;
}

void NukiLock::getWifiScanEntries(std::list<WifiScanEntry>* wifiScanEntries) {
  wifiScanEntries->clear();
  std::list<WifiScanEntry>::iterator it = listOfWifiScanEntries.begin();
  while (it != listOfWifiScanEntries.end()) {
    wifiScanEntries->push_back(*it);
    it++;
  }
}

Nuki::CmdResult NukiLock::addTimeControlEntry(NewTimeControlEntry newTimeControlEntry) {
//TODO verify data validity
  Action action{};
  unsigned char payload[sizeof(NewTimeControlEntry)] = {0};
  memcpy(payload, &newTimeControlEntry, sizeof(NewTimeControlEntry));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::AddTimeControlEntry;
  memcpy(action.payload, &payload, sizeof(NewTimeControlEntry));
  action.payloadLen = sizeof(NewTimeControlEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {      
      ESP_LOGD("NukiBle.NukiLock", "addTimeControlEntry, payloadlen: %d", sizeof(NewTimeControlEntry));
      printBuffer(action.payload, sizeof(NewTimeControlEntry), false, "new time control content: ", debugNukiHexData);
      logNewTimeControlEntry(newTimeControlEntry, true);
    }
  }
  return result;
}

Nuki::CmdResult NukiLock::updateTimeControlEntry(TimeControlEntry TimeControlEntry) {
  //TODO verify data validity
  Action action{};
  unsigned char payload[sizeof(TimeControlEntry)] = {0};
  memcpy(payload, &TimeControlEntry, sizeof(TimeControlEntry));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateTimeControlEntry;
  memcpy(action.payload, &payload, sizeof(TimeControlEntry));
  action.payloadLen = sizeof(TimeControlEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {      
      ESP_LOGD("NukiBle.NukiLock", "addTimeControlEntry, payloadlen: %d", sizeof(TimeControlEntry));
      printBuffer(action.payload, sizeof(TimeControlEntry), false, "updated time control content: ", debugNukiHexData);
      logTimeControlEntry(TimeControlEntry, true);
    }
  }
  return result;
}

Nuki::CmdResult NukiLock::removeTimeControlEntry(uint8_t entryId) {
//TODO verify data validity
  Action action{};
  unsigned char payload[1] = {0};
  memcpy(payload, &entryId, 1);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveTimeControlEntry;
  memcpy(action.payload, &payload, 1);
  action.payloadLen = 1;

  return executeAction(action);
}

Nuki::CmdResult NukiLock::retrieveTimeControlEntries() {
  Action action{};

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestTimeControlEntries;
  action.payloadLen = 0;

  listOfTimeControlEntries.clear();

  return executeAction(action);
}

void NukiLock::getTimeControlEntries(std::list<TimeControlEntry>* requestedTimeControlEntries) {
  requestedTimeControlEntries->clear();
  std::list<TimeControlEntry>::iterator it = listOfTimeControlEntries.begin();
  while (it != listOfTimeControlEntries.end()) {
    requestedTimeControlEntries->push_back(*it);
    it++;
  }
}

void NukiLock::getLogEntries(std::list<LogEntry>* requestedLogEntries) {
  requestedLogEntries->clear();

  for (const auto& it : listOfLogEntries) {
    requestedLogEntries->push_back(it);
  }
}

void NukiLock::getInternalLogEntries(std::list<InternalLogEntry>* requestedInternalLogEntries) {
  requestedInternalLogEntries->clear();

  for (const auto& it : listOfInternalLogEntries) {
    requestedInternalLogEntries->push_back(it);
  }
}

Nuki::CmdResult NukiLock::retrieveLogEntries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount) {
  Action action{};
  unsigned char payload[8] = {0};
  memcpy(payload, &startIndex, 4);
  memcpy(&payload[4], &count, 2);
  memcpy(&payload[6], &sortOrder, 1);
  memcpy(&payload[7], &totalCount, 1);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestLogEntries;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  listOfLogEntries.clear();

  return executeAction(action);
}

Nuki::CmdResult NukiLock::getAccessoryInfo(const uint8_t accessoryType) {
  Action action;
  unsigned char payload[1] = {0};
  memcpy(payload, &accessoryType, 1);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestAccessoryInfo;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);
  return executeAction(action);
}

bool NukiLock::isBatteryCritical() {
  if(keyTurnerState.criticalBatteryState != 255) {
    return ((keyTurnerState.criticalBatteryState & 1) == 1);
  }
  return false;
}

bool NukiLock::isKeypadBatteryCritical() {
  if(keyTurnerState.accessoryBatteryState != 255) {
    if ((keyTurnerState.accessoryBatteryState & 1) == 1) {
      return ((keyTurnerState.accessoryBatteryState & 3) == 3);
    }
  }
  return false;
}

bool NukiLock::isDoorSensorBatteryCritical() {
  if(keyTurnerState.accessoryBatteryState != 255) {
    if ((keyTurnerState.accessoryBatteryState & 4) == 4) {
      return ((keyTurnerState.accessoryBatteryState & 12) == 12);
    }
  }
  return false;
}

bool NukiLock::isBatteryCharging() {
  if(keyTurnerState.criticalBatteryState != 255) {
    return ((keyTurnerState.criticalBatteryState & 2) == 2);
  }
  return false;
}

uint8_t NukiLock::getBatteryPerc() {
  return (keyTurnerState.criticalBatteryState & 0b11111100) >> 1;
}

const ErrorCode NukiLock::getLastError() const {
  return (ErrorCode)errorCode;
}

Nuki::CmdResult NukiLock::setConfig(NewConfig newConfig) {
  Action action{};
  unsigned char payload[sizeof(NewConfig)] = {0};
  memcpy(payload, &newConfig, sizeof(NewConfig));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetConfig;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return executeAction(action);
}

Nuki::CmdResult NukiLock::setFromConfig(const Config config) {
  NewConfig newConfig;
  createNewConfig(&config, &newConfig);
  return setConfig(newConfig);
}

Nuki::CmdResult NukiLock::setFromAdvancedConfig(const AdvancedConfig config) {
  NewAdvancedConfig newConfig;
  createNewAdvancedConfig(&config, &newConfig);
  return setAdvancedConfig(newConfig);
}

Nuki::CmdResult NukiLock::setAdvancedConfig(NewAdvancedConfig newAdvancedConfig) {
  Action action{};
  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetAdvancedConfig;

  if (isLockUltra()) {
    unsigned char payload[sizeof(NewAdvancedConfig)] = {0};
    memcpy(payload, &newAdvancedConfig, sizeof(NewAdvancedConfig));
    memcpy(action.payload, &payload, sizeof(payload));
    action.payloadLen = sizeof(payload);      
  } else {
    unsigned char payload[sizeof(NewAdvancedConfig) - 2] = {0};
    memcpy(payload, &newAdvancedConfig, sizeof(NewAdvancedConfig) - 2);
    memcpy(action.payload, &payload, sizeof(payload));
    action.payloadLen = sizeof(payload);
  }
  return executeAction(action);
}


void NukiLock::createNewConfig(const Config* oldConfig, NewConfig* newConfig) {
  memcpy(newConfig->name, oldConfig->name, sizeof(newConfig->name));
  newConfig->latitude = oldConfig->latitude;
  newConfig->longitude = oldConfig->longitude;
  newConfig->autoUnlatch = oldConfig->autoUnlatch;
  newConfig->pairingEnabled = oldConfig->pairingEnabled;
  newConfig->buttonEnabled = oldConfig->buttonEnabled;
  newConfig->ledEnabled = oldConfig->ledEnabled;
  newConfig->ledBrightness = oldConfig->ledBrightness;
  newConfig->timeZoneOffset = oldConfig->timeZoneOffset;
  newConfig->dstMode = oldConfig->dstMode;
  newConfig->fobAction1 = oldConfig->fobAction1;
  newConfig->fobAction2 = oldConfig->fobAction2;
  newConfig->fobAction3 = oldConfig->fobAction3;
  newConfig->singleLock = oldConfig->singleLock;
  newConfig->advertisingMode = oldConfig->advertisingMode;
  newConfig->timeZoneId = oldConfig->timeZoneId;
}

void NukiLock::createNewAdvancedConfig(const AdvancedConfig* oldConfig, NewAdvancedConfig* newConfig) {
  newConfig->unlockedPositionOffsetDegrees = oldConfig->unlockedPositionOffsetDegrees;
  newConfig->lockedPositionOffsetDegrees = oldConfig->lockedPositionOffsetDegrees;
  newConfig->singleLockedPositionOffsetDegrees = oldConfig->singleLockedPositionOffsetDegrees;
  newConfig->unlockedToLockedTransitionOffsetDegrees = oldConfig->unlockedToLockedTransitionOffsetDegrees;
  newConfig->lockNgoTimeout = oldConfig->lockNgoTimeout;
  newConfig->singleButtonPressAction = oldConfig->singleButtonPressAction;
  newConfig->doubleButtonPressAction = oldConfig->doubleButtonPressAction;
  newConfig->detachedCylinder = oldConfig->detachedCylinder;
  newConfig->batteryType = oldConfig->batteryType;
  newConfig->automaticBatteryTypeDetection = oldConfig->automaticBatteryTypeDetection;
  newConfig->unlatchDuration = oldConfig->unlatchDuration;
  newConfig->autoLockTimeOut = oldConfig->autoLockTimeOut;
  newConfig->autoUnLockDisabled = oldConfig->autoUnLockDisabled;
  newConfig->nightModeEnabled = oldConfig->nightModeEnabled;
  memcpy(newConfig->nightModeStartTime, oldConfig->nightModeStartTime, sizeof(newConfig->nightModeStartTime));
  memcpy(newConfig->nightModeEndTime, oldConfig->nightModeEndTime, sizeof(newConfig->nightModeEndTime));
  newConfig->nightModeAutoLockEnabled = oldConfig->nightModeAutoLockEnabled;
  newConfig->nightModeAutoUnlockDisabled = oldConfig->nightModeAutoUnlockDisabled;
  newConfig->nightModeImmediateLockOnStart = oldConfig->nightModeImmediateLockOnStart;
  newConfig->autoLockEnabled = oldConfig->autoLockEnabled;
  newConfig->immediateAutoLockEnabled = oldConfig->immediateAutoLockEnabled;
  newConfig->autoUpdateEnabled = oldConfig->autoUpdateEnabled;
  newConfig->motorSpeed = oldConfig->motorSpeed;
  newConfig->enableSlowSpeedDuringNightMode = oldConfig->enableSlowSpeedDuringNightMode;
}

void NukiLock::handleReturnMessage(Command returnCode, unsigned char* data, uint16_t dataLen) {
  extendDisconnectTimeout();

  switch (returnCode) {
    case Command::KeyturnerStates : {
      printBuffer((uint8_t*)data, dataLen, false, "keyturnerStates", debugNukiHexData);
      memcpy(&keyTurnerState, data, dataLen);
      if (debugNukiReadableData) {
        logKeyturnerState(keyTurnerState, true);
      }
      break;
    }
    case Command::BatteryReport : {
      printBuffer((uint8_t*)data, dataLen, false, "batteryReport", debugNukiHexData);
      memcpy(&batteryReport, data, dataLen);
      if (debugNukiReadableData) {
        logBatteryReport(batteryReport, true);
      }
      break;
    }
    case Command::Config : {
      memcpy(&config, data, dataLen);
      if (debugNukiReadableData) {
        logConfig(config, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "config", debugNukiHexData);
      break;
    }
    case Command::AdvancedConfig : {
      memcpy(&advancedConfig, data, dataLen);
      if (debugNukiReadableData) {
        logAdvancedConfig(advancedConfig, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "advancedConfig", debugNukiHexData);
      break;
    }
    case Command::TimeControlEntry : {
      printBuffer((uint8_t*)data, dataLen, false, "timeControlEntry", debugNukiHexData);
      TimeControlEntry timeControlEntry;
      memcpy(&timeControlEntry, data, dataLen);
      listOfTimeControlEntries.push_back(timeControlEntry);
      break;
    }
    case Command::LogEntry : {
      printBuffer((uint8_t*)data, dataLen, false, "logEntry", debugNukiHexData);
      LogEntry logEntry;
      memcpy(&logEntry, data, dataLen);
      listOfLogEntries.push_back(logEntry);
      if (debugNukiReadableData) {
        logLogEntry(logEntry, true);
      }
      break;
    }
    case Command::InternalLogEntry : {
      printBuffer((uint8_t*)data, dataLen, false, "internalLogEntry", debugNukiHexData);
      InternalLogEntry internalLogEntry;
      memcpy(&internalLogEntry, data, dataLen);
      listOfInternalLogEntries.push_back(internalLogEntry);
      if (debugNukiReadableData) {
        logInternalLogEntry(internalLogEntry, true);
      }
      break;
    }
    case Command::MqttConfig :
      memcpy(&mqttConfig, data, dataLen);
      if (debugNukiReadableData) {
        logMqttConfig(mqttConfig, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "mqttConfig", debugNukiHexData);
      break;
    case Command::MqttConfigForMigration : {
      memcpy(&mqttConfigForMigration, data, dataLen);
      if (debugNukiReadableData) {
        logMqttConfigForMigration(mqttConfigForMigration, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "mqttConfigForMigration", debugNukiHexData);
      break;
    }
    case Command::WifiScanEntry : {
      printBuffer((uint8_t*)data, dataLen, false, "wifiScanEntry", debugNukiHexData);
      WifiScanEntry wifiScanEntry;
      memcpy(&wifiScanEntry, data, dataLen);
      listOfWifiScanEntries.push_back(wifiScanEntry);
      if (debugNukiReadableData) {
        logWifiScanEntry(wifiScanEntry, true);
      }
      break;
    }
    case Command::WifiConfig : {
      memcpy(&wifiConfig, data, dataLen);
      if (debugNukiReadableData) {
        logWifiConfig(wifiConfig, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "wifiConfig", debugNukiHexData);
      break;
    }
    case Command::WifiConfigForMigration : {
      memcpy(&wifiConfigForMigration, data, dataLen);
      if (debugNukiReadableData) {
        logWifiConfigForMigration(wifiConfigForMigration, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "wifiConfigForMigration", debugNukiHexData);
      break;
    }
    case Command::Keypad2Config : {
      memcpy(&keypad2Config, data, dataLen);
      if (debugNukiReadableData) {
        logKeypad2Config(keypad2Config, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "keypad2Config", debugNukiHexData);
      break;
    }
    case Command::GeneralStatistics : {
      memcpy(&generalStatistics, data, dataLen);
      if (debugNukiReadableData) {
        logGeneralStatistics(generalStatistics, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "generalStatistics", debugNukiHexData);
      break;
    }
    case Command::DailyStatistics : {
      memcpy(&dailyStatistics, data, dataLen);
      if (debugNukiReadableData) {
        logDailyStatistics(dailyStatistics, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "dailyStatistics", debugNukiHexData);
      break;
    }
    case Command::AccessoryInfo : {
      memcpy(&accessoryInfo, data, dataLen);
      if (debugNukiReadableData) {
        logAccessoryInfo(accessoryInfo, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "accessoryInfo", debugNukiHexData);
      break;
    }
    case Command::DoorSensorConfig : {
      memcpy(&doorSensorConfig, data, dataLen);
      if (debugNukiReadableData) {
        logDoorSensorConfig(doorSensorConfig, true);
      }
      printBuffer((uint8_t*)data, dataLen, false, "doorSensorConfig", debugNukiHexData);
      break;
    }
    default:
      NukiBle::handleReturnMessage(returnCode, data, dataLen);
  }
  lastMsgCodeReceived = returnCode;
}

void NukiLock::logErrorCode(uint8_t errorCode) {
  logLockErrorCode(errorCode, debugNukiReadableData);
}

}