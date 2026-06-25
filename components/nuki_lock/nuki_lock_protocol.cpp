#include "nuki_lock_protocol_utils.h"
#include "nuki_lock_protocol.h"
#include "nuki_ble_utils.h"

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

#include "esphome/core/log.h"

namespace esphome::nuki_lock {

static const char *const TAG = "NukiBle.NukiLock";

NukiLock::NukiLock(const std::string& device_name, const uint32_t device_id)
  : NukiBle(device_name,
            device_id,
            keyturnerPairingServiceUUID,
            keyturnerPairingServiceUltraUUID,
            keyturnerServiceUUID,
            keyturnerGdioUUID,
            keyturnerGdioUltraUUID,
            keyturnerUserDataUUID,
            device_name) {
    this->error_code_ = (uint8_t)ErrorCode::ERROR_UNKNOWN;
}

CmdResult NukiLock::lock_action(const LockAction lock_action, const uint32_t nukiAppId, const uint8_t flags, const char* nameSuffix, const uint8_t nameSuffixLen) {
  NukiAction action{};
  unsigned char payload[sizeof(LockAction) + 4 + 1 + 20] = {0};
  memcpy(payload, &lock_action, sizeof(LockAction));
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

  action.cmdType = CommandType::CommandWithChallengeAndAccept;
  action.command = Command::LockAction;
  memcpy(action.payload, &payload, payloadLen);
  action.payloadLen = payloadLen;

  return this->execute_action(action);
}

CmdResult NukiLock::keypad_action(KeypadActionSource source, uint32_t code, KeypadAction keypad_action) {
  NukiAction action{};
  unsigned char payload[6] = {(unsigned char)source};
  memcpy(&payload[1], &code, sizeof(code));
  memcpy(&payload[1+sizeof(code)], &keypad_action, sizeof(KeypadAction));
  uint8_t payloadLen = 6;

  action.cmdType = CommandType::CommandWithChallengeAndAccept;
  action.command = Command::KeypadAction;
  memcpy(action.payload, &payload, payloadLen);
  action.payloadLen = payloadLen;

  return this->execute_action(action);
}

CmdResult NukiLock::request_key_turner_state(KeyTurnerState* retrievedKeyTurnerState) {
  NukiAction action{};
  uint16_t payload = (uint16_t)Command::KeyturnerStates;

  action.cmdType = CommandType::Command;
  action.command = Command::RequestData;
  memcpy(&action.payload[0], &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    // print_buffer((uint8_t*)&retrievedKeyTurnerState, sizeof(retrievedKeyTurnerState), false, "retrieved Keyturner state", debug_nuki_hex_data);
    memcpy(retrievedKeyTurnerState, &this->key_turner_state_, sizeof(KeyTurnerState));
  }
  return result;
}

void NukiLock::retrieve_key_tuner_state(KeyTurnerState* retrievedKeyTurnerState) {
  memcpy(retrievedKeyTurnerState, &this->key_turner_state_, sizeof(KeyTurnerState));
}

CmdResult NukiLock::request_battery_report(BatteryReport* retrievedBatteryReport) {
  NukiAction action{};
  uint16_t payload = (uint16_t)Command::BatteryReport;

  action.cmdType = CommandType::Command;
  action.command = Command::RequestData;
  memcpy(&action.payload[0], &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    memcpy(retrievedBatteryReport, &this->battery_report_, sizeof(this->battery_report_));
  }
  return result;
}

CmdResult NukiLock::request_config(Config* retrievedConfig) {
  NukiAction action{};
  action.cmdType = CommandType::CommandWithChallenge;
  action.command = Command::RequestConfig;

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    memcpy(retrievedConfig, &this->config_, sizeof(Config));
  }
  return result;
}

CmdResult NukiLock::request_advanced_config(AdvancedConfig* retrievedAdvancedConfig) {
  NukiAction action{};
  action.cmdType = CommandType::CommandWithChallenge;
  action.command = Command::RequestAdvancedConfig;

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    memcpy(retrievedAdvancedConfig, &this->advanced_config_, sizeof(AdvancedConfig));
  }
  return result;
}

