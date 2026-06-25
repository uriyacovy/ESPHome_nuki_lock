#include "nuki_accessories.h"
#include "nuki_ble.h"
#include "nuki_lock_protocol.h"
#include "nuki_lock.h"
#include "nuki_ble_utils.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "esphome/core/log.h"
#include "esp_timer.h"

namespace esphome::nuki_lock {

static const char *const TAG = "nuki_accessories";

void log_new_keypad_entry(NewKeypadEntry newKeypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "code:%d", (unsigned int)newKeypadEntry.code);
    ESP_LOGD(TAG, "name:%s", (const char*)newKeypadEntry.name);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)newKeypadEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)newKeypadEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)newKeypadEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)newKeypadEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)newKeypadEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)newKeypadEntry.allowedFromMin);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)newKeypadEntry.allowedFromSec);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)newKeypadEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)newKeypadEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)newKeypadEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)newKeypadEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)newKeypadEntry.allowedUntilMin);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)newKeypadEntry.allowedUntilSec);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)newKeypadEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)newKeypadEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)newKeypadEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)newKeypadEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)newKeypadEntry.allowedUntilTimeMin);
  }
}

void log_keypad_entry(KeypadEntry keypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "codeId:%d", (unsigned int)keypadEntry.codeId);
    ESP_LOGD(TAG, "code:%d", (unsigned int)keypadEntry.code);
    ESP_LOGD(TAG, "name:%s", (const char*)keypadEntry.name);
    ESP_LOGD(TAG, "enabled:%d", (unsigned int)keypadEntry.enabled);
    ESP_LOGD(TAG, "dateCreatedYear:%d", (unsigned int)keypadEntry.dateCreatedYear);
    ESP_LOGD(TAG, "dateCreatedMonth:%d", (unsigned int)keypadEntry.dateCreatedMonth);
    ESP_LOGD(TAG, "dateCreatedDay:%d", (unsigned int)keypadEntry.dateCreatedDay);
    ESP_LOGD(TAG, "dateCreatedHour:%d", (unsigned int)keypadEntry.dateCreatedHour);
    ESP_LOGD(TAG, "dateCreatedMin:%d", (unsigned int)keypadEntry.dateCreatedMin);
    ESP_LOGD(TAG, "dateCreatedSec:%d", (unsigned int)keypadEntry.dateCreatedSec);
    ESP_LOGD(TAG, "dateLastActiveYear:%d", (unsigned int)keypadEntry.dateLastActiveYear);
    ESP_LOGD(TAG, "dateLastActiveMonth:%d", (unsigned int)keypadEntry.dateLastActiveMonth);
    ESP_LOGD(TAG, "dateLastActiveDay:%d", (unsigned int)keypadEntry.dateLastActiveDay);
    ESP_LOGD(TAG, "dateLastActiveHour:%d", (unsigned int)keypadEntry.dateLastActiveHour);
    ESP_LOGD(TAG, "dateLastActiveMin:%d", (unsigned int)keypadEntry.dateLastActiveMin);
    ESP_LOGD(TAG, "dateLastActiveSec:%d", (unsigned int)keypadEntry.dateLastActiveSec);
    ESP_LOGD(TAG, "lockCount:%d", (unsigned int)keypadEntry.lockCount);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)keypadEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)keypadEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)keypadEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)keypadEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)keypadEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)keypadEntry.allowedFromMin);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)keypadEntry.allowedFromSec);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)keypadEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)keypadEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)keypadEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)keypadEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)keypadEntry.allowedUntilMin);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)keypadEntry.allowedUntilSec);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)keypadEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)keypadEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)keypadEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)keypadEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)keypadEntry.allowedUntilTimeMin);
  }
}

