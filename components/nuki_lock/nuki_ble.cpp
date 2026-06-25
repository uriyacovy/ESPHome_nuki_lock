/**
 * @file nuki_ble.cpp
 *
 * Created: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include "nuki_ble.h"
#include "nuki_lock_protocol_utils.h"
#include "nuki_ble_utils.h"

#include <sodium.h>

#include "esphome/core/log.h"
#include "esp_timer.h"

#include <algorithm>
#include <atomic>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome::nuki_lock {

static const char *const TAG = "NukiBle";

NukiBle::NukiBle(const std::string& device_name,
                 const uint32_t device_id,
                 const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_uuid,
                 const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_ultra_uuid,
                 const esphome::esp32_ble_tracker::ESPBTUUID device_service_uuid,
                 const esphome::esp32_ble_tracker::ESPBTUUID gdio_uuid,
                 const esphome::esp32_ble_tracker::ESPBTUUID gdio_ultra_uuid,
                 const esphome::esp32_ble_tracker::ESPBTUUID user_data_uuid,
                 const std::string preferenced_id)
  : device_name_(device_name),
    device_id_(device_id),
    pairing_service_uuid_(pairing_service_uuid),
    pairing_service_ultra_uuid_(pairing_service_ultra_uuid),
    device_service_uuid_(device_service_uuid),
    gdio_uuid_(gdio_uuid),
    gdio_ultra_uuid_(gdio_ultra_uuid),
    user_data_uuid_(user_data_uuid),
    preferences_id_(preferenced_id)
{

  this->rssi_ = 0;
  this->last_received_beacon_ts_ = 0;
  this->last_heartbeat_ = 0;

  #ifdef DEBUG_NUKI_CONNECT
  this->debug_nuki_connect_ = true;
  #endif
  #ifdef DEBUG_NUKI_COMMUNICATION
  this->debug_nuki_communication_ = true;
  #endif
  #ifdef DEBUG_NUKI_READABLE_DATA
  this->debug_nuki_readable_data_ = true;
  #endif
  #ifdef DEBUG_NUKI_HEX_DATA
  this->debug_nuki_hex_data_ = true;
  #endif
  #ifdef DEBUG_NUKI_COMMAND
  this->debug_nuki_command_ = true;
  #endif
}

NukiBle::~NukiBle() {
}

void NukiBle::initialize() {
  this->ble_address_pref_ = make_credential_pref<uint8_t[6]>(BLE_ADDRESS_STORE_NAME);
  this->secret_key_pref_ = make_credential_pref<uint8_t[32]>(SECRET_KEY_STORE_NAME);
  this->auth_id_pref_ = make_credential_pref<uint8_t[4]>(AUTH_ID_STORE_NAME);
  this->security_pincode_pref_ = make_credential_pref<uint16_t>(SECURITY_PINCODE_STORE_NAME);
  this->ultra_pincode_pref_ = make_credential_pref<uint32_t>(ULTRA_PINCODE_STORE_NAME);
  this->is_ultra_pref_ = make_credential_pref<bool>(ULTRA_STORE_NAME);

  // No NimBLEDevice::init()/createClient() here anymore: the GATT client (this object,
  // via BLEClientBase) is registered with ESPHome's BLE tracker through Python codegen
  // (see lock.py once the wiring block of this migration lands), which also takes care of
  // bringing up the underlying BLE stack.
  this->is_paired_ = this->retrieve_credentials();
}

// Performs one pairing step per call so it can be driven by repeated calls (e.g. once
// per loop() tick) instead of blocking until pairing finishes or times out. Progress is
// kept in pairing_in_progress/pairing_keypair_generated/pairing_state across calls.
PairingResult NukiBle::pair_nuki(AuthorizationIdType idType) {
  if (!this->pairing_in_progress_) {
    this->authorization_id_type_ = idType;

    if (this->retrieve_credentials()) {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Already paired");
      }
      this->is_paired_ = true;
      return PairingResult::Success;
    }

    if (this->pairing_last_seen_ < (esp_timer_get_time() / 1000) - 2000) this->pairing_service_available_ = false;

    if (!(this->pairing_service_available_ && this->get_address() != 0)) {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "No nuki in pairing mode found");
      }
      return PairingResult::Pairing;
    }

    this->pairing_service_available_ = false;
    if (this->debug_nuki_connect_) {
      ESP_LOGD(TAG, "Nuki in pairing mode found");
    }

    this->pairing_in_progress_ = true;
    this->pairing_keypair_generated_ = false;
    this->pairing_state_ = PairingState::InitPairing;
  }

  // Refresh before the connect_ble() gate too - otherwise the watchdog timeout (last
  // refreshed once at ESP_GATTC_OPEN_EVT) never gets renewed while waiting for service
  // discovery/characteristic registration to finish, and can fire mid-handshake even
  // though the connection is still making legitimate progress.
  this->extend_disconnect_timeout();

  if (!this->connect_ble(true)) {
    // Connection attempt still in progress; retry next call.
    return PairingResult::Pairing;
  }

  if (!this->pairing_keypair_generated_) {
    crypto_box_keypair(this->my_public_key_, this->my_private_key_);
    this->pairing_keypair_generated_ = true;
  }

  this->pairing_state_ = this->pair_state_machine(this->pairing_state_);
  this->extend_disconnect_timeout();

  if (this->pairing_state_ != PairingState::Success && this->pairing_state_ != PairingState::Timeout) {
    return PairingResult::Pairing;
  }

  PairingResult result;
  if (this->pairing_state_ == PairingState::Success) {
    this->save_credentials();
    result = PairingResult::Success;
    this->last_heartbeat_ = (esp_timer_get_time() / 1000);
  } else {
    result = PairingResult::Timeout;
  }
  this->extend_disconnect_timeout();
  this->pairing_in_progress_ = false;

  if (this->debug_nuki_connect_) {
    ESP_LOGD(TAG, "pairing result %d", (unsigned int)result);
  }

  this->is_paired_ = (result == PairingResult::Success);
  return result;
}

void NukiBle::unpair_nuki() {
  this->delete_credentials();
  this->is_paired_ = false;
  if (this->debug_nuki_connect_) {
    ESP_LOGD(TAG, "[%s] Credentials deleted", this->device_name_.c_str());
  }
}

// Non-blocking: if not connected (or connected for the other mode - see pairing_mode in
// nuki_ble.h), kicks off a connection attempt (via the DISCOVERED state - see the comment
// at the set_state() call below) and returns false immediately; callers are expected to
// call this again on a later tick until it returns true. Once connected,
// gattc_event_handler()'s ESP_GATTC_SEARCH_CMPL_EVT/ESP_GATTC_REG_FOR_NOTIFY_EVT handling
// discovers the relevant characteristic and subscribes to it; connect_ble() only reports
// "ready to send" (returns true) once that subscription is confirmed active
// (notify_registered), since sending before then would mean the lock's response never
// reaches us.
bool NukiBle::connect_ble(bool pairing) {
  if (this->connected() && this->notify_registered_) {
    if (this->pairing_mode_ == pairing) {
      return true;
    }
    // Switching mode while connected - shouldn't normally happen (pairing-mode traffic and
    // normal-operation traffic are never interleaved within one connection), but reconnect
    // for the new mode just in case.
    this->disconnect();
    return false;
  }

  if (this->state() == esphome::esp32_ble_tracker::ClientState::IDLE) {
    this->pairing_mode_ = pairing;
    this->notify_registered_ = false;
    this->not_connected_logged_ = false;
    // Don't call connect() directly - that bypasses the tracker's
    // try_promote_discovered_clients_(), whose job is to stop scanning before connecting
    // (a single BLE radio can't do both at once; without this, a continuously-scanning
    // radio starves the connection attempt of airtime). Going through DISCOVERED instead
    // - the same state BLEClientBase's own default parse_device() sets - lets the tracker
    // pause/resume scanning around the connection like it does for every other BLE client.
    this->set_state(esphome::esp32_ble_tracker::ClientState::DISCOVERED);
  }
  return false;
}

void NukiBle::update_connection_state() {
  if (this->state() == esphome::esp32_ble_tracker::ClientState::CONNECTING || this->state() == esphome::esp32_ble_tracker::ClientState::DISCONNECTING) {
    return;
  }

  if (this->last_start_timeout_ != 0 && ((esp_timer_get_time() / 1000) - this->last_start_timeout_ > this->timeout_duration_)) {
    if (this->state() != esphome::esp32_ble_tracker::ClientState::IDLE) {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "disconnecting BLE on timeout");
      }

      this->disconnect();
    }

    this->last_start_timeout_ = 0;
  }
}

// Called by BLEClientBase once a connection has been fully torn down (after
// release_services(), which invalidates gdio_char/usdio_char), whether that happened via a
// normal ESP_GATTC_CLOSE_EVT or (reason == ESP_GATT_CONN_TIMEOUT) because disconnect() was
// requested but no CLOSE_EVT ever arrived within BLEClientBase's safety timeout - the
// latter replaces the original NimBLE-era "didn't disconnect within ~5s" detection.
void NukiBle::on_disconnect_complete(esp_err_t reason) {
  this->gdio_char_ = nullptr;
  this->usdio_char_ = nullptr;
  this->notify_registered_ = false;

  if (reason == ESP_GATT_CONN_TIMEOUT) {
    if (this->debug_nuki_connect_) {
      ESP_LOGD(TAG, "Error while disconnecting BLE client");
    }
    if (this->event_handler_) {
      this->event_handler_->notify(EventType::BLE_ERROR_ON_DISCONNECT);
    }
  }
}

void NukiBle::set_disconnect_timeout(uint32_t timeoutMs) {
  this->timeout_duration_ = timeoutMs;
}

void NukiBle::set_connect_timeout(uint8_t timeout) {
  this->connect_timeout_sec_ = timeout;
}

void NukiBle::set_general_timeout(uint32_t timeoutMs) {
  this->general_timeout_duration_ = timeoutMs;
}

void NukiBle::set_command_timeout(uint32_t timeoutMs) {
  this->command_timeout_duration_ = timeoutMs;
}

void NukiBle::set_connect_retries(uint8_t retries) {
  this->connect_retries_ = retries;
}

void NukiBle::extend_disconnect_timeout() {
  this->last_start_timeout_ = (esp_timer_get_time() / 1000);
  this->last_heartbeat_ = (esp_timer_get_time() / 1000);
}

bool NukiBle::parse_device(const esphome::esp32_ble_tracker::ESPBTDevice &device) {
  if (this->is_paired_) {
    if (this->get_address() != device.address_uint64()) {
      return false;
    }

    this->rssi_ = device.get_rssi();
    this->last_received_beacon_ts_ = (esp_timer_get_time() / 1000);

    // Nuki's normal-operation advertisement is a plain iBeacon broadcast (manufacturer
    // data only) - it does not also carry a Service UUID List AD type, so gating this on
    // device.get_service_uuids() containing device_service_uuid (as a prior version of this
    // code did) meant this branch could never fire: that UUID list is never present outside
    // of an active GATT connection. The address match above is already device-specific
    // enough on its own.
    if (this->debug_nuki_connect_) {
      ESP_LOGD(TAG, "Nuki Advertising: %s", device.address_str().c_str());
    }

    // Nuki repurposes the iBeacon "measured power" byte as a status-changed bit (bit 0),
    // not as an actual signal power reading.
    auto ibeacon = device.get_ibeacon();
    if (ibeacon.has_value()) {
      if (this->debug_nuki_connect_) {
        char uuid_buf[esphome::esp32_ble_tracker::UUID_STR_LEN];
        ESP_LOGD(TAG, "iBeacon Major: %d Minor: %d UUID: %s Power: %d", ibeacon.value().get_major(),
              ibeacon.value().get_minor(), ibeacon.value().get_uuid().to_str(uuid_buf), ibeacon.value().get_signal_power());
      }

      this->last_heartbeat_ = (esp_timer_get_time() / 1000);

      if ((ibeacon.value().get_signal_power() & 0x01) > 0) {
        if (this->event_handler_) {
          this->event_handler_->notify(EventType::KeyTurnerStatusUpdated);
        }

        this->status_updated_ = true;
      }
      else if (this->status_updated_)
      {
        this->status_updated_ = false;

        if (this->event_handler_) {
          this->event_handler_->notify(EventType::KeyTurnerStatusReset);
        }
      }
    }

    return ibeacon.has_value();
  } else {
    if (!this->pairing_mode_active_) {
      return false;
    }

    for (const auto &service_data : device.get_service_datas()) {
      if (service_data.uuid == this->pairing_service_uuid_) {
        if (this->debug_nuki_connect_) {
          ESP_LOGD(TAG, "Found nuki in pairing state: %s addr: %s", device.get_name().c_str(), device.address_str().c_str());
        }
        this->set_address(device.address_uint64());
        this->set_remote_addr_type(device.get_address_type());
        this->pairing_service_available_ = true;
        this->smart_lock_ultra_ = false;
        this->pairing_last_seen_ = (esp_timer_get_time() / 1000);
        return true;
      } else if (service_data.uuid == this->pairing_service_ultra_uuid_) {
        if (this->debug_nuki_connect_) {
          ESP_LOGD(TAG, "Found nuki ultra in pairing state: %s addr: %s", device.get_name().c_str(), device.address_str().c_str());
        }

        if (this->ultra_pin_code_ == 000000) {
          ESP_LOGD(TAG, "No pairing PIN code set, not pairing with Nuki SmartLock Ultra");
        } else {
          this->set_address(device.address_uint64());
          this->set_remote_addr_type(device.get_address_type());
          this->pairing_service_available_ = true;
          this->smart_lock_ultra_ = true;
          this->pairing_last_seen_ = (esp_timer_get_time() / 1000);
        }
        return true;
      }
    }
    return false;
  }
}

CmdResult NukiBle::generic_command(Command command, bool withPin) {
  NukiAction action;

  if (withPin) {
    action.cmdType = CommandType::CommandWithChallengeAndPin;
  } else {
    action.cmdType = CommandType::CommandWithChallenge;
  }
  action.command = command;

  CmdResult result = this->execute_action(action);
  return result;
}

CmdResult NukiBle::request_daily_statistics() {
  NukiAction action;
  unsigned char payload[5] = {0};
  payload[0] = 0;
  payload[1] = 0;
  payload[2] = 0;
  payload[3] = 0;
  payload[4] = 5;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestDailyStatistics;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  return result;
}

// Performs one step per call instead of blocking until every authorization entry has
// arrived; safe to call repeatedly (e.g. once per loop() tick) until it stops returning
// CmdResult::Working, exactly like retrieve_keypad_entries().
CmdResult NukiBle::retrieve_authorization_entries(const uint16_t offset, const uint16_t count) {
  // Refresh unconditionally, every call - see retrieve_keypad_entries() for why.
  this->extend_disconnect_timeout();

  if (this->auth_entries_retrieval_state_ == EntryRetrievalState::Idle) {
    NukiAction action;
    unsigned char payload[4] = {0};
    memcpy(payload, &offset, 2);
    memcpy(&payload[2], &count, 2);

    action.cmdType = CommandType::CommandWithChallengeAndPin;
    action.command = Command::RequestAuthorizationEntries;
    memcpy(action.payload, &payload, sizeof(payload));
    action.payloadLen = sizeof(payload);

    if (!this->auth_entries_request_sent_) {
      this->authorization_entry_count_received_ = false;
      this->expected_authorization_entry_count_ = 0;
      this->auth_entries_request_sent_ = true;
    }

    CmdResult result = this->execute_action(action);
    if (result == CmdResult::Working) {
      return CmdResult::Working;
    }
    this->auth_entries_request_sent_ = false;
    if (result != CmdResult::Success) {
      ESP_LOGW(TAG, "Retrieve authorization entries from lock failed");
      return result;
    }

    this->auth_entries_retrieval_state_ = EntryRetrievalState::AwaitingCount;
    this->auth_entries_retrieval_time_now_ = (esp_timer_get_time() / 1000);
  }

  if (this->auth_entries_retrieval_state_ == EntryRetrievalState::AwaitingCount) {
    //wait for return of Authorization Entry Count (0x0027)
    if ((esp_timer_get_time() / 1000) - this->auth_entries_retrieval_time_now_ > this->general_timeout_duration_) {
      ESP_LOGW(TAG, "Receive authorization entry count timeout");
      this->auth_entries_retrieval_state_ = EntryRetrievalState::Idle;
      this->disconnect();
      return CmdResult::TimeOut;
    }
    if (!this->authorization_entry_count_received_) {
      return CmdResult::Working;
    }
    // Count is known now (just arrived in the notification), so size the buffer for it in
    // one allocation instead of growing node-by-node as entries arrive.
    this->list_of_authorization_entries_.init(this->expected_authorization_entry_count_);
    this->auth_entries_retrieval_state_ = EntryRetrievalState::AwaitingEntries;
    this->auth_entries_retrieval_time_now_ = (esp_timer_get_time() / 1000);
  }

  //wait for return of Authorization Entries (0x000A)
  if ((esp_timer_get_time() / 1000) - this->auth_entries_retrieval_time_now_ > this->general_timeout_duration_) {
    ESP_LOGW(TAG, "Receive authorization entries timeout");
    this->auth_entries_retrieval_state_ = EntryRetrievalState::Idle;
    this->disconnect();
    return CmdResult::TimeOut;
  }
  if (this->list_of_authorization_entries_.size() < this->expected_authorization_entry_count_) {
    return CmdResult::Working;
  }

  this->auth_entries_retrieval_state_ = EntryRetrievalState::Idle;
  return CmdResult::Success;
}

void NukiBle::get_authorization_entries(std::vector<AuthorizationEntry>* requestedAuthorizationEntries) {
  requestedAuthorizationEntries->clear();
  for (const auto &entry : this->list_of_authorization_entries_) {
    requestedAuthorizationEntries->push_back(entry);
  }
}

CmdResult NukiBle::add_authorization_entry(NewAuthorizationEntry newAuthorizationEntry) {
  //TODO verify data validity
  NukiAction action;
  unsigned char payload[sizeof(NewAuthorizationEntry)] = {0};
  memcpy(payload, &newAuthorizationEntry, sizeof(NewAuthorizationEntry));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::AuthorizationDatInvite;
  memcpy(action.payload, &payload, sizeof(NewAuthorizationEntry));
  action.payloadLen = sizeof(NewAuthorizationEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "add_authorization_entry, payloadlen: %d", sizeof(NewAuthorizationEntry));
      print_buffer(action.payload, sizeof(NewAuthorizationEntry), false, "add_authorization_entry content: ", this->debug_nuki_hex_data_);
      log_new_authorization_entry(newAuthorizationEntry, this->debug_nuki_readable_data_);
    }
  }
  return result;
}

CmdResult NukiBle::delete_authorization_entry(uint32_t id) {
  NukiAction action;
  unsigned char payload[4] = {0};
  memcpy(payload, &id, 4);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveUserAuthorization;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return this->execute_action(action);
}

CmdResult NukiBle::update_authorization_entry(UpdatedAuthorizationEntry updatedAuthorizationEntry) {
  //TODO verify data validity
  NukiAction action;
  unsigned char payload[sizeof(UpdatedAuthorizationEntry)] = {0};
  memcpy(payload, &updatedAuthorizationEntry, sizeof(UpdatedAuthorizationEntry));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateAuthorization;
  memcpy(action.payload, &payload, sizeof(UpdatedAuthorizationEntry));
  action.payloadLen = sizeof(UpdatedAuthorizationEntry);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "add_authorization_entry, payloadlen: %d", sizeof(UpdatedAuthorizationEntry));
      print_buffer(action.payload, sizeof(UpdatedAuthorizationEntry), false, "updatedKeypad content: ", this->debug_nuki_hex_data_);
      log_updated_authorization_entry(updatedAuthorizationEntry, this->debug_nuki_readable_data_);
    }
  }
  return result;
}

uint16_t NukiBle::get_log_entry_count() {
  return this->log_entry_count_;
}

bool NukiBle::get_logging_enabled() {
  return this->logging_enabled_;
}

CmdResult NukiBle::enable_logging(const bool enable) {
  NukiAction action;
  unsigned char payload[1] = {(unsigned char)enable};

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::EnableLogging;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return this->execute_action(action);
}

CmdResult NukiBle::request_most_recent_command(uint16_t* retrievedCommand) {
  NukiAction action{};
  uint16_t payload = (uint16_t)Command::MostRecentCommand;

  action.cmdType = CommandType::Command;
  action.command = Command::RequestData;
  memcpy(&action.payload[0], &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    *retrievedCommand = this->most_recent_command_;
  }
  return result;
}

CmdResult NukiBle::set_security_pin(const uint16_t newSecurityPin) {
  NukiAction action;
  unsigned char payload[2] = {0};
  memcpy(payload, &newSecurityPin, 2);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetSecurityPin;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    this->pin_code_ = newSecurityPin;
    this->save_credentials();
  }
  return result;
}

CmdResult NukiBle::set_ultra_pin(const uint32_t newSecurityPin) {
  NukiAction action;
  unsigned char payload[4] = {0};
  memcpy(payload, &newSecurityPin, 4);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetSecurityPin;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    this->ultra_pin_code_ = newSecurityPin;
    this->save_credentials();
  }
  return result;
}

CmdResult NukiBle::verify_security_pin() {
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::VerifySecurityPin;
  action.payloadLen = 0;

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "Verify security pin code success");
    }
  }
  return result;
}

CmdResult NukiBle::request_calibration() {
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestCalibration;
  action.payloadLen = 0;

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "Calibration executed");
    }
  }
  return result;
}

CmdResult NukiBle::request_reboot() {
  NukiAction action;

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestReboot;
  action.payloadLen = 0;

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "Reboot executed");
    }
  }
  return result;
}

CmdResult NukiBle::update_time(TimeValue time) {
  NukiAction action;
  unsigned char payload[sizeof(TimeValue)] = {0};
  memcpy(payload, &time, sizeof(TimeValue));

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateTime;
  memcpy(action.payload, &payload, sizeof(TimeValue));
  action.payloadLen = sizeof(TimeValue);

  CmdResult result = this->execute_action(action);
  if (result == CmdResult::Success) {
    if (this->debug_nuki_readable_data_) {
      ESP_LOGD(TAG, "Time set: %d-%d-%d %d:%d:%d", time.year, time.month, time.day, time.hour, time.minute, time.second);
    }
  }
  return result;
}

bool NukiBle::save_security_pincode(const uint16_t pin_code) {
  if (!this->security_pincode_pref_.save(&pin_code) || !esphome::global_preferences->sync()) {
    ESP_LOGE(TAG, "ERROR: save_security_pincode failed");
    return false;
  }

  this->pin_code_ = pin_code;
  return true;
}

bool NukiBle::save_ultra_pincode(const uint32_t pin_code, bool save) {
  if (save) {
    if (!this->ultra_pincode_pref_.save(&pin_code) || !esphome::global_preferences->sync()) {
      ESP_LOGE(TAG, "ERROR: save_ultra_pincode failed");
      return false;
    }
  }

  this->ultra_pin_code_ = pin_code;
  return true;
}

void NukiBle::save_credentials() {
  unsigned char currentBleAddress[6];
  unsigned char storedBleAddress[6];
  uint16_t defaultPincode = 0;
  memcpy(currentBleAddress, this->get_remote_bda(), 6);

  this->ble_address_pref_.load(&storedBleAddress);

  bool isUltraLock = this->is_lock_ultra();
  this->is_ultra_pref_.save(&isUltraLock);

  if (this->is_lock_ultra()) {
    this->ultra_pincode_pref_.save(&this->ultra_pin_code_);
  } else {
    if (compare_char_array(currentBleAddress, storedBleAddress, 6)) {
      //only store earlier retreived pin code if address is the same
      //otherwise it is a different/new lock
      this->security_pincode_pref_.save(&this->pin_code_);
    } else {
      this->security_pincode_pref_.save(&defaultPincode);
    }
  }

  if (this->ble_address_pref_.save(&currentBleAddress)
      && this->secret_key_pref_.save(&this->secret_key_k_)
      && this->auth_id_pref_.save(&this->authorization_id_)
      && esphome::global_preferences->sync()
    ) {
    if (this->debug_nuki_connect_) {
      ESP_LOGD(TAG, "Credentials saved:");
      print_buffer(this->secret_key_k_, sizeof(this->secret_key_k_), false, SECRET_KEY_STORE_NAME, this->debug_nuki_hex_data_);
      print_buffer(currentBleAddress, 6, false, BLE_ADDRESS_STORE_NAME, this->debug_nuki_hex_data_);
      print_buffer(this->authorization_id_, sizeof(this->authorization_id_), false, AUTH_ID_STORE_NAME, this->debug_nuki_hex_data_);

      if (this->is_lock_ultra()) {
        ESP_LOGD(TAG, "pincode: %d", (unsigned int)this->ultra_pin_code_);
      } else {
        ESP_LOGD(TAG, "pincode: %d", this->pin_code_);
      }
    }
  } else {
    ESP_LOGE(TAG, "Error saving credentials");
  }
}

uint16_t NukiBle::get_security_pincode() {
  uint16_t storedPincode = 0000;
  if (this->security_pincode_pref_.load(&storedPincode)) {
    return storedPincode;
  }
  return 0;
}

uint32_t NukiBle::get_ultra_pincode() {
  uint32_t storedPincode = 000000;
  if (this->ultra_pincode_pref_.load(&storedPincode)) {
    return storedPincode;
  }
  return 0;
}

void NukiBle::get_mac_address(char* macAddress) {
  unsigned char buf[6];
  if (this->ble_address_pref_.load(&buf)) {
    esphome::format_mac_addr_upper(buf, macAddress);
  }
}

bool NukiBle::retrieve_credentials() {
  //TODO check on empty (invalid) credentials?
  unsigned char buff[6];

  if (this->ble_address_pref_.load(&buff)
    && this->secret_key_pref_.load(&this->secret_key_k_)
    && this->auth_id_pref_.load(&this->authorization_id_)
   ) {
    uint64_t address = 0;
    for (int i = 0; i < 6; i++) {
      address = (address << 8) | buff[i];
    }
    this->set_address(address);

    if (this->debug_nuki_connect_) {
      ESP_LOGI(TAG, "[%s] Credentials retrieved:", this->device_name_.c_str());
      print_buffer(this->secret_key_k_, sizeof(this->secret_key_k_), false, SECRET_KEY_STORE_NAME, this->debug_nuki_hex_data_);
      ESP_LOGD(TAG, "bleAddress: %s", this->address_str());
      print_buffer(this->authorization_id_, sizeof(this->authorization_id_), false, AUTH_ID_STORE_NAME, this->debug_nuki_hex_data_);
    }

    if (is_char_array_empty(this->secret_key_k_, sizeof(this->secret_key_k_)) || is_char_array_empty(this->authorization_id_, sizeof(this->authorization_id_))) {
      ESP_LOGW(TAG, "secret key OR authorization_id is empty: not paired");
      return false;
    }

    bool isUltraLock = false;
    this->is_ultra_pref_.load(&isUltraLock);
    this->smart_lock_ultra_ = isUltraLock;

    if (this->is_lock_ultra()) {
      this->ultra_pincode_pref_.load(&this->ultra_pin_code_);

      if (this->ultra_pin_code_ == 0) {
        ESP_LOGW(TAG, "Pincode is 000000, probably not defined");
      }
    } else {
      this->security_pincode_pref_.load(&this->pin_code_);

      if (this->pin_code_ == 0) {
        ESP_LOGW(TAG, "Pincode is 0000, probably not defined");
      }
    }
  } else {
    ESP_LOGE(TAG, "No credentials found - not paired yet!");
    return false;
  }
  return true;
}

void NukiBle::delete_credentials() {
  unsigned char emptySecretKeyK[32] = {0x00};
  unsigned char emptyAuthorizationId[4] = {0x00};
  bool isUltraLock = false;
  this->secret_key_pref_.save(&emptySecretKeyK);
  this->auth_id_pref_.save(&emptyAuthorizationId);
  this->is_ultra_pref_.save(&isUltraLock);
  esphome::global_preferences->sync();

  if (this->debug_nuki_connect_) {
    ESP_LOGD(TAG, "Credentials deleted");
  }
}

PairingState NukiBle::pair_state_machine(const PairingState nukiPairingState) {
  switch (nukiPairingState) {
    case PairingState::InitPairing: {
      memset(this->challenge_nonce_k_, 0, sizeof(this->challenge_nonce_k_));
      memset(this->remote_public_key_, 0, sizeof(this->remote_public_key_));
      this->received_status_ = 0xff;
      this->time_now_ = (esp_timer_get_time() / 1000);
      this->nuki_pairing_result_state_ = PairingState::ReqRemPubKey;
    }
    case PairingState::ReqRemPubKey: {
      //Request remote public key (Sent message should be 0100030027A7)
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Pairing: requesting remote public key");
      }
      unsigned char buff[sizeof(Command)];
      uint16_t cmd = (uint16_t)Command::PublicKey;
      memcpy(buff, &cmd, sizeof(Command));
      this->send_plain_message(Command::RequestData, buff, sizeof(Command));
      this->nuki_pairing_result_state_ = PairingState::RecRemPubKey;
    }
    case PairingState::RecRemPubKey: {
      if (is_char_array_not_empty(this->remote_public_key_, sizeof(this->remote_public_key_))) {
        this->nuki_pairing_result_state_ = PairingState::SendPubKey;
      }
      break;
    }
    case PairingState::SendPubKey: {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Pairing: sending client public key");
      }
      this->send_plain_message(Command::PublicKey, this->my_public_key_, sizeof(this->my_public_key_));
      this->nuki_pairing_result_state_ = PairingState::GenKeyPair;
    }
    case PairingState::GenKeyPair: {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Pairing: calculating DH shared key");
      }
      unsigned char sharedKeyS[32] = {0x00};
      crypto_scalarmult_curve25519(sharedKeyS, this->my_private_key_, this->remote_public_key_);
      print_buffer(sharedKeyS, sizeof(sharedKeyS), false, "Shared key s", this->debug_nuki_hex_data_);

      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Pairing: deriving long-term shared secret key");
      }
      unsigned char in[16];
      memset(in, 0, 16);
      unsigned char sigma[] = "expand 32-byte k";
      crypto_core_hsalsa20(this->secret_key_k_, in, sharedKeyS, sigma);
      print_buffer(this->secret_key_k_, sizeof(this->secret_key_k_), false, "Secret key k", this->debug_nuki_hex_data_);
      this->nuki_pairing_result_state_ = PairingState::CalculateAuth;
    }
    case PairingState::CalculateAuth: {
      if (is_char_array_not_empty(this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_))) {
        if (this->debug_nuki_connect_) {
          ESP_LOGD(TAG, "Pairing: calculating/verifying authenticator");
        }
        //concatenate local public key, remote public key and receive challenge data
        unsigned char hmacPayload[96];
        memcpy(&hmacPayload[0], this->my_public_key_, sizeof(this->my_public_key_));
        memcpy(&hmacPayload[32], this->remote_public_key_, sizeof(this->remote_public_key_));
        memcpy(&hmacPayload[64], this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_));
        print_buffer((uint8_t*)hmacPayload, sizeof(hmacPayload), false, "Concatenated data r", this->debug_nuki_hex_data_);
        crypto_auth_hmacsha256(this->authenticator_, hmacPayload, sizeof(hmacPayload), this->secret_key_k_);
        print_buffer(this->authenticator_, sizeof(this->authenticator_), false, "HMAC 256 result", this->debug_nuki_hex_data_);
        memset(this->challenge_nonce_k_, 0, sizeof(this->challenge_nonce_k_));
        this->nuki_pairing_result_state_ = PairingState::SendAuth;
      }
      break;
    }
    case PairingState::SendAuth: {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(TAG, "Pairing: sending authenticator");
      }
      this->send_plain_message(Command::AuthorizationAuthenticator, this->authenticator_, sizeof(this->authenticator_));
      this->ultra_auth_info_command_received_ = false;
      this->nuki_pairing_result_state_ = PairingState::SendAuthData;
    }
    case PairingState::SendAuthData: {
      if (this->is_lock_ultra()) {
        if (this->ultra_auth_info_command_received_) {
          this->ultra_auth_info_command_received_ = false;

          if (this->debug_nuki_connect_) {
            ESP_LOGD(TAG, "Pairing: sending authorization data (Ultra)");
          }

          unsigned char authorizationDataId[4] = {};
          unsigned char authorizationDataName[32] = {};
          authorizationDataId[0] = (this->device_id_ >> (8 * 0)) & 0xff;
          authorizationDataId[1] = (this->device_id_ >> (8 * 1)) & 0xff;
          authorizationDataId[2] = (this->device_id_ >> (8 * 2)) & 0xff;
          authorizationDataId[3] = (this->device_id_ >> (8 * 3)) & 0xff;
          memcpy(authorizationDataName, this->device_name_.c_str(), this->device_name_.size());

          //compose and send message
          unsigned char authorizationDataMessage[40];
          memcpy(&authorizationDataMessage[0], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationDataMessage[4], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationDataMessage[36], &this->ultra_pin_code_, 4);

          this->encrypt_pairing_ = true;
          this->send_encrypted_message(Command::AuthorizationData, authorizationDataMessage, sizeof(authorizationDataMessage));
          this->nuki_pairing_result_state_ = PairingState::RecStatus;
        }
      } else {
        if (is_char_array_not_empty(this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_))) {
          if (this->debug_nuki_connect_) {
            ESP_LOGD(TAG, "Pairing: sending authorization data");
          }
          unsigned char authorizationData[101] = {};
          unsigned char authorizationDataIdType[1] = {(unsigned char)this->authorization_id_type_ };
          unsigned char authorizationDataId[4] = {};
          unsigned char authorizationDataName[32] = {};
          unsigned char authorizationDataNonce[32] = {};
          authorizationDataId[0] = (this->device_id_ >> (8 * 0)) & 0xff;
          authorizationDataId[1] = (this->device_id_ >> (8 * 1)) & 0xff;
          authorizationDataId[2] = (this->device_id_ >> (8 * 2)) & 0xff;
          authorizationDataId[3] = (this->device_id_ >> (8 * 3)) & 0xff;
          memcpy(authorizationDataName, this->device_name_.c_str(), this->device_name_.size());
          generate_nonce(authorizationDataNonce, sizeof(authorizationDataNonce), this->debug_nuki_hex_data_);

          //calculate authenticator of message to send
          memcpy(&authorizationData[0], authorizationDataIdType, sizeof(authorizationDataIdType));
          memcpy(&authorizationData[1], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationData[5], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationData[37], authorizationDataNonce, sizeof(authorizationDataNonce));
          memcpy(&authorizationData[69], this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_));
          crypto_auth_hmacsha256(this->authenticator_, authorizationData, sizeof(authorizationData), this->secret_key_k_);

          //compose and send message
          unsigned char authorizationDataMessage[101];
          memcpy(&authorizationDataMessage[0], this->authenticator_, sizeof(this->authenticator_));
          memcpy(&authorizationDataMessage[32], authorizationDataIdType, sizeof(authorizationDataIdType));
          memcpy(&authorizationDataMessage[33], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationDataMessage[37], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationDataMessage[69], authorizationDataNonce, sizeof(authorizationDataNonce));

          memset(this->challenge_nonce_k_, 0, sizeof(this->challenge_nonce_k_));
          this->send_plain_message(Command::AuthorizationData, authorizationDataMessage, sizeof(authorizationDataMessage));
          this->nuki_pairing_result_state_ = PairingState::SendAuthIdConf;
        }
      }
      break;
    }
    case PairingState::SendAuthIdConf: {
      if (is_char_array_not_empty(this->authorization_id_, sizeof(this->authorization_id_))) {
        if (this->debug_nuki_connect_) {
          ESP_LOGD(TAG, "Pairing: sending authorization ID confirmation");
        }
        unsigned char confirmationData[36] = {};

        //calculate authenticator of message to send
        memcpy(&confirmationData[0], this->authorization_id_, sizeof(this->authorization_id_));
        memcpy(&confirmationData[4], this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_));
        crypto_auth_hmacsha256(this->authenticator_, confirmationData, sizeof(confirmationData), this->secret_key_k_);

        //compose and send message
        unsigned char confirmationDataMessage[36];
        memcpy(&confirmationDataMessage[0], this->authenticator_, sizeof(this->authenticator_));
        memcpy(&confirmationDataMessage[32], this->authorization_id_, sizeof(this->authorization_id_));
        this->send_plain_message(Command::AuthorizationIdConfirmation, confirmationDataMessage, sizeof(confirmationDataMessage));
        this->nuki_pairing_result_state_ = PairingState::RecStatus;
      }
      break;
    }
    case PairingState::RecStatus: {
      if (this->received_status_ == 0) {
        if (this->debug_nuki_connect_) {
          ESP_LOGD(TAG, "Pairing: done");
        }
        this->nuki_pairing_result_state_ = PairingState::Success;
      }
      break;
    }
    default: {
      ESP_LOGE(TAG, "Unknown pairing status");
      this->nuki_pairing_result_state_ = PairingState::Timeout;
    }
  }

  constexpr uint32_t pairing_timeout_ms = 30000;
  if ((esp_timer_get_time() / 1000) - this->time_now_ > pairing_timeout_ms) {
    ESP_LOGW(TAG, "Pairing timeout");
    this->nuki_pairing_result_state_ = PairingState::Timeout;
  }

  return this->nuki_pairing_result_state_;
}

bool NukiBle::send_encrypted_message(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen) {
  /*
  #     ADDITIONAL DATA (not encr)      #                    PLAIN DATA (encr)                             #
  #  nonce  # auth identifier # msg len # authorization identifier # command identifier # payload #  crc   #
  # 24 byte #    4 byte       # 2 byte  #      4 byte              #       2 byte       #  n byte # 2 byte #
  */

  //compose plain data
  unsigned char plainData[6 + payloadLen] = {};
  unsigned char plainDataWithCrc[8 + payloadLen] = {};

  if(this->encrypt_pairing_) {
    plainData[0] = (this->device_id_ >> (8 * 0)) & 0xff;
    plainData[1] = (this->device_id_ >> (8 * 1)) & 0xff;
    plainData[2] = (this->device_id_ >> (8 * 2)) & 0xff;
    plainData[3] = (this->device_id_ >> (8 * 3)) & 0xff;
  } else {
    memcpy(&plainData[0], &this->authorization_id_, sizeof(this->authorization_id_));
  }
  memcpy(&plainData[4], &commandIdentifier, sizeof(commandIdentifier));
  memcpy(&plainData[6], payload, payloadLen);

  //get crc over plain data
  uint16_t dataCrc = calculate_crc((uint8_t*)plainData, 0, sizeof(plainData));

  memcpy(&plainDataWithCrc[0], &plainData, sizeof(plainData));
  memcpy(&plainDataWithCrc[sizeof(plainData)], &dataCrc, sizeof(dataCrc));

  if (this->debug_nuki_hex_data_) {
    ESP_LOGD(TAG, "payloadlen: %d", payloadLen);
    ESP_LOGD(TAG, "sizeof(plainData): %d", sizeof(plainData));
    ESP_LOGD(TAG, "CRC: %02x", dataCrc);
  }
  print_buffer((uint8_t*)plainDataWithCrc, sizeof(plainDataWithCrc), false, "Plain data with CRC: ", this->debug_nuki_hex_data_);

  //compose additional data
  unsigned char additionalData[30] = {};
  generate_nonce(this->sent_nonce_, sizeof(this->sent_nonce_), this->debug_nuki_hex_data_);

  memcpy(&additionalData[0], this->sent_nonce_, sizeof(this->sent_nonce_));

  if(this->encrypt_pairing_) {
    additionalData[24] = (this->device_id_ >> (8 * 0)) & 0xff;
    additionalData[25] = (this->device_id_ >> (8 * 1)) & 0xff;
    additionalData[26] = (this->device_id_ >> (8 * 2)) & 0xff;
    additionalData[27] = (this->device_id_ >> (8 * 3)) & 0xff;
  } else {
    memcpy(&additionalData[24], this->authorization_id_, sizeof(this->authorization_id_));
  }

  //Encrypt plain data
  unsigned char plainDataEncr[ sizeof(plainDataWithCrc) + crypto_secretbox_MACBYTES] = {0};
  int encrMsgLen = encode(plainDataEncr, plainDataWithCrc, sizeof(plainDataWithCrc), this->sent_nonce_, this->secret_key_k_);

  if (encrMsgLen >= 0) {
    int16_t length = sizeof(plainDataEncr);
    memcpy(&additionalData[28], &length, 2);

    print_buffer((uint8_t*)additionalData, 30, false, "Additional data: ", this->debug_nuki_hex_data_);
    print_buffer((uint8_t*)this->secret_key_k_, sizeof(this->secret_key_k_), false, "Encryption key (secretKey): ", this->debug_nuki_hex_data_);
    print_buffer((uint8_t*)plainDataEncr, sizeof(plainDataEncr), false, "Plain data encrypted: ", this->debug_nuki_hex_data_);

    //compose complete message
    unsigned char dataToSend[sizeof(additionalData) + sizeof(plainDataEncr)] = {};
    memcpy(&dataToSend[0], additionalData, sizeof(additionalData));
    memcpy(&dataToSend[30], plainDataEncr, sizeof(plainDataEncr));

    if(this->encrypt_pairing_) {
      if (this->connect_ble(true)) {
        print_buffer((uint8_t*)dataToSend, sizeof(dataToSend), false, "Sending encrypted pairing message", this->debug_nuki_hex_data_);
        this->encrypt_pairing_ = false;
        this->recieve_encrypted_ = true;
        return this->gdio_char_->write_value((uint8_t*)dataToSend, sizeof(dataToSend), ESP_GATT_WRITE_TYPE_RSP) == ESP_OK;
      } else {
        // Not an error - connect_ble() returns false while still connecting/establishing,
        // which legitimately takes several seconds and is retried every tick, so this is
        // expected and not worth a warning. Log it once per attempt, not every retry.
        if (!this->not_connected_logged_) {
          this->not_connected_logged_ = true;
          ESP_LOGD(TAG, "Not connected yet, will retry sending encrypted pairing message");
        }
      }
    } else {
      if (this->connect_ble(false)) {
        print_buffer((uint8_t*)dataToSend, sizeof(dataToSend), false, "Sending encrypted message", this->debug_nuki_hex_data_);
        return this->usdio_char_->write_value((uint8_t*)dataToSend, sizeof(dataToSend), ESP_GATT_WRITE_TYPE_RSP) == ESP_OK;
      } else {
        // Not an error - see comment above. Log it once per attempt, not every retry.
        if (!this->not_connected_logged_) {
          this->not_connected_logged_ = true;
          ESP_LOGD(TAG, "Not connected yet, will retry sending encrypted message");
        }
      }
    }
  } else {
    ESP_LOGW(TAG, "Send msg failed due to encryption fail");
  }
  return false;
}