//basic config change methods
CmdResult NukiLock::set_name(const std::string& name) {
  if (name.length() > 32) {
    ESP_LOGW(TAG, "set_name, too long (max32)");
    return CmdResult::Failed;
  }

  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  memset(config->name, 0, sizeof(config->name));
  memcpy(config->name, name.c_str(), name.length());
  return this->commit_config_write();
}

CmdResult NukiLock::set_latitude(const float degrees) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->latitude = degrees;
  return this->commit_config_write();
}

CmdResult NukiLock::set_longitude(const float degrees) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->longitude = degrees;
  return this->commit_config_write();
}

CmdResult NukiLock::enable_auto_unlatch(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->autoUnlatch = enable;
  return this->commit_config_write();
}

CmdResult NukiLock::set_fob_action(const uint8_t fobActionNr, const uint8_t fobAction) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  switch (fobActionNr) {
    case 1:
      config->fobAction1 = fobAction;
      break;
    case 2:
      config->fobAction2 = fobAction;
      break;
    case 3:
      config->fobAction3 = fobAction;
      break;
    default:
      // Invalid fobActionNr - abandon the fetched config rather than leaving it pending,
      // since this call never reaches commit_config_write() to do that itself.
      this->config_write_pending_ = false;
      return CmdResult::Error;
  }
  return this->commit_config_write();
}

CmdResult NukiLock::retrieve_internal_log_entries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount) {
  NukiAction action;
  unsigned char payload[8] = {0};
  memcpy(payload, &startIndex, 4);
  memcpy(&payload[4], &count, 2);
  memcpy(&payload[6], &sortOrder, 1);
  memcpy(&payload[7], &totalCount, 1);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestInternalLogEntries;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  this->list_of_internal_log_entries_.clear();

  return this->execute_action(action);
}

CmdResult NukiLock::enable_dst(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->dstMode = enable;
  return this->commit_config_write();
}

CmdResult NukiLock::set_time_zone_offset(const int16_t minutes) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->timeZoneOffset = minutes;
  return this->commit_config_write();
}

CmdResult NukiLock::set_time_zone_id(const TimeZoneId timeZoneId) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->timeZoneId = timeZoneId;
  return this->commit_config_write();
}

CmdResult NukiLock::enable_button(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->buttonEnabled = enable;
  return this->commit_config_write();
}

