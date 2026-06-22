#pragma once
/**
 * @file NukiBle.h
 *
 * Created: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include "NimBLEDevice.h"
#include "NukiConstants.h"
#include "NukiDataTypes.h"

#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"
#include "BleInterfaces.h"

#include <sodium.h>

#include <esp_task_wdt.h>
#include "esp_log.h"
#include "esp_timer.h"

#include <atomic>
#include <list>
#include <cstdint>
#include <cstring>
#include <string>

#define PAIRING_TIMEOUT 30000
#define HEARTBEAT_TIMEOUT 30000

#ifdef CONFIG_IDF_TARGET_ESP32P4
typedef enum {
    ESP_PWR_LVL_N24 = 0,              /*!< Corresponding to -24 dBm */
    ESP_PWR_LVL_N21 = 1,              /*!< Corresponding to -21 dBm */
    ESP_PWR_LVL_N18 = 2,              /*!< Corresponding to -18 dBm */
    ESP_PWR_LVL_N15 = 3,              /*!< Corresponding to -15 dBm */
    ESP_PWR_LVL_N12 = 4,              /*!< Corresponding to -12 dBm */
    ESP_PWR_LVL_N9  = 5,              /*!< Corresponding to  -9 dBm */
    ESP_PWR_LVL_N6  = 6,              /*!< Corresponding to  -6 dBm */
    ESP_PWR_LVL_N3  = 7,              /*!< Corresponding to  -3 dBm */
    ESP_PWR_LVL_N0  = 8,              /*!< Corresponding to   0 dBm */
    ESP_PWR_LVL_P3  = 9,             /*!< Corresponding to  +3 dBm */
    ESP_PWR_LVL_P6  = 10,             /*!< Corresponding to  +6 dBm */
    ESP_PWR_LVL_P9  = 11,             /*!< Corresponding to  +9 dBm */
    ESP_PWR_LVL_P12 = 12,             /*!< Corresponding to  +12 dBm */
    ESP_PWR_LVL_P15 = 13,             /*!< Corresponding to  +15 dBm */
    ESP_PWR_LVL_P18 = 14,             /*!< Corresponding to  +18 dBm */
    ESP_PWR_LVL_P20 = 15,              /*!< Corresponding to  +20 dBm */
    ESP_PWR_LVL_P21 = 15,              /*!< Corresponding to  +20 dBm, this enum variable has been deprecated */
    ESP_PWR_LVL_INVALID = 0xFF,         /*!< Indicates an invalid value */
} esp_power_level_t;
#endif

namespace Nuki {
class NukiBle : public BLEClientCallbacks, public BleScanner::Subscriber {
  public:
    NukiBle(const std::string& deviceName,
            const uint32_t deviceId,
            const NimBLEUUID pairingServiceUUID,
            const NimBLEUUID pairingServiceUltraUUID,
            const NimBLEUUID deviceServiceUUID,
            const NimBLEUUID gdioUUID,
            const NimBLEUUID gdioUltraUUID,
            const NimBLEUUID userDataUUID,
            const std::string preferencedId);
    virtual ~NukiBle();

    /**
     * @brief Set the Event Handler object
     *
     * @param handler method to handle the event
     */
    void setEventHandler(Nuki::SmartlockEventHandler* handler);

    /**
     * @brief Checks if credentials are stored in preferences, if not initiate pairing
     *
     * @return
     */
    Nuki::PairingResult pairNuki(AuthorizationIdType idType = AuthorizationIdType::Bridge);

    /**
     * @brief Delete stored credentials
     */
    void unPairNuki();
    
    /**
     * @brief Reset BLE host
     */
    void resetHost();

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
    void updateConnectionState();

    /**
     * @brief Set the BLE Disconnect Timeout, if longer than ~20 sec the lock will disconnect by itself
     * if there is no BLE communication
     *
     * @param timeoutMs
     */
    void setDisconnectTimeout(uint32_t timeoutMs);

    /**
     * @brief Set the BLE General Timeout in milliseconds.
     *
     * @param timeoutMs
     */
    void setGeneralTimeout(uint32_t timeoutMs);
    
