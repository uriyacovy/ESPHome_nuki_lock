#pragma once
/**
 * @file nuki_ble.h
 *
 * Created: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include "nuki_ble_constants.h"
#include "nuki_accessories.h"

#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/esp32_ble_client/ble_client_base.h"

#include <sodium.h>

#include "esphome/core/log.h"
#include "esp_timer.h"

#include <atomic>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome::nuki_lock {

// BLEClientBase (esp32_ble_client) extends esphome::esp32_ble_tracker::ESPBTClient, which itself extends
// esphome::esp32_ble_tracker::ESPBTDeviceListener - so parse_device() below already overrides the listener
// interface transitively; esphome::esp32_ble_tracker::ESPBTDeviceListener must not be inherited again directly,
// or it would create a duplicate (ambiguous) base.
class NukiBle : public esphome::esp32_ble_client::BLEClientBase {
  public:
    NukiBle(const std::string& device_name,
            const uint32_t device_id,
            const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_uuid,
            const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_ultra_uuid,
            const esphome::esp32_ble_tracker::ESPBTUUID device_service_uuid,
            const esphome::esp32_ble_tracker::ESPBTUUID gdio_uuid,
            const esphome::esp32_ble_tracker::ESPBTUUID gdio_ultra_uuid,
            const esphome::esp32_ble_tracker::ESPBTUUID user_data_uuid,
            const std::string preferenced_id);
    virtual ~NukiBle();

    /**
     * @brief Set the Event Handler object
     *
     * @param handler method to handle the event
     */
    void set_event_handler(SmartlockEventHandler* handler);

    /**
     * @brief Checks if credentials are stored in preferences, if not initiate pairing
     *
     * @return
     */
    PairingResult pair_nuki(AuthorizationIdType idType = AuthorizationIdType::Bridge);

    /**
     * @brief Delete stored credentials
     */
    void unpair_nuki();
    
    /**
     * @brief checks the time past after last connect/communication sent, if the time past > timeout
     * it will disconnect the BLE connection with the lock so that lock will start sending advertisements.
     *
     * This method is optional since the lock will automatically disconnect after approximately 20
     * seconds. However, the lock might be unresponsive during this time if the connection is stale.
     * For this reason, using this method is advised.
     * If used, this method should be run in loop or a task.
     *
     */
    void update_connection_state();

    /**
     * @brief Set the BLE Disconnect Timeout, if longer than ~20 sec the lock will disconnect by itself
     * if there is no BLE communication
     *
     * @param timeoutMs
     */
    void set_disconnect_timeout(uint32_t timeoutMs);

    /**
     * @brief Set the BLE General Timeout in milliseconds.
     *
     * @param timeoutMs
     */
    void set_general_timeout(uint32_t timeoutMs);
    
    /**
     * @brief Set the BLE Command Timeout in seconds.
     *
     * @param timeoutMs
     */
    void set_command_timeout(uint32_t timeoutMs);
    
    /**
     * @brief Set the BLE Connect Timeout in seconds.
     *
     * @param timeout
     */
    void set_connect_timeout(uint8_t timeout);

    /**
     * @brief Set the BLE Connect number of retries.
     *
     * @param retries
     */
    void set_connect_retries(uint8_t retries);

    /**
     * @brief Returns pairing state (if credentials are stored or not)
     */
    const bool is_paired_with_lock() const;
    
    /**
     * @brief Returns if BLE is pairing/paired/connected with a Smart Lock Ultra
     */
    const bool is_lock_ultra() const;

    /**
     * @brief Returns the log entry count. Only available after executing retreiveLogEntries.
     */
    uint16_t get_log_entry_count();

    /**
     * @brief Returns whether the lock's internal event logging is currently enabled. Only
     * available after executing retrieve_log_entries() (which also requests the log entry
     * count, including this flag).
     */
    bool get_logging_enabled();

    /**
     * @brief Sends a request to enable or disable the lock's internal event logging via BLE
     *
     * @param enable true to enable logging, false to disable
     */
    CmdResult enable_logging(const bool enable);

    /**
     * @brief Requests the identifier of the most recently executed command from the lock via BLE
     *
     * @param retrievedCommand the command identifier of the most recently executed command
     */
    CmdResult request_most_recent_command(uint16_t* retrievedCommand);

    /**
     * @brief Send a new keypad entry to the lock via BLE
     *
     * @param newKeypadEntry Nuki api based datatype to be sent
     * Keypad Codes that start with 12 are not allowed
     * 0 is not allowed
     * Duplicates are not allowed
     */
    CmdResult add_keypad_entry(NewKeypadEntry newKeypadEntry);

    /**
     * @brief Send an updated keypad entry to the lock via BLE
     *
     * @param updatedKeypadEntry Nuki api based datatype to be sent
     * Keypad Codes that start with 12 are not allowed
     * 0 is not allowed
     * Duplicates are not allowed
     */
    CmdResult update_keypad_entry(UpdatedKeypadEntry updatedKeyPadEntry);

    /**
    * @brief Returns the keypad entry count.
    * Only available after executing retreiveKeypadEntries.
    */
    uint16_t get_keypad_entry_count();

    /**
     * @brief Request the lock via BLE to send the existing keypad entries
     *
     * @param offset The start offset to be read.
     * @param count The number of entries to be read, starting at the specified offset.
     */
    CmdResult retrieve_keypad_entries(const uint16_t offset, const uint16_t count);

    /**
     * @brief Get the Keypad Entries stored on the esp (after executing retreieveLogKeypadEntries)
     *
     * @param requestedKeyPadEntries list to store the returned Keypad entries
     */
    void get_keypad_entries(std::vector<KeypadEntry>* requestedKeyPadEntries);

    /**
     * @brief Request the lock via BLE to send the existing fingerprint entries
     *
     */
    CmdResult retrieve_fingerprint_entries();
    
    /**
     * @brief Get the Fingerprint Entries stored on the esp (after executing retrieve_fingerprint_entries)
     *
     * @param requestedFingerprintEntries list to store the returned Fingerprint entries
     */
    void get_fingerprint_entries(std::vector<FingerprintEntry>* requestedFingerprintEntries);

    /**
    * @brief Delete a Keypad Entry
    *
    * @param id Id to be deleted
    */
    CmdResult delete_keypad_entry(uint16_t id);
    
    /**
     * @brief Request the lock via BLE to send the existing authorizationentries
     *
     * @param offset The start offset to be read.
     * @param count The number of entries to be read, starting at the specified offset.
     */
    CmdResult retrieve_authorization_entries(const uint16_t offset, const uint16_t count);

    /**
     * @brief Get the Authorization Entries stored on the esp (after executing retreiveAuthorizationEntries)
     *
     * @param requestedAuthorizationEntries list to store the returned Authorization entries
     */
    void get_authorization_entries(std::vector<AuthorizationEntry>* requestedAuthorizationEntries);

    /**
     * @brief Sends a new authorization entry to the lock via BLE
     *
     * @param newAuthorizationEntry Nuki api based datatype to send
     */
    CmdResult add_authorization_entry(NewAuthorizationEntry newAuthorizationEntry);

    /**
     * @brief Deletes the authorization entry from the lock
     *
     * @param id id to be deleted
     */
    CmdResult delete_authorization_entry(const uint32_t id);

    /**
     * @brief Sends an updated authorization entry to the lock via BLE
     *
     * @param updatedAuthorizationEntry Nuki api based datatype to send
     */
    CmdResult update_authorization_entry(UpdatedAuthorizationEntry updatedAuthorizationEntry);

    /**
     * @brief Sends an calibration (mechanical) request to the lock via BLE
     */
    CmdResult request_calibration();

    /**
     * @brief Sends an reboot request to the lock via BLE
     *
     */
    CmdResult request_reboot();
    
    /**
     * @brief Sends a custom command to the lock via BLE
     *
     * @param command Nuki command to execute
     * @param withPin Set to true when using challenge and pin command
     */
    CmdResult generic_command(Command command, bool withPin = true);

    /**
     * @brief Sends a request for daily statistics to the lock via BLE
     *
     */
    CmdResult request_daily_statistics();

    /**
     * @brief Sends the time to be set to the lock via BLE
     *
     * @param time Nuki api based datatype to send
     */
    CmdResult update_time(TimeValue time);

    /**
     * @brief Saves the pincode on the esp. This pincode is used for sending/setting config via BLE to the lock
     * by other methods and needs to be the same pincode as stored in the lock
     *
     * @param pin_code
     * @return true if stored successfully
     */
    bool save_security_pincode(const uint16_t pin_code);
    bool save_ultra_pincode(const uint32_t pin_code, bool save = true);

    /**
     * @brief Gets the pincode stored on the esp. This pincode is used for sending/setting config via BLE to the lock
     * by other methods and needs to be the same pincode as stored in the lock
     *
     * @return pincode
     */
    uint16_t get_security_pincode();
    uint32_t get_ultra_pincode();

    /**
     * @brief Send the new pincode command to the lock via BLE
     * (this command uses the earlier by save_security_pincode() stored pincode which needs to be the same as
     * the pincode stored in the lock)
     *
     * @param newSecurityPin
     * @return CmdResult
     */
    CmdResult set_security_pin(const uint16_t newSecurityPin);
    CmdResult set_ultra_pin(const uint32_t newSecurityPin);

    /**
     * @brief Send the verify pincode command via BLE to the lock.
     * This command uses the earlier by save_security_pincode() stored pincode
     *
     * @return CmdResult returns success when the pin code is correct (same as stored in the lock)
     */
    CmdResult verify_security_pin();

    /**
     * @brief Gets the ble mac address of the paired lock stored on the esp.
     *
     * @return 18 byte Char array with mac address
     */
    void get_mac_address(char* macAddress);

    /**
     * @brief Initializes stored preferences based on the devicename passed in the constructor,
     * creates the BLE client, sets the BLE callback and checks if the lock is paired
     * (if credentials are stored in preferences)
     */
    void initialize();

    /**
    * @brief Returns the RSSI of the last received ble beacon broadcast
    *
    * @return RSSI value
    */
    int get_rssi() const;

    /**
    * @brief Returns the timestamp in milliseconds when the last ble beacon has been received from the device
    *
    * @return Timestamp in milliseconds
    */
    int64_t get_last_received_beacon_ts() const;

    /**
    * @brief Returns the timestamp (millis) of the last received BLE beacon from the lock.
    *
    * @return Last heartbeat value
    */
    int64_t get_last_heartbeat();

    /**
     * @brief Whether to enable or disable connect debug logging
     *
     * @param enable Set to true to enable connect debug logging
     */
    void set_debug_connect(bool enable);

    /**
     * @brief Whether to enable or disable communication debug logging
     *
     * @param enable Set to true to enable communication debug logging
     */
    void set_debug_communication(bool enable);

    /**
     * @brief Whether to enable or disable readable data debug logging
     *
     * @param enable Set to true to enable readable data debug logging
     */
    void set_debug_readable_data(bool enable);

    /**
     * @brief Whether to enable or disable hex data debug logging
     *
     * @param enable Set to true to enable hex data debug logging
     */
    void set_debug_hex_data(bool enable);

    /**
     * @brief Whether to enable or disable command debug logging
     *
     * @param enable Set to true to enable command debug logging
     */
    void set_debug_command(bool enable);

    /**
     * @brief Tells parse_device() whether the ESPHome side is actively trying to pair right
     * now (the `pairing_mode` switch), so that an unrelated nearby lock - or this lock
     * outside of an intentional pairing attempt - entering its own pairing-advertisement
     * mode doesn't get logged/tracked as a discovery.
     *
     * @param active Set to true while pairing mode is active
     */
    void set_pairing_mode_active(bool active) { this->pairing_mode_active_ = active; }

  protected:
    // Connects (if needed) to the service relevant for `pairing` (pairing service/GDIO vs.
    // device service/USDIO - see register_on_gdio_char()/register_on_usdio_char()) and returns
    // true once ready to send. Targets BLEClientBase's own address_ (see set_address() call
    // sites). Non-blocking: connect() (from BLEClientBase) returns immediately, with the
    // actual connection progress observed via gattc_event_handler()/state(); callers are
    // expected to call this repeatedly (e.g. once
    // per loop() tick) until it returns true.
    bool connect_ble(bool pairing);
    void extend_disconnect_timeout();

    template <typename TDeviceAction>
    CmdResult execute_action(const TDeviceAction action);

    template <typename TDeviceAction>
    CmdResult cmd_state_machine(const TDeviceAction action);

    template <typename TDeviceAction>
    CmdResult cmd_chall_state_machine(const TDeviceAction action, const bool sendPinCode = false);

    template <typename TDeviceAction>
    CmdResult cmd_chall_acc_state_machine(const TDeviceAction action);

    virtual void handle_return_message(Command returnCode, unsigned char* data, uint16_t dataLen);
    virtual void log_error_code(uint8_t error_code) = 0;

    // Cannot initialize to any meaningful value since error namespaces are only
    // defined for NukeBle descendants. Using zero as a safe default, which should
    // work better than random for a general case.
    uint8_t error_code_ = 0;

    Command last_msg_code_received_ = Command::Empty;

    bool debug_nuki_connect_ = false;
    bool debug_nuki_communication_ = false;
    bool debug_nuki_readable_data_ = false;
    bool debug_nuki_hex_data_ = false;
    bool debug_nuki_command_ = false;

  private:
    bool status_updated_ = false;
    bool smart_lock_ultra_ = false;
    bool ultra_auth_info_command_received_ = false;
    bool encrypt_pairing_ = false;
    bool recieve_encrypted_ = false;
    uint32_t timeout_duration_ = 1000;
    uint32_t general_timeout_duration_ = 10000;
    uint32_t command_timeout_duration_ = 3000;
    // Kept (and still settable) for API compatibility with nuki_lock.cpp's setup(); no
    // longer consumed internally - BLEClientBase::connect() is a single non-blocking
    // attempt per call, and the existing execute_action()-driven retry-via-repeated-calls
    // (see the template methods below) already retries connect_ble() every tick for free, bounded by
    // command_timeout_duration like every other command.
    uint8_t connect_timeout_sec_ = 1;
    uint8_t connect_retries_ = 5;

    // pair_nuki() used to run pair_state_machine() to completion in a blocking loop;
    // it now performs one step per call and keeps its progress in these members
    // so it can be driven by repeated calls (e.g. once per loop() tick).
    bool pairing_in_progress_ = false;
    bool pairing_keypair_generated_ = false;
    PairingState pairing_state_ = PairingState::InitPairing;

    // retrieve_keypad_entries() used to busy-wait for the keypad code count and then for
    // every individual code to arrive via notify_callback(); it now tracks progress here
    // and performs one step per call instead, like execute_action().
    enum class KeypadRetrievalState : uint8_t { Idle, AwaitingCount, AwaitingCodes };
    KeypadRetrievalState keypad_retrieval_state_ = KeypadRetrievalState::Idle;
    int64_t keypad_retrieval_time_now_ = 0;
    // RequestKeypadCodes uses cmd_chall_state_machine(), whose "success" detection is satisfied
    // by the *first* non-empty/non-error response - which, for this command, is the
    // KeypadCodeCount notification itself. So by the time execute_action() reports Success
    // (not Working) on some tick, keypad_code_count_received may already be true, set from
    // *that same* notification. Re-clearing it unconditionally on every Idle-state poll
    // (while still waiting for execute_action() to finish) would wipe it out right as we
    // transition to AwaitingCount on that very tick. keypad_request_sent guards the
    // clear/reset so it only happens once, when a *new* retrieval actually starts.
    bool keypad_request_sent_ = false;

    // retrieve_authorization_entries() used to busy-wait for the entry count and then for
    // every individual entry to arrive via notify_callback(), same as keypad codes above; it
    // now tracks progress here and performs one step per call instead. Without this,
    // execute_action()'s underlying challenge/accept handshake completes (and reports
    // Success) as soon as the lock acknowledges the request - well before the individual
    // AuthorizationEntry notifications that make up the actual response have arrived,
    // letting callers move on to the next BLE command and interrupt the still-incoming list.
    enum class EntryRetrievalState : uint8_t { Idle, AwaitingCount, AwaitingEntries };
    EntryRetrievalState auth_entries_retrieval_state_ = EntryRetrievalState::Idle;
    int64_t auth_entries_retrieval_time_now_ = 0;
    bool authorization_entry_count_received_ = false;
    uint16_t expected_authorization_entry_count_ = 0;
    // Guards the clear/reset above against the same Idle-state-keeps-getting-re-polled
    // problem described for keypad_request_sent.
    bool auth_entries_request_sent_ = false;

    // GATT client (Block 2 of the ESPHome-BLE-stack migration): connect_ble() now just
    // drives BLEClientBase's own (non-blocking) connect()/state(), instead of a dedicated
    // mini-task running blocking NimBLE calls. gattc_event_handler() drives the rest of the
    // connection lifecycle (service/characteristic discovery, notify subscription) that
    // register_on_gdio_char()/register_on_usdio_char()/notify_callback() used to do directly via
    // NimBLE.
    bool gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                             esp_ble_gattc_cb_param_t *param) override;
    void on_disconnect_complete(esp_err_t reason) override;
    bool parse_device(const esphome::esp32_ble_tracker::ESPBTDevice &device) override;

    // Whether the current (or in-progress) connection was/is for the pairing handshake
    // (Keyturner Pairing Service/GDIO) or normal operation (Keyturner Service/USDIO) - set
    // by connect_ble(), read by gattc_event_handler()'s ESP_GATTC_SEARCH_CMPL_EVT handling to
    // decide which characteristic to discover/subscribe.
    bool pairing_mode_ = false;
    // Set once gattc_event_handler() observes a successful ESP_GATTC_REG_FOR_NOTIFY_EVT for
    // the relevant characteristic; connect_ble() is only "ready to send" once this is true -
    // sending before the notify subscription is active would mean the lock's response
    // never reaches us.
    bool notify_registered_ = false;
    // Send functions log once when connect_ble() isn't ready yet, instead of every single
    // retry tick (which, at the 50ms cadence used while a Nuki operation is active, would
    // otherwise spam dozens of identical lines for a connection attempt that legitimately
    // takes several seconds). Reset by connect_ble() whenever a fresh connect() is kicked
    // off, so the next wait logs its one line again.
    bool not_connected_logged_ = false;
    esphome::esp32_ble_client::BLECharacteristic *gdio_char_ = nullptr;
    esphome::esp32_ble_client::BLECharacteristic *usdio_char_ = nullptr;

    bool register_on_gdio_char();
    bool register_on_usdio_char();

    bool send_plain_message(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen);
    bool send_encrypted_message(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen);

    void notify_callback(uint16_t handle, uint8_t* data, uint16_t length);
    void save_credentials();
    bool retrieve_credentials();
    void delete_credentials();
    PairingState pair_state_machine(const PairingState nukiPairingState);
    PairingState nuki_pairing_result_state_ = PairingState::InitPairing;

    unsigned char authenticator_[32];

    // Pairing credentials are persisted through ESPHome's own preferences/NVS
    // wrapper instead of a separate NVS namespace, so they participate in the
    // same batched flash writes (esphome::preferences) as the rest of ESPHome.
    template <typename T>
    esphome::ESPPreferenceObject make_credential_pref(const char* keyName) {
      return esphome::global_preferences->make_preference<T>(esphome::fnv1_hash(preferences_id_ + "_" + keyName), true);
    }
    esphome::ESPPreferenceObject ble_address_pref_;
    esphome::ESPPreferenceObject secret_key_pref_;
    esphome::ESPPreferenceObject auth_id_pref_;
    esphome::ESPPreferenceObject security_pincode_pref_;
    esphome::ESPPreferenceObject ultra_pincode_pref_;
    esphome::ESPPreferenceObject is_ultra_pref_;

    bool pairing_service_available_ = false;
    // Set via set_pairing_mode_active() - mirrors NukiLockComponent's `pairing_mode` switch, so
    // parse_device()'s unpaired-device branch only reacts to pairing-mode advertisements
    // while we're actually trying to pair.
    bool pairing_mode_active_ = false;
    std::string device_name_;       //The name to be displayed for this authorization and used for storing preferences
    uint32_t device_id_;            //The ID of the Nuki App, Nuki Bridge or Nuki Fob to be authorized.

    //Keyturner Pairing Service
    const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_uuid_;
    //Keyturner Pairing Service Ultra
    const esphome::esp32_ble_tracker::ESPBTUUID pairing_service_ultra_uuid_;
    //Keyturner Service
    const esphome::esp32_ble_tracker::ESPBTUUID device_service_uuid_;
    //Keyturner pairing Data Input Output characteristic
    const esphome::esp32_ble_tracker::ESPBTUUID gdio_uuid_;
    //Keyturner pairing Data Input Output characteristic Ultra
    const esphome::esp32_ble_tracker::ESPBTUUID gdio_ultra_uuid_;
    //User-Specific Data Input Output characteristic
    const esphome::esp32_ble_tracker::ESPBTUUID user_data_uuid_;

    const std::string preferences_id_;

    CommandState nuki_command_state_ = CommandState::Idle;

    bool is_paired_ = false;

    SmartlockEventHandler* event_handler_;

    uint8_t received_status_;
    bool crc_check_oke_;

    unsigned char remote_public_key_[32] = {0x00};
    unsigned char challenge_nonce_k_[32] = {0x00};
    unsigned char authorization_id_[4] = {0x00};
    unsigned char my_public_key_[32] = {0x00};
    unsigned char my_private_key_[32] = {0x00};
    uint16_t pin_code_ = 0000;
    uint32_t ultra_pin_code_ = 000000;
    unsigned char secret_key_k_[32] = {0x00};

    unsigned char sent_nonce_[crypto_secretbox_NONCEBYTES] = {};

    uint16_t nr_of_keypad_codes_ = 0;
    uint8_t nr_of_received_keypad_codes_ = 0;
    bool keypad_code_count_received_ = false;
    uint16_t log_entry_count_ = 0;
    bool logging_enabled_ = false;
    uint16_t most_recent_command_ = 0;
    std::atomic_int rssi_;
    int64_t time_now_ = 0;
    std::atomic_llong last_heartbeat_;
    int64_t last_start_timeout_ = 0;
    int64_t pairing_last_seen_ = 0;
    std::atomic_llong last_received_beacon_ts_;

    // Entry count is known up front (lock sends a count notification before the entries
    // themselves), so a single allocation sized to that count avoids std::list's per-node
    // allocation churn on every retrieval.
    esphome::FixedVector<KeypadEntry> list_of_key_pad_entries_;
    // No count notification precedes fingerprint entries, so the final size isn't known
    // up front - std::vector still avoids std::list's per-node allocations.
    std::vector<FingerprintEntry> list_of_fingerprint_entries_;
    esphome::FixedVector<AuthorizationEntry> list_of_authorization_entries_;
    AuthorizationIdType authorization_id_type_ = AuthorizationIdType::Bridge;
};