//advanced config change methods
CmdResult NukiLock::set_unlocked_position_offset_degrees(const int16_t degrees) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->unlockedPositionOffsetDegrees = degrees;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_locked_position_offset_degrees(const int16_t degrees) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->lockedPositionOffsetDegrees = degrees;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_single_locked_position_offset_degrees(const int16_t degrees) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->singleLockedPositionOffsetDegrees = degrees;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_unlocked_to_locked_transition_offset_degrees(const int16_t degrees) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->unlockedToLockedTransitionOffsetDegrees = degrees;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_lock_ngo_timeout(const uint8_t timeout) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->lockNgoTimeout = timeout;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_detached_cylinder(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->detachedCylinder = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_unlatch_duration(const uint8_t duration) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->unlatchDuration = duration;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_auto_lock_time_out(const uint8_t timeout) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->autoLockTimeOut = timeout;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_night_mode(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeEnabled = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_night_mode_start_time(unsigned char starttime[2]) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeStartTime[0] = starttime[0];
  config->nightModeStartTime[1] = starttime[1];    
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_night_mode_end_time(unsigned char endtime[2]) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeEndTime[0] = endtime[0];
  config->nightModeEndTime[1] = endtime[1];    
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_night_mode_auto_lock(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeAutoLockEnabled = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::disable_night_mode_auto_unlock(const bool disable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeAutoUnlockDisabled = disable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_night_mode_immediate_lock_on_start(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->nightModeImmediateLockOnStart = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_single_button_press_action(const ButtonPressAction action) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->singleButtonPressAction = action;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_double_button_press_action(const ButtonPressAction action) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->doubleButtonPressAction = action;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_battery_type(const BatteryType type) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->batteryType = type;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_auto_battery_type_detection(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->automaticBatteryTypeDetection = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::disable_auto_unlock(const bool disable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->autoUnLockDisabled = disable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_auto_lock(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->autoLockEnabled = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_immediate_auto_lock(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->immediateAutoLockEnabled = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_auto_update(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->autoUpdateEnabled = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::set_motor_speed(const MotorSpeed speed) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->motorSpeed = speed;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_slow_speed_during_night_mode(const bool enable) {
  AdvancedConfig* config;
  CmdResult result = this->begin_advanced_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->enable_slow_speed_during_night_mode = enable;
  return this->commit_advanced_config_write();
}

CmdResult NukiLock::enable_pairing(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->pairing_enabled = enable;
  return this->commit_config_write();
}

bool NukiLock::pairing_enabled() {
  Config config;
  CmdResult result = this->request_config(&config);
  if (result == CmdResult::Success) {
    return config.pairing_enabled;
  }
  return false;
}

CmdResult NukiLock::enable_led_flash(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->ledEnabled = enable;
  return this->commit_config_write();
}

CmdResult NukiLock::set_led_brightness(const uint8_t level) {
  //level is from 0 (off) to 5(max)
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->ledBrightness = level > 5 ? 5 : level;
  return this->commit_config_write();
}

CmdResult NukiLock::enable_single_lock(const bool enable) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->singleLock = enable;
  return this->commit_config_write();
}

CmdResult NukiLock::set_advertising_mode(const AdvertisingMode mode) {
  Config* config;
  CmdResult result = this->begin_config_write(&config);
  if (result != CmdResult::Success) {
    return result;
  }
  config->advertisingMode = mode;
  return this->commit_config_write();
}

CmdResult NukiLock::scan_wifi(uint8_t scanDurationSeconds) {
  NukiAction action;
  unsigned char payload[1] = {0};
  memcpy(payload, &scanDurationSeconds, 1);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::ScanWifi;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  this->list_of_wifi_scan_entries_.clear();

  CmdResult result = this->execute_action(action);
  return result;
}

void NukiLock::get_wifi_scan_entries(std::vector<WifiScanEntry>* wifiScanEntries) {
  wifiScanEntries->clear();
  for (const auto &entry : this->list_of_wifi_scan_entries_) {
    wifiScanEntries->push_back(entry);
  }
}

CmdResult NukiLock::add_time_control_entry(NewTimeControlEntry newTimeControlEntry) {
//TODO verify data validity
  NukiAction action{};
  unsigned char payload[sizeof(NewTimeControlEntry)] = {0};
  memcpy(payload, &newTimeControlEntry, sizeof(NewTimeControlEntry));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::AddTimeControlEntry;
  memcpy(action.payload, &payload, sizeof(NewTimeControlEntry));
  action.payloadLen = sizeof(NewTimeControlEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {      
      ESP_LOGD(TAG, "add_time_control_entry, payloadlen: %d", sizeof(NewTimeControlEntry));
      print_buffer(action.payload, sizeof(NewTimeControlEntry), false, "new time control content: ", this->debug_nuki_hex_data_);
      log_new_time_control_entry(newTimeControlEntry, true);
    }
  }
  return result;
}

CmdResult NukiLock::update_time_control_entry(TimeControlEntry TimeControlEntry) {
  //TODO verify data validity
  NukiAction action{};
  unsigned char payload[sizeof(TimeControlEntry)] = {0};
  memcpy(payload, &TimeControlEntry, sizeof(TimeControlEntry));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateTimeControlEntry;
  memcpy(action.payload, &payload, sizeof(TimeControlEntry));
  action.payloadLen = sizeof(TimeControlEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {      
      ESP_LOGD(TAG, "add_time_control_entry, payloadlen: %d", sizeof(TimeControlEntry));
      print_buffer(action.payload, sizeof(TimeControlEntry), false, "updated time control content: ", this->debug_nuki_hex_data_);
      log_time_control_entry(TimeControlEntry, true);
    }
  }
  return result;
}

CmdResult NukiLock::remove_time_control_entry(uint8_t entryId) {
//TODO verify data validity
  NukiAction action{};
  unsigned char payload[1] = {0};
  memcpy(payload, &entryId, 1);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveTimeControlEntry;
  memcpy(action.payload, &payload, 1);
  action.payloadLen = 1;

  return this->execute_action(action);
}

CmdResult NukiLock::retrieve_time_control_entries() {
  NukiAction action{};

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestTimeControlEntries;
  action.payloadLen = 0;

  this->list_of_time_control_entries_.clear();

  return this->execute_action(action);
}

void NukiLock::get_time_control_entries(std::vector<TimeControlEntry>* requestedTimeControlEntries) {
  requestedTimeControlEntries->clear();
  for (const auto &entry : this->list_of_time_control_entries_) {
    requestedTimeControlEntries->push_back(entry);
  }
}

void NukiLock::get_log_entries(std::vector<LogEntry>* requestedLogEntries) {
  requestedLogEntries->clear();

  for (const auto& it : this->list_of_log_entries_) {
    requestedLogEntries->push_back(it);
  }
}

void NukiLock::get_internal_log_entries(std::vector<InternalLogEntry>* requestedInternalLogEntries) {
  requestedInternalLogEntries->clear();

  for (const auto& it : this->list_of_internal_log_entries_) {
    requestedInternalLogEntries->push_back(it);
  }
}

CmdResult NukiLock::retrieve_log_entries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount) {
  NukiAction action{};
  unsigned char payload[8] = {0};
  memcpy(payload, &startIndex, 4);
  memcpy(&payload[4], &count, 2);
  memcpy(&payload[6], &sortOrder, 1);
  memcpy(&payload[7], &totalCount, 1);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestLogEntries;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  this->list_of_log_entries_.clear();

  return this->execute_action(action);
}

bool NukiLock::is_battery_critical() {
  if(this->key_turner_state_.criticalBatteryState != 255) {
    return ((this->key_turner_state_.criticalBatteryState & 1) == 1);
  }
  return false;
}

bool NukiLock::is_door_sensor_battery_critical() {
  if(this->key_turner_state_.accessoryBatteryState != 255) {
    if ((this->key_turner_state_.accessoryBatteryState & 4) == 4) {
      return ((this->key_turner_state_.accessoryBatteryState & 12) == 12);
    }
  }
  return false;
}

bool NukiLock::is_battery_charging() {
  if(this->key_turner_state_.criticalBatteryState != 255) {
    return ((this->key_turner_state_.criticalBatteryState & 2) == 2);
  }
  return false;
}

uint8_t NukiLock::get_battery_perc() {
  return (this->key_turner_state_.criticalBatteryState & 0b11111100) >> 1;
}

const ErrorCode NukiLock::get_last_error() const {
  return (ErrorCode)this->error_code_;
}

CmdResult NukiLock::set_config(NewConfig newConfig) {
  NukiAction action{};
  unsigned char payload[sizeof(NewConfig)] = {0};
  memcpy(payload, &newConfig, sizeof(NewConfig));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetConfig;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return this->execute_action(action);
}

CmdResult NukiLock::set_from_config(const Config config) {
  NewConfig newConfig;
  this->create_new_config(&config, &newConfig);
  return this->set_config(newConfig);
}

CmdResult NukiLock::set_from_advanced_config(const AdvancedConfig config) {
  NewAdvancedConfig newConfig;
  this->create_new_advanced_config(&config, &newConfig);
  return this->set_advanced_config(newConfig);
}

CmdResult NukiLock::set_advanced_config(NewAdvancedConfig newAdvancedConfig) {
  NukiAction action{};
  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetAdvancedConfig;

  if (this->is_lock_ultra()) {
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
  return this->execute_action(action);
}

void NukiLock::create_new_config(const Config* oldConfig, NewConfig* newConfig) {
  memcpy(newConfig->name, oldConfig->name, sizeof(newConfig->name));
  newConfig->latitude = oldConfig->latitude;
  newConfig->longitude = oldConfig->longitude;
  newConfig->autoUnlatch = oldConfig->autoUnlatch;
  newConfig->pairing_enabled = oldConfig->pairing_enabled;
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

void NukiLock::create_new_advanced_config(const AdvancedConfig* oldConfig, NewAdvancedConfig* newConfig) {
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
  newConfig->enable_slow_speed_during_night_mode = oldConfig->enable_slow_speed_during_night_mode;
}

// Read-modify-write helpers shared by every Config-based setter (set_latitude,
// enable_auto_unlatch, set_fob_action, ...). These setters are called repeatedly (once per
// loop() tick) until they return a terminal CmdResult, like every other Nuki command - so
// unlike a one-shot blocking call, the "request current config, then write the modified
// config back" sequence can't just be two unconditional steps in a row: by the time a second
// tick re-invokes the setter, the *previous* tick's request_config() may have already
// succeeded, and re-running it would misattribute the in-flight set_from_config()'s response
// to a phantom new request_config() call, sending the write twice. config_write_pending_
// remembers "config already fetched, just keep advancing the write" across those repeated
// calls, the same way pairing_state_ does for pair_nuki().
//
// If config_ was refreshed (by this or any other request_config(), e.g. the periodic status
// poll) within config_cache_ttl_ms_, that snapshot is reused instead of fetching a fresh one -
// trading a small risk of overwriting a change made elsewhere (Nuki app, another authorized
// device) in that window for fewer BLE round-trips on frequently-changed settings.
// config_cache_ttl_ms_ defaults to 0 (always fetch fresh).
CmdResult NukiLock::begin_config_write(Config** out_config) {
  if (!this->config_write_pending_) {
    bool cache_fresh = this->config_cache_ttl_ms_ > 0 && this->config_fetched_at_ms_ >= 0 &&
        (esp_timer_get_time() / 1000) - this->config_fetched_at_ms_ < this->config_cache_ttl_ms_;
    if (cache_fresh) {
      this->config_write_buffer_ = this->config_;
    } else {
      CmdResult result = this->request_config(&this->config_write_buffer_);
      if (result != CmdResult::Success) {
        return result;  // Working: keep waiting; anything else: propagate the failure as-is
      }
    }
    this->config_write_pending_ = true;
  }
  *out_config = &this->config_write_buffer_;
  return CmdResult::Success;
}

CmdResult NukiLock::commit_config_write() {
  CmdResult result = this->set_from_config(this->config_write_buffer_);
  if (result != CmdResult::Working) {
    this->config_write_pending_ = false;
  }
  return result;
}

CmdResult NukiLock::begin_advanced_config_write(AdvancedConfig** out_config) {
  if (!this->advanced_config_write_pending_) {
    bool cache_fresh = this->config_cache_ttl_ms_ > 0 && this->advanced_config_fetched_at_ms_ >= 0 &&
        (esp_timer_get_time() / 1000) - this->advanced_config_fetched_at_ms_ < this->config_cache_ttl_ms_;
    if (cache_fresh) {
      this->advanced_config_write_buffer_ = this->advanced_config_;
    } else {
      CmdResult result = this->request_advanced_config(&this->advanced_config_write_buffer_);
      if (result != CmdResult::Success) {
        return result;
      }
    }
    this->advanced_config_write_pending_ = true;
  }
  *out_config = &this->advanced_config_write_buffer_;
  return CmdResult::Success;
}

CmdResult NukiLock::commit_advanced_config_write() {
  CmdResult result = this->set_from_advanced_config(this->advanced_config_write_buffer_);
  if (result != CmdResult::Working) {
    this->advanced_config_write_pending_ = false;
  }
  return result;
}

void NukiLock::handle_return_message(Command returnCode, unsigned char* data, uint16_t dataLen) {
  this->extend_disconnect_timeout();

  switch (returnCode) {
    case Command::KeyturnerStates : {
      print_buffer((uint8_t*)data, dataLen, false, "keyturnerStates", this->debug_nuki_hex_data_);
      memcpy(&this->key_turner_state_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_keyturner_state(this->key_turner_state_, true);
      }
      break;
    }
    case Command::BatteryReport : {
      print_buffer((uint8_t*)data, dataLen, false, "battery_report", this->debug_nuki_hex_data_);
      memcpy(&this->battery_report_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_battery_report(this->battery_report_, true);
      }
      break;
    }
    case Command::Config : {
      memcpy(&this->config_, data, dataLen);
      this->config_fetched_at_ms_ = esp_timer_get_time() / 1000;
      if (this->debug_nuki_readable_data_) {
        log_config(this->config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "config", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AdvancedConfig : {
      memcpy(&this->advanced_config_, data, dataLen);
      this->advanced_config_fetched_at_ms_ = esp_timer_get_time() / 1000;
      if (this->debug_nuki_readable_data_) {
        log_advanced_config(this->advanced_config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "advanced_config", this->debug_nuki_hex_data_);
      break;
    }
    case Command::TimeControlEntry : {
      print_buffer((uint8_t*)data, dataLen, false, "timeControlEntry", this->debug_nuki_hex_data_);
      TimeControlEntry timeControlEntry;
      memcpy(&timeControlEntry, data, dataLen);
      this->list_of_time_control_entries_.push_back(timeControlEntry);
      break;
    }
    case Command::LogEntry : {
      print_buffer((uint8_t*)data, dataLen, false, "logEntry", this->debug_nuki_hex_data_);
      LogEntry logEntry;
      memcpy(&logEntry, data, dataLen);
      this->list_of_log_entries_.push_back(logEntry);
      if (this->debug_nuki_readable_data_) {
        log_log_entry(logEntry, true);
      }
      break;
    }
    case Command::InternalLogEntry : {
      print_buffer((uint8_t*)data, dataLen, false, "internalLogEntry", this->debug_nuki_hex_data_);
      InternalLogEntry internalLogEntry;
      memcpy(&internalLogEntry, data, dataLen);
      this->list_of_internal_log_entries_.push_back(internalLogEntry);
      if (this->debug_nuki_readable_data_) {
        log_internal_log_entry(internalLogEntry, true);
      }
      break;
    }
    case Command::MqttConfig :
      memcpy(&this->mqtt_config_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_mqtt_config(this->mqtt_config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "mqtt_config", this->debug_nuki_hex_data_);
      break;
    case Command::MqttConfigForMigration : {
      memcpy(&this->mqtt_config_for_migration_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_mqtt_config_for_migration(this->mqtt_config_for_migration_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "mqtt_config_for_migration", this->debug_nuki_hex_data_);
      break;
    }
    case Command::WifiScanEntry : {
      print_buffer((uint8_t*)data, dataLen, false, "wifiScanEntry", this->debug_nuki_hex_data_);
      WifiScanEntry wifiScanEntry;
      memcpy(&wifiScanEntry, data, dataLen);
      this->list_of_wifi_scan_entries_.push_back(wifiScanEntry);
      if (this->debug_nuki_readable_data_) {
        log_wifi_scan_entry(wifiScanEntry, true);
      }
      break;
    }
    case Command::WifiConfig : {
      memcpy(&this->wifi_config_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_wifi_config(this->wifi_config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "wifi_config", this->debug_nuki_hex_data_);
      break;
    }
    case Command::WifiConfigForMigration : {
      memcpy(&this->wifi_config_for_migration_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_wifi_config_for_migration(this->wifi_config_for_migration_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "wifi_config_for_migration", this->debug_nuki_hex_data_);
      break;
    }
    case Command::Keypad2Config : {
      memcpy(&this->keypad2_config_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_keypad2_config(this->keypad2_config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "keypad2_config", this->debug_nuki_hex_data_);
      break;
    }
    case Command::GeneralStatistics : {
      memcpy(&this->general_statistics_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_general_statistics(this->general_statistics_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "general_statistics", this->debug_nuki_hex_data_);
      break;
    }
    case Command::DailyStatistics : {
      memcpy(&this->daily_statistics_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_daily_statistics(this->daily_statistics_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "daily_statistics", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AccessoryInfo : {
      memcpy(&this->accessory_info_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_accessory_info(this->accessory_info_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "accessory_info", this->debug_nuki_hex_data_);
      break;
    }
    case Command::DoorSensorConfig : {
      memcpy(&this->door_sensor_config_, data, dataLen);
      if (this->debug_nuki_readable_data_) {
        log_door_sensor_config(this->door_sensor_config_, true);
      }
      print_buffer((uint8_t*)data, dataLen, false, "door_sensor_config", this->debug_nuki_hex_data_);
      break;
    }
    default:
      NukiBle::handle_return_message(returnCode, data, dataLen);
  }
  this->last_msg_code_received_ = returnCode;
}

void NukiLock::log_error_code(uint8_t error_code) {
  // Always show the human-readable name, not just behind debug_nuki_readable_data - this is
  // reporting an actual error from the lock, not a verbose protocol dump.
  log_lock_error_code(error_code, true);
}

}  // namespace esphome::nuki_lock