void log_updated_keypad_entry(UpdatedKeypadEntry updatedKeypadEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "codeId:%d", (unsigned int)updatedKeypadEntry.codeId);
    ESP_LOGD(TAG, "code:%d", (unsigned int)updatedKeypadEntry.code);
    ESP_LOGD(TAG, "name:%s", (const char*)updatedKeypadEntry.name);
    ESP_LOGD(TAG, "enabled:%d", (unsigned int)updatedKeypadEntry.enabled);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)updatedKeypadEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)updatedKeypadEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)updatedKeypadEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)updatedKeypadEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)updatedKeypadEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)updatedKeypadEntry.allowedFromMin);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)updatedKeypadEntry.allowedFromSec);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)updatedKeypadEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)updatedKeypadEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)updatedKeypadEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)updatedKeypadEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)updatedKeypadEntry.allowedUntilMin);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)updatedKeypadEntry.allowedUntilSec);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)updatedKeypadEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)updatedKeypadEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)updatedKeypadEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)updatedKeypadEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)updatedKeypadEntry.allowedUntilTimeMin);
  }
}

void log_fingerprint_entry(FingerprintEntry fingerprintEntry, bool debug) {
  if (debug) {
    char hexString[65]; // 32 bytes * 2 chars + 1 null terminator
    for (size_t i = 0; i < 32; i++) {
        sprintf(&hexString[i*2], "%02x", fingerprintEntry.fingerprintId[i]);
    }
    hexString[64] = '\0';

    ESP_LOGD(TAG, "fingerprintId: %s", hexString);
    ESP_LOGD(TAG, "keypadCodeId: %d", (unsigned int)fingerprintEntry.keypadCodeId);
    ESP_LOGD(TAG, "name: %s", fingerprintEntry.name);
  }
}

void log_keypad2_config(Keypad2Config keypad2Config, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "updatePending: %d", (unsigned int)keypad2Config.updatePending);
    ESP_LOGD(TAG, "ledBrightness: %d", (unsigned int)keypad2Config.ledBrightness);
    ESP_LOGD(TAG, "batteryType: %d", (unsigned int)keypad2Config.batteryType);
    ESP_LOGD(TAG, "buttonMode: %d", (unsigned int)keypad2Config.buttonMode);
    ESP_LOGD(TAG, "lockAction: %d", (unsigned int)keypad2Config.lockAction);
  }
}

void log_accessory_info(AccessoryInfo accessoryInfo, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "dateYear: %d", (unsigned int)accessoryInfo.dateYear);
    ESP_LOGD(TAG, "dateMonth: %d", (unsigned int)accessoryInfo.dateMonth);
    ESP_LOGD(TAG, "dateDay: %d", (unsigned int)accessoryInfo.dateDay);
    ESP_LOGD(TAG, "dateHour: %d", (unsigned int)accessoryInfo.dateHour);
    ESP_LOGD(TAG, "dateMinute: %d", (unsigned int)accessoryInfo.dateMinute);
    ESP_LOGD(TAG, "dateSecond: %d", (unsigned int)accessoryInfo.dateSecond);
    ESP_LOGD(TAG, "accessoryNukiId: %d", (unsigned int)accessoryInfo.accessoryNukiId);
    ESP_LOGD(TAG, "accessoryType: %d", (unsigned int)accessoryInfo.accessoryType);
    ESP_LOGD(TAG, "firmwareVersion: %d.%d.%d", accessoryInfo.firmwareVersion[0], accessoryInfo.firmwareVersion[1], accessoryInfo.firmwareVersion[2]);
    ESP_LOGD(TAG, "hardwareRevision: %d.%d", accessoryInfo.hardwareRevision[0], accessoryInfo.hardwareRevision[1]);
    ESP_LOGD(TAG, "productVariantDifferentiator: %d", (unsigned int)accessoryInfo.productVariantDifferentiator);
    ESP_LOGD(TAG, "mostRecentBatteryVoltage: %d", (unsigned int)accessoryInfo.mostRecentBatteryVoltage);
    ESP_LOGD(TAG, "mostRecentTemperature: %d", (unsigned int)accessoryInfo.mostRecentTemperature);
  }
}