    /**
     * @brief Set the BLE Command Timeout in seconds.
     *
     * @param timeoutMs
     */
    void setCommandTimeout(uint32_t timeoutMs);
    
    /**
     * @brief Set the BLE Connect Timeout in seconds.
     *
     * @param timeout
     */
    void setConnectTimeout(uint8_t timeout);

    /**
     * @brief Set the BLE Connect number of retries.
     *
     * @param retries
     */
    void setConnectRetries(uint8_t retries);

    /**
     * @brief Returns pairing state (if credentials are stored or not)
     */
    const bool isPairedWithLock() const;
    
    /**
     * @brief Returns if BLE is pairing/paired/connected with a Smart Lock Ultra
     */
    const bool isLockUltra() const;

    /**
     * @brief Returns the log entry count. Only available after executing retreiveLogEntries.
     */
    uint16_t getLogEntryCount();

    /**
     * @brief Send a new keypad entry to the lock via BLE
     *
     * @param newKeypadEntry Nuki api based datatype to be sent
     * Keypad Codes that start with 12 are not allowed
     * 0 is not allowed
     * Duplicates are not allowed
     */
    Nuki::CmdResult addKeypadEntry(NewKeypadEntry newKeypadEntry);

    /**
     * @brief Send an updated keypad entry to the lock via BLE
     *
     * @param updatedKeypadEntry Nuki api based datatype to be sent
     * Keypad Codes that start with 12 are not allowed
     * 0 is not allowed
     * Duplicates are not allowed
     */
    Nuki::CmdResult updateKeypadEntry(UpdatedKeypadEntry updatedKeyPadEntry);

    /**
    * @brief Returns the keypad entry count.
    * Only available after executing retreiveKeypadEntries.
    */
    uint16_t getKeypadEntryCount();

    /**
     * @brief Request the lock via BLE to send the existing keypad entries
     *
     * @param offset The start offset to be read.
     * @param count The number of entries to be read, starting at the specified offset.
     */
    Nuki::CmdResult retrieveKeypadEntries(const uint16_t offset, const uint16_t count);

    /**
     * @brief Get the Keypad Entries stored on the esp (after executing retreieveLogKeypadEntries)
     *
     * @param requestedKeyPadEntries list to store the returned Keypad entries
     */
    void getKeypadEntries(std::list<KeypadEntry>* requestedKeyPadEntries);

    /**
     * @brief Request the lock via BLE to send the existing fingerprint entries
     *
     */
    Nuki::CmdResult retrieveFingerprintEntries();
    
    /**
     * @brief Get the Fingerprint Entries stored on the esp (after executing retrieveFingerprintEntries)
     *
     * @param requestedFingerprintEntries list to store the returned Fingerprint entries
     */
    void getFingerprintEntries(std::list<FingerprintEntry>* requestedFingerprintEntries);    

    /**
    * @brief Delete a Keypad Entry
    *
    * @param id Id to be deleted
    */
    Nuki::CmdResult deleteKeypadEntry(uint16_t id);
    
    /**
     * @brief Request the lock via BLE to send the existing authorizationentries
     *
     * @param offset The start offset to be read.
     * @param count The number of entries to be read, starting at the specified offset.
     */
    Nuki::CmdResult retrieveAuthorizationEntries(const uint16_t offset, const uint16_t count);

    /**
     * @brief Get the Authorization Entries stored on the esp (after executing retreiveAuthorizationEntries)
     *
     * @param requestedAuthorizationEntries list to store the returned Authorization entries
     */
    void getAuthorizationEntries(std::list<AuthorizationEntry>* requestedAuthorizationEntries);

    /**
     * @brief Sends a new authorization entry to the lock via BLE
     *
     * @param newAuthorizationEntry Nuki api based datatype to send
     */
    Nuki::CmdResult addAuthorizationEntry(NewAuthorizationEntry newAuthorizationEntry);

    /**
     * @brief Deletes the authorization entry from the lock
     *
     * @param id id to be deleted
     */
    Nuki::CmdResult deleteAuthorizationEntry(const uint32_t id);