// Template method definitions below (formerly NukiBle.hpp, merged in as part of consolidating
// the vendored Nuki library into normal ESPHome component file conventions). Templates must
// stay visible in the header for instantiation, so this can't move to nuki_ble.cpp.

// Separate from nuki_ble.cpp's own file-local TAG ("NukiBle") - this header is included by
// multiple .cpp files (each with their own TAG), and an unqualified TAG referenced from
// these template bodies would resolve per-translation-unit in a way that's not worth
// relying on. Declaring it here keeps it unambiguous regardless of which .cpp instantiates
// these templates.
static constexpr const char *const HPP_TAG = "NukiBle";

// Performs exactly one step of the command identified by `action` and returns
// immediately (no internal retry/wait loop). nuki_command_state (a member, shared with
// cmd_state_machine()/cmd_chall_state_machine()/cmd_chall_acc_state_machine()) keeps the command's
// progress between calls, so callers are expected to call this repeatedly (e.g. once per
// loop() tick) with the *same* action until it stops returning CmdResult::Working.
template<typename TDeviceAction>
CmdResult NukiBle::execute_action(const TDeviceAction action) {
  if(!this->is_paired_with_lock()) {
    if (this->debug_nuki_connect_) {
      ESP_LOGD(HPP_TAG, "Checking paired state");
    }
    if (this->retrieve_credentials()) {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(HPP_TAG, "Credentials retrieved from preferences, ready for commands");
      }
    } else {
      if (this->debug_nuki_connect_) {
        ESP_LOGD(HPP_TAG, "Credentials NOT retrieved from preferences, first pair with the lock");
      }
      return CmdResult::NotPaired;
    }
  }

  if (this->debug_nuki_communication_) {
    ESP_LOGD(HPP_TAG, "Start executing - command: %u", (unsigned int)action.command);
  }

  this->extend_disconnect_timeout();

  CmdResult result;
  if (action.cmdType == CommandType::Command) {
    result = this->cmd_state_machine(action);
  }
  else if (action.cmdType == CommandType::CommandWithChallenge) {
    result = this->cmd_chall_state_machine(action);
  }
  else if (action.cmdType == CommandType::CommandWithChallengeAndAccept) {
    result = this->cmd_chall_acc_state_machine(action);
  }
  else if (action.cmdType == CommandType::CommandWithChallengeAndPin) {
    result = this->cmd_chall_state_machine(action, true);
  }
  else {
    ESP_LOGW(HPP_TAG, "Unknown cmd type");
    this->disconnect();
    return CmdResult::Failed;
  }

  if (result != CmdResult::Working && (result == CmdResult::Error || result == CmdResult::Failed)) {
    this->disconnect();
  }
  return result;
}