// Performs one step per call instead of blocking until every keypad code has arrived;
// safe to call repeatedly (e.g. once per loop() tick) until it stops returning
// CmdResult::Working, exactly like execute_action().
CmdResult NukiBle::retrieve_keypad_entries(const uint16_t offset, const uint16_t count) {
  // Refresh unconditionally, every call - otherwise the watchdog timeout (refreshed inside
  // execute_action() only while sending the initial request) never gets renewed while
  // waiting for the count/codes notifications afterward, and can disconnect mid-wait even
  // though the lock is still legitimately about to deliver them.
  this->extend_disconnect_timeout();

  if (this->keypad_retrieval_state_ == KeypadRetrievalState::Idle) {
    NukiAction action;
    unsigned char payload[4] = {0};
    memcpy(payload, &offset, 2);
    memcpy(&payload[2], &count, 2);

    action.cmdType = CommandType::CommandWithChallengeAndPin;
    action.command = Command::RequestKeypadCodes;
    memcpy(action.payload, &payload, sizeof(payload));
    action.payloadLen = sizeof(payload);

    if (!this->keypad_request_sent_) {
      this->nr_of_received_keypad_codes_ = 0;
      this->keypad_code_count_received_ = false;
      this->keypad_request_sent_ = true;
    }

    CmdResult result = this->execute_action(action);
    if (result == CmdResult::Working) {
      return CmdResult::Working;
    }
    this->keypad_request_sent_ = false;
    if (result != CmdResult::Success) {
      ESP_LOGW(TAG, "Retrieve keypad codes from lock failed");
      return result;
    }

    this->keypad_retrieval_state_ = KeypadRetrievalState::AwaitingCount;
    this->keypad_retrieval_time_now_ = (esp_timer_get_time() / 1000);
  }

  if (this->keypad_retrieval_state_ == KeypadRetrievalState::AwaitingCount) {
    //wait for return of Keypad Code Count (0x0044)
    if ((esp_timer_get_time() / 1000) - this->keypad_retrieval_time_now_ > this->general_timeout_duration_) {
      ESP_LOGW(TAG, "Receive keypad count timeout");
      this->keypad_retrieval_state_ = KeypadRetrievalState::Idle;
      this->disconnect();
      return CmdResult::TimeOut;
    }
    if (!this->keypad_code_count_received_) {
      return CmdResult::Working;
    }
    if (this->debug_nuki_command_) {
      ESP_LOGD(TAG, "Keypad code count %d", this->get_keypad_entry_count());
    }
    // Count is known now (just arrived in the notification handled above), so size the
    // buffer for it in one allocation instead of growing node-by-node as entries arrive.
    this->list_of_key_pad_entries_.init(this->get_keypad_entry_count());
    this->keypad_retrieval_state_ = KeypadRetrievalState::AwaitingCodes;
    this->keypad_retrieval_time_now_ = (esp_timer_get_time() / 1000);
  }

  //wait for return of Keypad Codes (0x0045)
  if ((esp_timer_get_time() / 1000) - this->keypad_retrieval_time_now_ > this->general_timeout_duration_) {
    ESP_LOGW(TAG, "Receive keypadcodes timeout");
    this->keypad_retrieval_state_ = KeypadRetrievalState::Idle;
    this->disconnect();
    return CmdResult::TimeOut;
  }
  if (this->nr_of_received_keypad_codes_ < this->get_keypad_entry_count()) {
    return CmdResult::Working;
  }

  if (this->debug_nuki_command_) {
    ESP_LOGD(TAG, "%d codes received", this->nr_of_received_keypad_codes_);
  }
  this->keypad_retrieval_state_ = KeypadRetrievalState::Idle;
  return CmdResult::Success;
}