    /**
     * @brief Sends an updated authorization entry to the lock via BLE
     *
     * @param updatedAuthorizationEntry Nuki api based datatype to send
     */
    Nuki::CmdResult updateAuthorizationEntry(UpdatedAuthorizationEntry updatedAuthorizationEntry);

    /**
     * @brief Sends an calibration (mechanical) request to the lock via BLE
     */
    Nuki::CmdResult requestCalibration();

    /**
     * @brief Sends an reboot request to the lock via BLE
     *
     */
    Nuki::CmdResult requestReboot();
    
    /**
     * @brief Sends a custom command to the lock via BLE
     *
     * @param command Nuki command to execute
     * @param withPin Set to true when using challenge and pin command
     */
    Nuki::CmdResult genericCommand(Command command, bool withPin = true);

    /**
     * @brief Sends a request for daily statistics to the lock via BLE
     *
     */
    Nuki::CmdResult requestDailyStatistics();

    /**
     * @brief Sends the time to be set to the lock via BLE
     *
     * @param time Nuki api based datatype to send
     */
    Nuki::CmdResult updateTime(TimeValue time);

    /**
     * @brief Saves the pincode on the esp. This pincode is used for sending/setting config via BLE to the lock
     * by other methods and needs to be the same pincode as stored in the lock
     *
     * @param pinCode
     * @return true if stored successfully
     */
    bool saveSecurityPincode(const uint16_t pinCode);
    bool saveUltraPincode(const uint32_t pinCode, bool save = true);

    /**
     * @brief Gets the pincode stored on the esp. This pincode is used for sending/setting config via BLE to the lock
     * by other methods and needs to be the same pincode as stored in the lock
     *
     * @return pincode
     */
    uint16_t getSecurityPincode();
    uint32_t getUltraPincode();

    /**
     * @brief Send the new pincode command to the lock via BLE
     * (this command uses the earlier by saveSecurityPincode() stored pincode which needs to be the same as
     * the pincode stored in the lock)
     *
     * @param newSecurityPin
     * @return Nuki::CmdResult
     */
    Nuki::CmdResult setSecurityPin(const uint16_t newSecurityPin);
    Nuki::CmdResult setUltraPin(const uint32_t newSecurityPin);

    /**
     * @brief Send the verify pincode command via BLE to the lock.
     * This command uses the earlier by saveSecurityPincode() stored pincode
     *
     * @return Nuki::CmdResult returns success when the pin code is correct (same as stored in the lock)
     */
    Nuki::CmdResult verifySecurityPin();

    /**
     * @brief Gets the ble mac address of the paired lock stored on the esp.
     *
     * @return 18 byte Char array with mac address
     */
    void getMacAddress(char* macAddress);

    /**
     * @brief Initializes stored preferences based on the devicename passed in the constructor,
     * creates the BLE client, sets the BLE callback and checks if the lock is paired
     * (if credentials are stored in preferences)
     */
    void initialize();

    /**
     * @brief the transmission power.
     * @param [in] powerLevel The power level to set, can be one of:
     * *   ESP_PWR_LVL_N12 = 0, Corresponding to -12dbm
     * *   ESP_PWR_LVL_N9  = 1, Corresponding to  -9dbm
     * *   ESP_PWR_LVL_N6  = 2, Corresponding to  -6dbm
     * *   ESP_PWR_LVL_N3  = 3, Corresponding to  -3dbm
     * *   ESP_PWR_LVL_N0  = 4, Corresponding to   0dbm
     * *   ESP_PWR_LVL_P3  = 5, Corresponding to  +3dbm
     * *   ESP_PWR_LVL_P6  = 6, Corresponding to  +6dbm
     * *   ESP_PWR_LVL_P9  = 7, Corresponding to  +9dbm
     */
    void setPower(esp_power_level_t powerLevel);

    /**
     * @brief Registers the BLE scanner to be used for scanning for advertisements from the lock.
     * BleScanner::Publisher is defined in dependent library https://github.com/I-Connect/BleScanner.git
     *
     * @param bleScanner the publisher of the BLE scanner
     */
    void registerBleScanner(BleScanner::Publisher* bleScanner);