template <typename TDeviceAction>
CmdResult NukiBle::cmd_state_machine(const TDeviceAction action) {
  this->extend_disconnect_timeout();
  switch (this->nuki_command_state_) {
    case CommandState::Idle: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Sending command: %u", (unsigned int)action.command);
      }
      this->last_msg_code_received_ = Command::Empty;

      if (this->send_encrypted_message(Command::RequestData, action.payload, action.payloadLen)) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::CmdSent;
      } else if (!(this->connected() && this->notify_registered_)) {
        // Connection not fully ready yet (covers the whole CONNECTING/CONNECTED/ESTABLISHED
        // window until notify-subscription confirms, not just CONNECTING) - retry next call
        // instead of disconnecting a connection attempt that's still legitimately in
        // progress (e.g. mid service discovery).
        return CmdResult::Working;
      } else {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Sending command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Command failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ != Command::ErrorReport && this->last_msg_code_received_ != Command::Empty) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command done");
        }
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Success;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ != 69) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ == 69) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command failed: lock busy");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Lock_Busy;
      }
    }
    break;
    default: {
      ESP_LOGW(HPP_TAG, "Unknown request command state");
      this->disconnect();
      return CmdResult::Failed;
      break;
    }
  }
  return CmdResult::Working;
}

template <typename TDeviceAction>
CmdResult NukiBle::cmd_chall_state_machine(const TDeviceAction action, const bool sendPinCode) {
  this->extend_disconnect_timeout();
  switch (this->nuki_command_state_) {
    case CommandState::Idle: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Sending challenge");
      }
      this->last_msg_code_received_ = Command::Empty;
      unsigned char payload[sizeof(Command)] = {0x04, 0x00};  //challenge

      if (this->send_encrypted_message(Command::RequestData, payload, sizeof(Command))) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::ChallengeSent;
      } else if (!(this->connected() && this->notify_registered_)) {
        // Connection not fully ready yet (covers the whole CONNECTING/CONNECTED/ESTABLISHED
        // window until notify-subscription confirms, not just CONNECTING) - retry next call
        // instead of disconnecting a connection attempt that's still legitimately in
        // progress (e.g. mid service discovery).
        return CmdResult::Working;
      } else {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Sending challenge failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      }
      break;
    }
    case CommandState::ChallengeSent: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Receiving challenge response");
      }
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Command failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ == Command::Challenge) {
        this->nuki_command_state_ = CommandState::ChallengeRespReceived;
        this->last_msg_code_received_ = Command::Empty;
      }
      break;
    }
    case CommandState::ChallengeRespReceived: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Sending command: %u", (unsigned int)action.command);
      }
      this->last_msg_code_received_ = Command::Empty;
      this->crc_check_oke_ = false;
      //add received challenge nonce to payload
      uint8_t payloadLen = action.payloadLen + sizeof(this->challenge_nonce_k_);
      if (sendPinCode) {
        if (this->is_lock_ultra()) {
          payloadLen = payloadLen + 4;
        } else {
          payloadLen = payloadLen + 2;
        }
      }
      unsigned char payload[payloadLen];
      memcpy(payload, action.payload, action.payloadLen);
      memcpy(&payload[action.payloadLen], this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_));
      if (sendPinCode) {
        if (this->is_lock_ultra()) {
          memcpy(&payload[action.payloadLen + sizeof(this->challenge_nonce_k_)], &this->ultra_pin_code_, 4);
        } else {
          memcpy(&payload[action.payloadLen + sizeof(this->challenge_nonce_k_)], &this->pin_code_, 2);
        }
      }

      if (this->send_encrypted_message(action.command, payload, payloadLen)) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::CmdSent;
      } else if (!(this->connected() && this->notify_registered_)) {
        // Connection not fully ready yet (covers the whole CONNECTING/CONNECTED/ESTABLISHED
        // window until notify-subscription confirms, not just CONNECTING) - retry next call
        // instead of disconnecting a connection attempt that's still legitimately in
        // progress (e.g. mid service discovery).
        return CmdResult::Working;
      } else {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Sending command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Receiving data");
      }
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Command failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ != 69) {
        if (this->debug_nuki_communication_) {
            ESP_LOGD(HPP_TAG, "Sending command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ == 69) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command failed: lock busy");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Lock_Busy;
      } else if (this->crc_check_oke_) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Data received");
        }
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::Success;
      }
      break;
    }
    default:
      ESP_LOGW(HPP_TAG, "Unknown request command state");
      this->disconnect();
      return CmdResult::Failed;
      break;
  }
  return CmdResult::Working;
}