CmdResult NukiBle::add_keypad_entry(NewKeypadEntry newKeypadEntry) {
  //TODO verify data validity, ie check for invalid chars in name
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::AddKeypadCode;
  memcpy(action.payload, &newKeypadEntry, sizeof(NewKeypadEntry));
  action.payloadLen = sizeof(NewKeypadEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "addKeyPadEntry, payloadlen: %d", sizeof(NewKeypadEntry));
      print_buffer(action.payload, sizeof(NewKeypadEntry), false, "addKeyPadCode content: ", this->debug_nuki_hex_data_);
      log_new_keypad_entry(newKeypadEntry, this->debug_nuki_readable_data_);
    }
  }
  return result;
}

CmdResult NukiBle::update_keypad_entry(UpdatedKeypadEntry updatedKeyPadEntry) {
  //TODO verify data validity
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateKeypadCode;
  memcpy(action.payload, &updatedKeyPadEntry, sizeof(UpdatedKeypadEntry));
  action.payloadLen = sizeof(UpdatedKeypadEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "addKeyPadEntry, payloadlen: %d", sizeof(UpdatedKeypadEntry));
      print_buffer(action.payload, sizeof(UpdatedKeypadEntry), false, "updatedKeypad content: ", this->debug_nuki_hex_data_);
      log_updated_keypad_entry(updatedKeyPadEntry, this->debug_nuki_readable_data_);
    }
  }
  return result;
}

void NukiBle::get_fingerprint_entries(std::vector<FingerprintEntry>* requestedFingerprintEntries) {
  requestedFingerprintEntries->clear();
  for (const auto &entry : this->list_of_fingerprint_entries_) {
    requestedFingerprintEntries->push_back(entry);
  }
}

void NukiBle::get_keypad_entries(std::vector<KeypadEntry>* requestedKeypadCodes) {
  requestedKeypadCodes->clear();
  for (const auto &entry : this->list_of_key_pad_entries_) {
    requestedKeypadCodes->push_back(entry);
  }
}

uint16_t NukiBle::get_keypad_entry_count() {
  return this->nr_of_keypad_codes_;
}

CmdResult NukiBle::delete_keypad_entry(uint16_t id) {
  NukiAction action;
  unsigned char payload[2] = {0};
  memcpy(payload, &id, 2);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveKeypadCode;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return this->execute_action(action);
}

CmdResult NukiBle::retrieve_fingerprint_entries() {
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestFingerprintEntries;

  this->list_of_fingerprint_entries_.clear();

  return this->execute_action(action);
}

CmdResult NukiLock::get_accessory_info(const uint8_t accessoryType) {
  NukiAction action;
  unsigned char payload[1] = {0};
  memcpy(payload, &accessoryType, 1);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestAccessoryInfo;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);
  return this->execute_action(action);
}

bool NukiLock::is_keypad_battery_critical() {
  if(this->key_turner_state_.accessoryBatteryState != 255) {
    if ((this->key_turner_state_.accessoryBatteryState & 1) == 1) {
      return ((this->key_turner_state_.accessoryBatteryState & 3) == 3);
    }
  }
  return false;
}

bool NukiLockComponent::valid_keypad_id(int32_t id) {
    bool is_valid = std::find(keypad_code_ids_.begin(), keypad_code_ids_.end(), id) != keypad_code_ids_.end();
    if (!is_valid) {
        ESP_LOGE(TAG, "Keypad id %d unknown.", id);
    }
    return is_valid;
}

bool NukiLockComponent::valid_keypad_name(std::string name) {
    bool name_valid = !(name == "" || name == "--");
    if (!name_valid) {
        ESP_LOGE(TAG, "Keypad name '%s' is invalid.", name.c_str());
    }
    return name_valid;
}

bool NukiLockComponent::valid_keypad_code(int32_t code) {
    bool code_valid = (code > 100000 && code < 1000000 && (std::to_string(code).find('0') == std::string::npos));
    if (!code_valid) {
        ESP_LOGE(TAG, "Keypad code %d is invalid. Code must be 6 digits, without 0.", code);
    }
    return code_valid;
}