    /**
    * @brief Returns the RSSI of the last received ble beacon broadcast
    *
    * @return RSSI value
    */
    int getRssi() const;

    /**
    * @brief Returns the timestamp in milliseconds when the last ble beacon has been received from the device
    *
    * @return Timestamp in milliseconds
    */
    int64_t getLastReceivedBeaconTs() const;

    /**
    * @brief Returns the BLE address of the device if paired.
    *
    * @return BLE address
    */
    const BLEAddress getBleAddress() const;

    /**
    * @brief Returns the timestamp (millis) of the last received BLE beacon from the lock.
    *
    * @return Last heartbeat value
    */
    int64_t getLastHeartbeat();

    /**
     * @brief Whether to enable or disable connect debug logging
     *
     * @param enable Set to true to enable connect debug logging
     */
    void setDebugConnect(bool enable);

    /**
     * @brief Whether to enable or disable communication debug logging
     *
     * @param enable Set to true to enable communication debug logging
     */
    void setDebugCommunication(bool enable);

    /**
     * @brief Whether to enable or disable readable data debug logging
     *
     * @param enable Set to true to enable readable data debug logging
     */
    void setDebugReadableData(bool enable);

    /**
     * @brief Whether to enable or disable hex data debug logging
     *
     * @param enable Set to true to enable hex data debug logging
     */
    void setDebugHexData(bool enable);

    /**
     * @brief Whether to enable or disable command debug logging
     *
     * @param enable Set to true to enable command debug logging
     */
    void setDebugCommand(bool enable);
    
  protected:
    bool connectBle(const BLEAddress bleAddress, bool pairing);
    void extendDisconnectTimeout();
    void logMessageVar(const char* message, unsigned int var, int level = 4);
    void logMessageVar(const char* message, const char* var, int level = 4);
    void logMessage(const char* message, int level = 4);

    template <typename TDeviceAction>
    Nuki::CmdResult executeAction(const TDeviceAction action);

    template <typename TDeviceAction>
    Nuki::CmdResult cmdStateMachine(const TDeviceAction action);

    template <typename TDeviceAction>
    Nuki::CmdResult cmdChallStateMachine(const TDeviceAction action, const bool sendPinCode = false);

    template <typename TDeviceAction>
    Nuki::CmdResult cmdChallAccStateMachine(const TDeviceAction action);

    virtual void handleReturnMessage(Command returnCode, unsigned char* data, uint16_t dataLen);
    virtual void logErrorCode(uint8_t errorCode) = 0;

    // Cannot initialize to any meaningful value since error namespaces are only
    // defined for NukeBle descendants. Using zero as a safe default, which should
    // work better than random for a general case.
    uint8_t errorCode = 0;

    Command lastMsgCodeReceived = Command::Empty;

    bool debugNukiConnect = false;
    bool debugNukiCommunication = false;
    bool debugNukiReadableData = false;
    bool debugNukiHexData = false;
    bool debugNukiCommand = false;

  private:
    bool connecting = false;
    bool disconnecting = false;
    bool statusUpdated = false;
    bool refreshServices = false;
    bool smartLockUltra = false;
    bool ultraAuthInfoCommandReceived = false;
    bool encryptPairing = false;
    bool recieveEncrypted = false;
    uint32_t timeoutDuration = 1000;
    uint32_t generalTimeoutDuration = 10000;
    uint32_t commandTimeoutDuration = 3000;    
    uint8_t connectTimeoutSec = 1;
    uint8_t connectRetries = 5;
    uint32_t countDisconnects = 0;

    void onConnect(BLEClient*) override;
    void onDisconnect(BLEClient*, int reason) override;
    void disconnect();
    void onResult(const BLEAdvertisedDevice* advertisedDevice) override;
    bool registerOnGdioChar();
    bool registerOnUsdioChar();

    bool sendPlainMessage(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen);
    bool sendEncryptedMessage(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen);

    NimBLERemoteCharacteristic::notify_callback callback;