template <typename TDeviceAction>
CmdResult NukiBle::cmd_chall_acc_state_machine(const TDeviceAction action) {
  this->extend_disconnect_timeout();
  switch (this->nuki_command_state_) {
    case CommandState::Idle: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Sending challenge");
      }
      this->last_msg_code_received_ = Command::Empty;
      unsigned char payload[sizeof(Command)] = {0x04, 0x00};  //challenge

      if (this->send_encrypted_message(Command::RequestData, payload, sizeof(Command))) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::ChallengeSent;
      } else if (!(this->connected() && this->notify_registered_)) {
        // Connection not fully ready yet (covers the whole CONNECTING/CONNECTED/ESTABLISHED
        // window until notify-subscription confirms, not just CONNECTING) - retry next call
        // instead of disconnecting a connection attempt that's still legitimately in
        // progress (e.g. mid service discovery).
        return CmdResult::Working;
      } else {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Sending challenge failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      }
      break;
    }
    case CommandState::ChallengeSent: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Receiving challenge response");
      }
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Command failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ == Command::Challenge) {
        this->nuki_command_state_ = CommandState::ChallengeRespReceived;
        this->last_msg_code_received_ = Command::Empty;
      }
      break;
    }
    case CommandState::ChallengeRespReceived: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Sending command: %u", (unsigned int)action.command);
      }
      this->last_msg_code_received_ = Command::Empty;
      //add received challenge nonce to payload
      uint8_t payloadLen = action.payloadLen + sizeof(this->challenge_nonce_k_);
      unsigned char payload[payloadLen];
      memcpy(payload, action.payload, action.payloadLen);
      memcpy(&payload[action.payloadLen], this->challenge_nonce_k_, sizeof(this->challenge_nonce_k_));

      if (this->send_encrypted_message(action.command, payload, action.payloadLen + sizeof(this->challenge_nonce_k_))) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::CmdSent;
      } else if (!(this->connected() && this->notify_registered_)) {
        // Connection not fully ready yet (covers the whole CONNECTING/CONNECTED/ESTABLISHED
        // window until notify-subscription confirms, not just CONNECTING) - retry next call
        // instead of disconnecting a connection attempt that's still legitimately in
        // progress (e.g. mid service discovery).
        return CmdResult::Working;
      } else {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Sending command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Receiving accept");
      }
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Accept failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ == Command::Status && (CommandStatus)this->received_status_ == CommandStatus::Accepted) {
        this->time_now_ = (esp_timer_get_time() / 1000);
        this->nuki_command_state_ = CommandState::CmdAccepted;
        this->last_msg_code_received_ = Command::Empty;
      } else if (this->last_msg_code_received_ == Command::Status && (CommandStatus)this->received_status_ == CommandStatus::Complete) {
        //accept was skipped on lock because ie unlock command when lock allready unlocked?
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command success (skipped)");
        }
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Success;
      }
      break;
    }
    case CommandState::CmdAccepted: {
      if (this->debug_nuki_communication_) {
        ESP_LOGD(HPP_TAG, "Receiving complete");
      }
      if ((esp_timer_get_time() / 1000) - this->time_now_ > this->command_timeout_duration_) {
        ESP_LOGW(HPP_TAG, "Command failed: timeout");
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        return CmdResult::TimeOut;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ != 69) {
        if (this->debug_nuki_communication_) {
            ESP_LOGD(HPP_TAG, "Sending command failed");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Failed;
      } else if (this->last_msg_code_received_ == Command::ErrorReport && this->error_code_ == 69) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command failed: lock busy");
        }
        this->disconnect();
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Lock_Busy;
      } else if ((CommandStatus)this->last_msg_code_received_ == CommandStatus::Complete) {
        if (this->debug_nuki_communication_) {
          ESP_LOGD(HPP_TAG, "Command success");
        }
        this->nuki_command_state_ = CommandState::Idle;
        this->last_msg_code_received_ = Command::Empty;
        return CmdResult::Success;
      }
      break;
    }
    default:
      ESP_LOGW(HPP_TAG, "Unknown request command state");
      this->disconnect();
      return CmdResult::Failed;
      break;
  }
  return CmdResult::Working;
}

}  // namespace esphome::nuki_lock