bool NukiBle::send_plain_message(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen) {
  /*
  #                PLAIN DATA                   #
  #command identifier  #   payload   #   crc    #
  #      2 byte        #   n byte    #  2 byte  #
  */

  //compose data
  char dataToSend[200];
  memcpy(&dataToSend, &commandIdentifier, sizeof(commandIdentifier));
  memcpy(&dataToSend[2], payload, payloadLen);
  uint16_t dataCrc = calculate_crc((uint8_t*)dataToSend, 0, payloadLen + 2);

  memcpy(&dataToSend[2 + payloadLen], &dataCrc, sizeof(dataCrc));
  print_buffer((uint8_t*)dataToSend, payloadLen + 4, false, "Sending plain message", this->debug_nuki_hex_data_);
  if (this->debug_nuki_hex_data_) {
    ESP_LOGD(TAG, "Command identifier: %02x, CRC: %04x", (unsigned int)commandIdentifier, dataCrc);
  }

  if (this->connect_ble(true)) {
    return this->gdio_char_->write_value((uint8_t*)dataToSend, payloadLen + 4, ESP_GATT_WRITE_TYPE_RSP) == ESP_OK;
  } else {
    // Not an error - see comment in send_encrypted_message(). Log it once per attempt, not
    // every retry.
    if (!this->not_connected_logged_) {
      this->not_connected_logged_ = true;
      ESP_LOGD(TAG, "Not connected yet, will retry sending plain message");
    }
  }
  return false;
}