void NukiLockComponent::add_keypad_entry(std::string name, int32_t code) {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot add keypad entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    if (!(valid_keypad_name(name) && valid_keypad_code(code))) {
        ESP_LOGE(TAG, "add_keypad_entry invalid parameters");
        return;
    }

    NewKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    size_t name_len = name.length();
    memcpy(&entry.name, name.c_str(), name_len > 20 ? 20 : name_len);
    entry.code = code;

    this->queue_nuki_command([this, entry] { return this->nuki_lock_.add_keypad_entry(entry); }, [](CmdResult result) {
        if (result == CmdResult::Success) {
            ESP_LOGI(TAG, "add_keypad_entry is sucessful");
        } else {
            ESP_LOGE(TAG, "add_keypad_entry: add_keypad_entry failed (result %d)", result);
        }
    });
}

void NukiLockComponent::update_keypad_entry(int32_t id, std::string name, int32_t code, bool enabled) {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot update keypad entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    if (!(valid_keypad_id(id) && valid_keypad_name(name) && valid_keypad_code(code))) {
        ESP_LOGE(TAG, "update_keypad_entry invalid parameters");
        return;
    }

    UpdatedKeypadEntry entry;
    memset(&entry, 0, sizeof(entry));
    entry.codeId = id;
    size_t name_len = name.length();
    memcpy(&entry.name, name.c_str(), name_len > 20 ? 20 : name_len);
    entry.code = code;
    entry.enabled = enabled ? 1 : 0;

    this->queue_nuki_command([this, entry] { return this->nuki_lock_.update_keypad_entry(entry); }, [](CmdResult result) {
        if (result == CmdResult::Success) {
            ESP_LOGI(TAG, "update_keypad_entry is sucessful");
        } else {
            ESP_LOGE(TAG, "update_keypad_entry: update_keypad_entry failed (result %d)", result);
        }
    });
}

void NukiLockComponent::delete_keypad_entry(int32_t id) {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot retrieve delete entry");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    if (!valid_keypad_id(id)) {
        ESP_LOGE(TAG, "delete_keypad_entry invalid parameters");
        return;
    }

    this->queue_nuki_command([this, id] { return this->nuki_lock_.delete_keypad_entry(id); }, [](CmdResult result) {
        if (result == CmdResult::Success) {
            ESP_LOGI(TAG, "delete_keypad_entry is sucessful");
        } else {
            ESP_LOGE(TAG, "delete_keypad_entry: delete_keypad_entry failed (result %d)", result);
        }
    });
}

void NukiLockComponent::print_keypad_entries() {
    if (!this->nuki_lock_.is_paired_with_lock()) {
        ESP_LOGE(TAG, "Lock is not paired, cannot retrieve keypad entries");
        return;
    }

    if (!keypad_paired_) {
        ESP_LOGE(TAG, "Keypad is not paired to Nuki");
        return;
    }

    if(this->pin_state_ != PinState::Valid) {
        ESP_LOGW(TAG, "It seems like you did not set a valid pin!");
        return;
    }

    this->queue_nuki_command([this] { return this->nuki_lock_.retrieve_keypad_entries(0, 0xffff); }, [this](CmdResult result) {
        if (result == CmdResult::Success) {
            ESP_LOGI(TAG, "retrieve_keypad_entries sucess");
            std::vector<KeypadEntry> entries;
            this->nuki_lock_.get_keypad_entries(&entries);

            std::sort(entries.begin(), entries.end(), [](const KeypadEntry& a, const KeypadEntry& b) { return a.codeId < b.codeId; });

            keypad_code_ids_.clear();
            keypad_code_ids_.reserve(entries.size());
            for (const auto& entry : entries) {
                keypad_code_ids_.push_back(entry.codeId);
                ESP_LOGI(TAG, "keypad #%d %s is %s", entry.codeId, entry.name, entry.enabled ? "enabled" : "disabled");
            }
        } else {
            ESP_LOGE(TAG, "print_keypad_entries: retrieve_keypad_entries failed (result %d)", result);
        }
    });
}

}  // namespace esphome::nuki_lock