    void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void saveCredentials();
    bool retrieveCredentials();
    void deleteCredentials();
    Nuki::PairingState pairStateMachine(const Nuki::PairingState nukiPairingState);
    Nuki::PairingState nukiPairingResultState = Nuki::PairingState::InitPairing;

    unsigned char authenticator[32];

    // Pairing credentials are persisted through ESPHome's own preferences/NVS
    // wrapper instead of a separate NVS namespace, so they participate in the
    // same batched flash writes (esphome::preferences) as the rest of ESPHome.
    template <typename T>
    esphome::ESPPreferenceObject makeCredentialPref(const char* keyName) {
      return esphome::global_preferences->make_preference<T>(esphome::fnv1_hash(preferencesId + "_" + keyName), true);
    }
    esphome::ESPPreferenceObject bleAddressPref;
    esphome::ESPPreferenceObject secretKeyPref;
    esphome::ESPPreferenceObject authIdPref;
    esphome::ESPPreferenceObject securityPincodePref;
    esphome::ESPPreferenceObject ultraPincodePref;
    esphome::ESPPreferenceObject isUltraPref;

    BLEAddress bleAddress = BLEAddress("", 0);
    bool pairingServiceAvailable = false;
    std::string deviceName;       //The name to be displayed for this authorization and used for storing preferences
    uint32_t deviceId;            //The ID of the Nuki App, Nuki Bridge or Nuki Fob to be authorized.
    BLEClient* pClient = nullptr;

    //Keyturner Pairing Service
    const NimBLEUUID pairingServiceUUID;
    //Keyturner Pairing Service Ultra
    const NimBLEUUID pairingServiceUltraUUID;
    //Keyturner Service
    const NimBLEUUID deviceServiceUUID;
    //Keyturner pairing Data Input Output characteristic
    const NimBLEUUID gdioUUID;
    //Keyturner pairing Data Input Output characteristic Ultra
    const NimBLEUUID gdioUltraUUID;
    //User-Specific Data Input Output characteristic
    const NimBLEUUID userDataUUID;

    const std::string preferencesId;

    BLERemoteService* pKeyturnerPairingService = nullptr;
    BLERemoteCharacteristic* pGdioCharacteristic = nullptr;
    BLERemoteService* pKeyturnerDataService = nullptr;
    BLERemoteCharacteristic* pUsdioCharacteristic = nullptr;

    Nuki::CommandState nukiCommandState = Nuki::CommandState::Idle;

    BleScanner::Publisher* bleScanner = nullptr;
    bool isPaired = false;

    Nuki::SmartlockEventHandler* eventHandler;

    uint8_t receivedStatus;
    bool crcCheckOke;

    unsigned char remotePublicKey[32] = {0x00};
    unsigned char challengeNonceK[32] = {0x00};
    unsigned char authorizationId[4] = {0x00};
    unsigned char myPublicKey[32] = {0x00};
    unsigned char myPrivateKey[32] = {0x00};
    uint16_t pinCode = 0000;
    uint32_t ultraPinCode = 000000;
    unsigned char secretKeyK[32] = {0x00};

    unsigned char sentNonce[crypto_secretbox_NONCEBYTES] = {};

    uint16_t nrOfKeypadCodes = 0;
    uint8_t nrOfReceivedKeypadCodes = 0;
    bool keypadCodeCountReceived = false;
    uint16_t logEntryCount = 0;
    bool loggingEnabled = false;
    std::atomic_int rssi;
    int64_t timeNow = 0;
    std::atomic_llong lastHeartbeat;
    int64_t lastStartTimeout = 0;
    int64_t pairingLastSeen = 0;
    std::atomic_llong lastReceivedBeaconTs;

    std::list<KeypadEntry> listOfKeyPadEntries;
    std::list<FingerprintEntry> listOfFingerprintEntries;
    std::list<AuthorizationEntry> listOfAuthorizationEntries;
    AuthorizationIdType authorizationIdType = AuthorizationIdType::Bridge;
};

} // namespace Nuki

#include "NukiBle.hpp"