// Called from gattc_event_handler()'s ESP_GATTC_SEARCH_CMPL_EVT handling (i.e. once
// connected and services/characteristics are discovered - see BLEClientBase). Looks up the
// GDIO (or GDIO-Ultra) characteristic under the Keyturner Pairing Service and requests a
// notify/indicate subscription on it; gattc_event_handler()'s ESP_GATTC_REG_FOR_NOTIFY_EVT
// handling sets notify_registered once that subscription is confirmed. On any failure here,
// disconnect() gives up on this connection attempt entirely (a fresh connect will retry
// service discovery from scratch) - matching the original NimBLE behavior.
bool NukiBle::register_on_gdio_char() {
  esphome::esp32_ble_tracker::ESPBTUUID serviceUuid = this->is_lock_ultra() ? this->pairing_service_ultra_uuid_ : this->pairing_service_uuid_;
  esphome::esp32_ble_tracker::ESPBTUUID charUuid = this->is_lock_ultra() ? this->gdio_ultra_uuid_ : this->gdio_uuid_;

  auto *chr = this->get_characteristic(serviceUuid, charUuid);
  if (chr == nullptr) {
    ESP_LOGW(TAG, "Unable to get GDIO characteristic");
    this->disconnect();
    return false;
  }

  if (!(chr->properties & ESP_GATT_CHAR_PROP_BIT_INDICATE)) {
    if (this->debug_nuki_communication_) {
      ESP_LOGD(TAG, "GDIO characteristic canIndicate false, stop connecting");
    }
    this->disconnect();
    return false;
  }

  this->gdio_char_ = chr;
  auto status = esp_ble_gattc_register_for_notify(this->get_gattc_if(), this->get_remote_bda(), chr->handle);
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "Unable to subscribe to GDIO characteristic");
    this->disconnect();
    return false;
  }

  if (this->debug_nuki_communication_) {
    ESP_LOGD(TAG, "GDIO characteristic registered");
  }
  return true;
}

// Same as register_on_gdio_char(), but for the USDIO characteristic under the Keyturner
// (device) Service, used for normal (non-pairing) operation.
bool NukiBle::register_on_usdio_char() {
  auto *chr = this->get_characteristic(this->device_service_uuid_, this->user_data_uuid_);
  if (chr == nullptr) {
    ESP_LOGW(TAG, "Unable to get USDIO characteristic");
    this->disconnect();
    return false;
  }

  if (!(chr->properties & ESP_GATT_CHAR_PROP_BIT_INDICATE)) {
    if (this->debug_nuki_communication_) {
      ESP_LOGD(TAG, "USDIO characteristic canIndicate false, stop connecting");
    }
    this->disconnect();
    return false;
  }

  this->usdio_char_ = chr;
  auto status = esp_ble_gattc_register_for_notify(this->get_gattc_if(), this->get_remote_bda(), chr->handle);
  if (status != ESP_OK) {
    ESP_LOGW(TAG, "Unable to subscribe to USDIO characteristic");
    this->disconnect();
    return false;
  }

  if (this->debug_nuki_communication_) {
    ESP_LOGD(TAG, "USDIO characteristic registered");
  }
  return true;
}

// Drives the GATT connection lifecycle: calls the base class implementation first (which
// handles connect/MTU negotiation/service discovery/the generic notify-subscribe CCCD
// write and returns false for events that aren't for this client), then adds Nuki-specific
// handling on top - discovering the relevant characteristic once services are known,
// tracking notify-subscription completion, and routing incoming data to notify_callback().
bool NukiBle::gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                                  esp_ble_gattc_cb_param_t *param) {
  if (!esphome::esp32_ble_client::BLEClientBase::gattc_event_handler(event, gattc_if, param)) {
    return false;
  }

  switch (event) {
    case ESP_GATTC_OPEN_EVT: {
      if (param->open.status == ESP_GATT_OK || param->open.status == ESP_GATT_ALREADY_OPEN) {
        this->extend_disconnect_timeout();
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      // On failure, register_on_gdio_char()/register_on_usdio_char() already called disconnect().
      if (this->pairing_mode_) {
        this->register_on_gdio_char();
      } else {
        this->register_on_usdio_char();
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      auto *chr = this->pairing_mode_ ? this->gdio_char_ : this->usdio_char_;
      if (chr != nullptr && param->reg_for_notify.handle == chr->handle) {
        if (param->reg_for_notify.status == ESP_GATT_OK) {
          this->notify_registered_ = true;
        } else {
          ESP_LOGW(TAG, "Unable to subscribe characteristic (status %d)", param->reg_for_notify.status);
          this->disconnect();
        }
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT: {
      this->notify_callback(param->notify.handle, param->notify.value, param->notify.value_len);
      break;
    }
    default:
      break;
  }
  return true;
}

// Called from gattc_event_handler()'s ESP_GATTC_NOTIFY_EVT handling. handle identifies
// which characteristic the data arrived on (compared against gdio_char/usdio_char's
// handles) in place of the NimBLE-era UUID comparison.
void NukiBle::notify_callback(uint16_t handle, uint8_t* recData, uint16_t length) {
  this->last_heartbeat_ = (esp_timer_get_time() / 1000);

  bool isGdioHandle = this->gdio_char_ != nullptr && handle == this->gdio_char_->handle;
  bool isUsdioHandle = this->usdio_char_ != nullptr && handle == this->usdio_char_->handle;
  // The GDIO-Ultra characteristic carries both plain (pairing) and, briefly during one
  // pairing step, encrypted messages for Ultra locks - see recieve_encrypted below.
  bool isGdioUltra = isGdioHandle && this->is_lock_ultra();

  if (this->debug_nuki_communication_) {
    ESP_LOGD(TAG, "Notify callback for characteristic handle: %d of length: %d", handle, length);
  }
  print_buffer((uint8_t*)recData, length, false, "Received data", this->debug_nuki_hex_data_);

  if ((isGdioHandle && !this->is_lock_ultra()) || (isGdioUltra && (!this->recieve_encrypted_ || length < 24))) {
    //handle not encrypted msg
    uint16_t returnCode = ((uint16_t)recData[1] << 8) | recData[0];
    this->crc_check_oke_ = crc_valid(recData, length, this->debug_nuki_communication_);
    if (this->crc_check_oke_) {
      unsigned char plainData[200];
      memcpy(plainData, &recData[2], length - 4);
      this->handle_return_message((Command)returnCode, plainData, length - 4);
    }
  } else if (isUsdioHandle || (isGdioUltra && this->recieve_encrypted_)) {
    if (isGdioUltra) {
      this->recieve_encrypted_ = false;
    }
    //handle encrypted msg
    unsigned char recNonce[crypto_secretbox_NONCEBYTES];
    unsigned char recAuthorizationId[4];
    unsigned char recMsgLen[2];
    memcpy(recNonce, &recData[0], crypto_secretbox_NONCEBYTES);
    memcpy(recAuthorizationId, &recData[crypto_secretbox_NONCEBYTES], 4);
    memcpy(recMsgLen, &recData[crypto_secretbox_NONCEBYTES + 4], 2);
    uint16_t encrMsgLen = 0;
    memcpy(&encrMsgLen, recMsgLen, 2);
    unsigned char encrData[encrMsgLen];
    memcpy(&encrData, &recData[crypto_secretbox_NONCEBYTES + 6], encrMsgLen);

    unsigned char decrData[encrMsgLen - crypto_secretbox_MACBYTES];
    decode(decrData, encrData, encrMsgLen, recNonce, this->secret_key_k_);

    if (this->debug_nuki_communication_) {
      ESP_LOGD(TAG, "Received encrypted msg, len: %d", encrMsgLen);
    }
    print_buffer(recNonce, sizeof(recNonce), false, "received nonce", this->debug_nuki_hex_data_);
    print_buffer(recAuthorizationId, sizeof(recAuthorizationId), false, "Received AuthorizationId", this->debug_nuki_hex_data_);
    print_buffer(encrData, sizeof(encrData), false, "Rec encrypted data", this->debug_nuki_hex_data_);
    print_buffer(decrData, sizeof(decrData), false, "Decrypted data", this->debug_nuki_hex_data_);

    this->crc_check_oke_ = crc_valid(decrData, sizeof(decrData), this->debug_nuki_communication_);
    if (this->crc_check_oke_) {
      uint16_t returnCode = 0;
      memcpy(&returnCode, &decrData[4], 2);
      unsigned char payload[sizeof(decrData) - 8];
      memcpy(&payload, &decrData[6], sizeof(payload));
      this->handle_return_message((Command)returnCode, payload, sizeof(payload));
    }
  }
}

void NukiBle::handle_return_message(Command returnCode, unsigned char* data, uint16_t dataLen) {
  switch (returnCode) {
    case Command::RequestData : {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(TAG, "requestData");
      }
      break;
    }
    case Command::PublicKey : {
      memcpy(this->remote_public_key_, data, 32);
      print_buffer(this->remote_public_key_, sizeof(this->remote_public_key_), false,  "Remote public key", this->debug_nuki_hex_data_);
      break;
    }
    case Command::Challenge : {
      memcpy(this->challenge_nonce_k_, data, 32);
      print_buffer((uint8_t*)data, dataLen, false, "Challenge", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationAuthenticator : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationAuthenticator", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationData : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationData", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationId : {
      unsigned char lockId[16];
      print_buffer((uint8_t*)data, dataLen, false, "authorization_id data", this->debug_nuki_hex_data_);
      if (this->is_lock_ultra()) {
        memcpy(this->authorization_id_, &data[0], 4);
        memcpy(lockId, &data[4], sizeof(lockId));
        this->received_status_ = 0;
      } else {
        memcpy(this->authorization_id_, &data[32], 4);
        memcpy(lockId, &data[36], sizeof(lockId));
        memcpy(this->challenge_nonce_k_, &data[52], sizeof(this->challenge_nonce_k_));
      }
      print_buffer(this->authorization_id_, sizeof(this->authorization_id_), false, AUTH_ID_STORE_NAME, this->debug_nuki_hex_data_);
      print_buffer(lockId, sizeof(lockId), false, "lockId", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationEntry : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationEntry", this->debug_nuki_hex_data_);
      AuthorizationEntry authEntry;
      memcpy(&authEntry, data, dataLen);
      this->list_of_authorization_entries_.push_back(authEntry);
      if (this->debug_nuki_readable_data_) {
        log_authorization_entry(authEntry, true);
      }
      break;
    }
    case Command::Status : {
      print_buffer((uint8_t*)data, dataLen, false, "status", this->debug_nuki_hex_data_);
      this->received_status_ = data[0];
      if (this->debug_nuki_communication_) {
        if (this->received_status_ == 0) {
          ESP_LOGD(TAG, "command COMPLETE");
        } else if (this->received_status_ == 1) {
          ESP_LOGD(TAG, "command ACCEPTED");
        }
      }
      break;
    }
    case Command::OpeningsClosingsSummary : {
      print_buffer((uint8_t*)data, dataLen, false, "openingsClosingsSummary", this->debug_nuki_hex_data_);
      ESP_LOGW(TAG, "NOT IMPLEMENTED ONLY FOR NUKI v1"); //command is not available on Nuki v2 (only on Nuki v1)
      break;
    }
    case Command::ErrorReport : {
      ESP_LOGE(TAG, "Error: %02x for command: %02x:%02x", data[0], data[2], data[1]);
      memcpy(&this->error_code_, &data[0], sizeof(this->error_code_));
      this->log_error_code(data[0]);
      if ((uint8_t)data[0] == (uint8_t)0x21) {
        if (this->event_handler_) {
          this->event_handler_->notify(EventType::ERROR_BAD_PIN);
        }
      }
      break;
    }
    case Command::AuthorizationIdConfirmation : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationIdConfirmation", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationIdInvite : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationIdInvite", this->debug_nuki_hex_data_);
      break;
    }
    case Command::AuthorizationInfo : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationInfo", this->debug_nuki_hex_data_);
      this->ultra_auth_info_command_received_ = true;
      break;
    }
    case Command::AuthorizationEntryCount : {
      print_buffer((uint8_t*)data, dataLen, false, "authorizationEntryCount", this->debug_nuki_hex_data_);
      memcpy(&this->expected_authorization_entry_count_, data, 2);
      this->authorization_entry_count_received_ = true;
      ESP_LOGD(TAG, "authorizationEntryCount: %d", this->expected_authorization_entry_count_);
      break;
    }
    case Command::LogEntryCount : {
      this->logging_enabled_ = data[0] != 0;
      memcpy(&this->log_entry_count_, &data[1], sizeof(this->log_entry_count_));
      if (this->debug_nuki_readable_data_) {
        ESP_LOGD(TAG, "Logging enabled: %d, total nr of log entries: %d", this->logging_enabled_, this->log_entry_count_);
      }
      print_buffer((uint8_t*)data, dataLen, false, "log_entry_count", this->debug_nuki_hex_data_);
      break;
    }
    case Command::TimeControlEntryCount : {
      print_buffer((uint8_t*)data, dataLen, false, "timeControlEntryCount", this->debug_nuki_hex_data_);
      break;
    }
    case Command::TimeControlEntryId : {
      print_buffer((uint8_t*)data, dataLen, false, "timeControlEntryId", this->debug_nuki_hex_data_);
      break;
    }
    case Command::KeypadCodeId : {
      print_buffer((uint8_t*)data, dataLen, false, "keypadCodeId", this->debug_nuki_hex_data_);
      break;
    }
    case Command::MostRecentCommand : {
      memcpy(&this->most_recent_command_, data, sizeof(this->most_recent_command_));
      if (this->debug_nuki_readable_data_) {
        ESP_LOGD(TAG, "Most recent command: %04x", this->most_recent_command_);
      }
      print_buffer((uint8_t*)data, dataLen, false, "most_recent_command", this->debug_nuki_hex_data_);
      break;
    }
    case Command::KeypadCodeCount : {
      memcpy(&this->nr_of_keypad_codes_, data, 2);
      this->keypad_code_count_received_ = true;
      print_buffer((uint8_t*)data, dataLen, false, "keypadCodeCount", this->debug_nuki_hex_data_);
      if (this->debug_nuki_readable_data_) {
        uint16_t count = 0;
        memcpy(&count, data, 2);
        ESP_LOGD(TAG, "keyPadCodeCount: %d", count);
      }

      break;
    }
    case Command::FingerprintEntry : {
      FingerprintEntry fingerprintEntry;
      memcpy(&fingerprintEntry, data, dataLen);
      this->list_of_fingerprint_entries_.push_back(fingerprintEntry);

      print_buffer((uint8_t*)data, dataLen, false, "fingerprintEntry", this->debug_nuki_hex_data_);
      if (this->debug_nuki_readable_data_) {
        log_fingerprint_entry(fingerprintEntry, true);
      }
      break;
    }
    case Command::KeypadCode : {
      KeypadEntry keypadEntry;
      memcpy(&keypadEntry, data, dataLen);
      this->list_of_key_pad_entries_.push_back(keypadEntry);
      this->nr_of_received_keypad_codes_++;

      print_buffer((uint8_t*)data, dataLen, false, "keypadCode", this->debug_nuki_hex_data_);
      if (this->debug_nuki_readable_data_) {
        log_keypad_entry(keypadEntry, true);
      }
      break;
    }
    case Command::KeypadAction : {
      print_buffer((uint8_t*)data, dataLen, false, "keypad_action", this->debug_nuki_hex_data_);
      break;
    }
    default:
      ESP_LOGE(TAG, "UNKNOWN RETURN COMMAND: %04x", (unsigned int)returnCode);
  }
}

void NukiBle::set_event_handler(SmartlockEventHandler* handler) {
  this->event_handler_ = handler;
}

const bool NukiBle::is_paired_with_lock() const {
  return this->is_paired_;
};

const bool NukiBle::is_lock_ultra() const {
  return this->smart_lock_ultra_;
};

int NukiBle::get_rssi() const {
  return this->rssi_;
}

int64_t NukiBle::get_last_received_beacon_ts() const {
  return this->last_received_beacon_ts_;
}

int64_t NukiBle::get_last_heartbeat() {
  return this->last_heartbeat_;
}

void NukiBle::set_debug_connect(bool enable) {
  this->debug_nuki_connect_ = enable;
}

void NukiBle::set_debug_communication(bool enable) {
  this->debug_nuki_communication_ = enable;
}

void NukiBle::set_debug_readable_data(bool enable) {
  this->debug_nuki_readable_data_ = enable;
}

void NukiBle::set_debug_hex_data(bool enable) {
  this->debug_nuki_hex_data_ = enable;
}

void NukiBle::set_debug_command(bool enable) {
  this->debug_nuki_command_ = enable;
}

}  // namespace esphome::nuki_lock