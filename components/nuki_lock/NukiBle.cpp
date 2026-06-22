/**
 * @file NukiBle.cpp
 *
 * Created: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include "NukiBle.h"
#include "NukiLockUtils.h"
#include "NukiUtils.h"

#include <sodium.h>
#include "NimBLEBeacon.h"

#include <esp_task_wdt.h>
#include "esp_log.h"
#include "esp_timer.h"

#include <algorithm>
#include <atomic>
#include <list>
#include <cstdint>
#include <cstring>
#include <string>

namespace Nuki {

NukiBle::NukiBle(const std::string& deviceName,
                 const uint32_t deviceId,
                 const NimBLEUUID pairingServiceUUID,
                 const NimBLEUUID pairingServiceUltraUUID,
                 const NimBLEUUID deviceServiceUUID,
                 const NimBLEUUID gdioUUID,
                 const NimBLEUUID gdioUltraUUID,
                 const NimBLEUUID userDataUUID,
                 const std::string preferencedId)
  : deviceName(deviceName),
    deviceId(deviceId),
    pairingServiceUUID(pairingServiceUUID),
    pairingServiceUltraUUID(pairingServiceUltraUUID),
    deviceServiceUUID(deviceServiceUUID),
    gdioUUID(gdioUUID),
    gdioUltraUUID(gdioUltraUUID),
    userDataUUID(userDataUUID),
    preferencesId(preferencedId)
{
  rssi = 0;
  lastReceivedBeaconTs = 0;
  lastHeartbeat = 0;

  #ifdef DEBUG_NUKI_CONNECT
  debugNukiConnect = true;
  #endif
  #ifdef DEBUG_NUKI_COMMUNICATION
  debugNukiCommunication = true;
  #endif
  #ifdef DEBUG_NUKI_READABLE_DATA
  debugNukiReadableData = true;
  #endif
  #ifdef DEBUG_NUKI_HEX_DATA
  debugNukiHexData = true;
  #endif
  #ifdef DEBUG_NUKI_COMMAND
  debugNukiCommand = true;
  #endif
}

NukiBle::~NukiBle() {
  if (bleScanner != nullptr) {
    bleScanner->unsubscribe(this);
    bleScanner = nullptr;
  }
}

void NukiBle::initialize() {
  bleAddressPref = makeCredentialPref<uint8_t[6]>(BLE_ADDRESS_STORE_NAME);
  secretKeyPref = makeCredentialPref<uint8_t[32]>(SECRET_KEY_STORE_NAME);
  authIdPref = makeCredentialPref<uint8_t[4]>(AUTH_ID_STORE_NAME);
  securityPincodePref = makeCredentialPref<uint16_t>(SECURITY_PINCODE_STORE_NAME);
  ultraPincodePref = makeCredentialPref<uint32_t>(ULTRA_PINCODE_STORE_NAME);
  isUltraPref = makeCredentialPref<bool>(ULTRA_STORE_NAME);

  if (!NimBLEDevice::isInitialized())
  {
    NimBLEDevice::init(deviceName);
  }

  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(this);
  #if !defined(CONFIG_IDF_TARGET_ESP32C5)
  //DISABLE FOR ALL ESPS FOR NOW BASED ON ISSUES WITH C5 (2025-06-18)
  //pClient->setConnectionParams(12,12,0,600,64,64);
  #endif
  ESP_LOGD("NukiBle", "[%s] Connect timeout %d ms", deviceName.c_str(), connectTimeoutSec * 1000);
  pClient->setConnectTimeout(connectTimeoutSec * 1000);

  isPaired = retrieveCredentials();

  using namespace std::placeholders;
  callback = std::bind(&NukiBle::notifyCallback, this, _1, _2, _3, _4);
}

void NukiBle::setPower(esp_power_level_t powerLevel) {
  if (!NimBLEDevice::isInitialized()) {
    NimBLEDevice::init(deviceName);
  }

  int power = 9;

  switch(powerLevel)
  {
    case ESP_PWR_LVL_N12:
      power = -12;
      break;
    case ESP_PWR_LVL_N9:
      power = -9;
      break;
    case ESP_PWR_LVL_N6:
      power = -6;
      break;
    case ESP_PWR_LVL_N0:
      power = 0;
      break;
    case ESP_PWR_LVL_P3:
      power = 3;
      break;
    case ESP_PWR_LVL_P6:
      power = 6;
      break;
    case ESP_PWR_LVL_P9:
      power = 9;
      break;
    default:
      power = 9;
      break;
  }

  NimBLEDevice::setPower(power);
}

void NukiBle::registerBleScanner(BleScanner::Publisher* bleScanner) {
  this->bleScanner = bleScanner;
  bleScanner->subscribe(this);
}

PairingResult NukiBle::pairNuki(AuthorizationIdType idType) {
  authorizationIdType = idType;

  if (retrieveCredentials()) {
    if (debugNukiConnect) {
      ESP_LOGD("NukiBle", "Already paired");
    }
    isPaired = true;
    return PairingResult::Success;
  }
  PairingResult result = PairingResult::Pairing;

  if (pairingLastSeen < (esp_timer_get_time() / 1000) - 2000) pairingServiceAvailable = false;

  if (pairingServiceAvailable && bleAddress != BLEAddress("", 0)) {
    pairingServiceAvailable = false;
    if (debugNukiConnect) {
      ESP_LOGD("NukiBle", "Nuki in pairing mode found");
    }
    if (connectBle(bleAddress, true)) {
      crypto_box_keypair(myPublicKey, myPrivateKey);

      PairingState nukiPairingState = PairingState::InitPairing;
      do {
        nukiPairingState = pairStateMachine(nukiPairingState);
        extendDisconnectTimeout();
        vTaskDelay(pdMS_TO_TICKS(50));
      } while ((nukiPairingState != PairingState::Success) && (nukiPairingState != PairingState::Timeout));

      if (nukiPairingState == PairingState::Success) {
        saveCredentials();
        result = PairingResult::Success;
        lastHeartbeat = (esp_timer_get_time() / 1000);
      } else {
        result = PairingResult::Timeout;
      }
      extendDisconnectTimeout();
    }
  } else {
    if (debugNukiConnect) {
      ESP_LOGD("NukiBle", "No nuki in pairing mode found");
    }
  }

  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "pairing result %d", (unsigned int)result);
  }

  isPaired = (result == PairingResult::Success);
  return result;
}

void NukiBle::unPairNuki() {
  deleteCredentials();
  isPaired = false;
  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "[%s] Credentials deleted", deviceName.c_str());
  }
}

void NukiBle::resetHost() {
  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "[%s] Resetting BLE host", deviceName.c_str());
  }
  
  ble_hs_sched_reset(0);
}

bool NukiBle::connectBle(const BLEAddress bleAddress, bool pairing) {
  connecting = true;
  bleScanner->enableScanning(false);

  if (debugNukiConnect) {
    #if (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0))
    ESP_LOGD("NukiBle", "connecting within: %s", pcTaskGetTaskName(xTaskGetCurrentTaskHandle()));
    #else
    ESP_LOGD("NukiBle", "connecting within: %s", pcTaskGetName(xTaskGetCurrentTaskHandle()));
    #endif
  }

  uint8_t connectRetry = 0;

  while (connectRetry < connectRetries) {
    if(!pClient->isConnected()) {
      if (!pClient->connect(bleAddress, refreshServices)) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "[%s] Failed to connect", deviceName.c_str());
        }
        connectRetry++;
        #ifndef NUKI_NO_WDT_RESET
        esp_task_wdt_reset();
        #endif
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      } else {
        refreshServices = false;
      }
    }

    if (debugNukiConnect) {
      ESP_LOGD("NukiBle", "[%s] Connected to: %s RSSI: %d", deviceName.c_str(), pClient->getPeerAddress().toString().c_str(), pClient->getRssi());
    }

    if(pairing) {
      if (!registerOnGdioChar()) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "[%s] Failed to connect on registering GDIO", deviceName.c_str());
        }
        connectRetry++;
        #ifndef NUKI_NO_WDT_RESET
        esp_task_wdt_reset();
        #endif
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
    } else {
      if (!registerOnUsdioChar()) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "[%s] Failed to connect on registering USDIO", deviceName.c_str());
        }
        connectRetry++;
        #ifndef NUKI_NO_WDT_RESET
        esp_task_wdt_reset();
        #endif
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
    }

    bleScanner->enableScanning(true);
    connecting = false;
    return true;
  }

  bleScanner->enableScanning(true);
  connecting = false;
  return false;
}

void NukiBle::updateConnectionState() {
  if (connecting || disconnecting) {
    return;
  }

  if (lastStartTimeout != 0 && ((esp_timer_get_time() / 1000) - lastStartTimeout > timeoutDuration)) {
    if (pClient) {
      if (pClient->isConnected()) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "disconnecting BLE on timeout");
        }

        disconnect();
      }
    }

    lastStartTimeout = 0;
  }
}

void NukiBle::disconnect()
{
  if (disconnecting) {
    return;
  }

  disconnecting = true;

  if (pGdioCharacteristic != nullptr) {
    pGdioCharacteristic->unsubscribe(false);
  }

  pGdioCharacteristic = nullptr;
  pKeyturnerPairingService = nullptr;

  if (pUsdioCharacteristic != nullptr) {
    pUsdioCharacteristic->unsubscribe(false);
  }

  pUsdioCharacteristic = nullptr;
  pKeyturnerDataService = nullptr;


  if (pClient) {
    if (pClient->isConnected()) {
      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "Disconnecting BLE");
      }

      countDisconnects++;
      pClient->disconnect();
      int loop = 0;

      while ((countDisconnects > 0 || pClient->isConnected()) && loop < 50) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", ".");
        }
        loop++;
        vTaskDelay(pdMS_TO_TICKS(100));
      }

      if (pClient->isConnected())
      {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "Error while disconnecting BLE client");
        }
        eventHandler->notify(EventType::BLE_ERROR_ON_DISCONNECT);
      }
    }
  }

  disconnecting = false;
}

void NukiBle::setDisconnectTimeout(uint32_t timeoutMs) {
  timeoutDuration = timeoutMs;
}

void NukiBle::setConnectTimeout(uint8_t timeout) {
  connectTimeoutSec = timeout;
}

void NukiBle::setGeneralTimeout(uint32_t timeoutMs) {
  generalTimeoutDuration = timeoutMs;
}

void NukiBle::setCommandTimeout(uint32_t timeoutMs) {
  commandTimeoutDuration = timeoutMs;
}

void NukiBle::setConnectRetries(uint8_t retries) {
  connectRetries = retries;
}

void NukiBle::extendDisconnectTimeout() {
  lastStartTimeout = (esp_timer_get_time() / 1000);
  lastHeartbeat = (esp_timer_get_time() / 1000);
}

void NukiBle::onResult(const BLEAdvertisedDevice* advertisedDevice) {
  if (isPaired) {
    if (bleAddress == advertisedDevice->getAddress()) {
      rssi = advertisedDevice->getRSSI();
      lastReceivedBeaconTs = (esp_timer_get_time() / 1000);

      std::string manufacturerData = advertisedDevice->getManufacturerData();
      uint8_t* manufacturerDataPtr = (uint8_t*)manufacturerData.data();
      bool isKeyTurnerUUID = true;
      std::string serviceUUID = deviceServiceUUID.toString();

      std::string pHex = advertisedDevice->toString();
      serviceUUID.erase(std::remove(serviceUUID.begin(), serviceUUID.end(), '-'), serviceUUID.end());

      if (pHex.find(serviceUUID) == std::string::npos) {
        isKeyTurnerUUID = false;
      }

      if (isKeyTurnerUUID) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "Nuki Advertising: %s", advertisedDevice->toString().c_str());
        }

        uint8_t cManufacturerData[100];
        manufacturerData.copy((char*)cManufacturerData, manufacturerData.length(), 0);

        if (manufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
          BLEBeacon oBeacon = BLEBeacon();

          oBeacon.setData((uint8_t*)manufacturerData.c_str(), (uint8_t)manufacturerData.length());
          if (debugNukiConnect) {
            ESP_LOGD("NukiBle", "iBeacon ID: %04X Major: %d Minor: %d UUID: %s Power: %d", oBeacon.getManufacturerId(),
                  ENDIAN_CHANGE_U16(oBeacon.getMajor()), ENDIAN_CHANGE_U16(oBeacon.getMinor()),
                  oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower());
          }

          lastHeartbeat = (esp_timer_get_time() / 1000);

          if ((oBeacon.getSignalPower() & 0x01) > 0) {
            if (eventHandler) {
              eventHandler->notify(EventType::KeyTurnerStatusUpdated);
            }

            statusUpdated = true;
          }
          else if (statusUpdated)
          {
            statusUpdated = false;

            if (eventHandler) {
              eventHandler->notify(EventType::KeyTurnerStatusReset);
            }
          }
        }
      }
    }
  } else {
    if (advertisedDevice->haveServiceData()) {
      if (advertisedDevice->getServiceData(pairingServiceUUID) != "") {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "Found nuki in pairing state: %s addr: %s", std::string(advertisedDevice->getName()).c_str(), std::string(advertisedDevice->getAddress()).c_str());
        }
        bleAddress = advertisedDevice->getAddress();
        pairingServiceAvailable = true;
        smartLockUltra = false;
        pairingLastSeen = (esp_timer_get_time() / 1000);
      } else if (advertisedDevice->getServiceData(pairingServiceUltraUUID) != "") {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "Found nuki ultra in pairing state: %s addr: %s", std::string(advertisedDevice->getName()).c_str(), std::string(advertisedDevice->getAddress()).c_str());
        }

        if (ultraPinCode == 000000) {
          ESP_LOGD("NukiBle", "No pairing PIN code set, not pairing with Nuki SmartLock Ultra");
        } else {
          bleAddress = advertisedDevice->getAddress();
          pairingServiceAvailable = true;
          smartLockUltra = true;
          pairingLastSeen = (esp_timer_get_time() / 1000);
        }
      }
    }
  }
}

Nuki::CmdResult NukiBle::retrieveKeypadEntries(const uint16_t offset, const uint16_t count) {
  NukiLock::Action action;
  unsigned char payload[4] = {0};
  memcpy(payload, &offset, 2);
  memcpy(&payload[2], &count, 2);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestKeypadCodes;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  listOfKeyPadEntries.clear();
  nrOfReceivedKeypadCodes = 0;
  keypadCodeCountReceived = false;

  int64_t timeNow = (esp_timer_get_time() / 1000);
  Nuki::CmdResult result = executeAction(action);

  if (result == Nuki::CmdResult::Success) {
    //wait for return of Keypad Code Count (0x0044)
    while (!keypadCodeCountReceived) {
      if ((esp_timer_get_time() / 1000) - timeNow > generalTimeoutDuration) {
        ESP_LOGW("NukiBle", "Receive keypad count timeout");
        disconnect();
        return CmdResult::TimeOut;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (debugNukiCommand) {
      ESP_LOGD("NukiBle", "Keypad code count %d", getKeypadEntryCount());
    }

    //wait for return of Keypad Codes (0x0045)
    timeNow = (esp_timer_get_time() / 1000);
    while (nrOfReceivedKeypadCodes < getKeypadEntryCount()) {
      if ((esp_timer_get_time() / 1000) - timeNow > generalTimeoutDuration) {
        ESP_LOGW("NukiBle", "Receive keypadcodes timeout");
        disconnect();
        return CmdResult::TimeOut;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (debugNukiCommand) {
      ESP_LOGD("NukiBle", "%d codes received", nrOfReceivedKeypadCodes);
    }
  } else {
    ESP_LOGW("NukiBle", "Retrieve keypad codes from lock failed");
  }
  return result;
}

Nuki::CmdResult NukiBle::addKeypadEntry(NewKeypadEntry newKeypadEntry) {
  //TODO verify data validity, ie check for invalid chars in name
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::AddKeypadCode;
  memcpy(action.payload, &newKeypadEntry, sizeof(NewKeypadEntry));
  action.payloadLen = sizeof(NewKeypadEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "addKeyPadEntry, payloadlen: %d", sizeof(NewKeypadEntry));
      printBuffer(action.payload, sizeof(NewKeypadEntry), false, "addKeyPadCode content: ", debugNukiHexData);
      NukiLock::logNewKeypadEntry(newKeypadEntry, debugNukiReadableData);
    }
  }
  return result;
}

Nuki::CmdResult NukiBle::genericCommand(Command command, bool withPin) {
  NukiLock::Action action;

  if (withPin) {
    action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  } else {
    action.cmdType = Nuki::CommandType::CommandWithChallenge;
  }
  action.command = command;

  Nuki::CmdResult result = executeAction(action);
  return result;
}

Nuki::CmdResult NukiBle::requestDailyStatistics() {
  NukiLock::Action action;
  unsigned char payload[5] = {0};
  payload[0] = 0;
  payload[1] = 0;
  payload[2] = 0;
  payload[3] = 0;
  payload[4] = 5;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Nuki::Command::RequestDailyStatistics;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  Nuki::CmdResult result = executeAction(action);
  return result;
}

Nuki::CmdResult NukiBle::updateKeypadEntry(UpdatedKeypadEntry updatedKeyPadEntry) {
  //TODO verify data validity
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateKeypadCode;
  memcpy(action.payload, &updatedKeyPadEntry, sizeof(UpdatedKeypadEntry));
  action.payloadLen = sizeof(UpdatedKeypadEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "addKeyPadEntry, payloadlen: %d", sizeof(UpdatedKeypadEntry));
      printBuffer(action.payload, sizeof(UpdatedKeypadEntry), false, "updatedKeypad content: ", debugNukiHexData);
      NukiLock::logUpdatedKeypadEntry(updatedKeyPadEntry, debugNukiReadableData);
    }
  }
  return result;
}

void NukiBle::getFingerprintEntries(std::list<FingerprintEntry>* requestedFingerprintEntries) {
  requestedFingerprintEntries->clear();
  std::list<FingerprintEntry>::iterator it = listOfFingerprintEntries.begin();
  while (it != listOfFingerprintEntries.end()) {
    requestedFingerprintEntries->push_back(*it);
    it++;
  }
}

void NukiBle::getKeypadEntries(std::list<KeypadEntry>* requestedKeypadCodes) {
  requestedKeypadCodes->clear();
  std::list<KeypadEntry>::iterator it = listOfKeyPadEntries.begin();
  while (it != listOfKeyPadEntries.end()) {
    requestedKeypadCodes->push_back(*it);
    it++;
  }
}

uint16_t NukiBle::getKeypadEntryCount() {
  return nrOfKeypadCodes;
}

CmdResult NukiBle::deleteKeypadEntry(uint16_t id) {
  NukiLock::Action action;
  unsigned char payload[2] = {0};
  memcpy(payload, &id, 2);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveKeypadCode;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return executeAction(action);
}

Nuki::CmdResult NukiBle::retrieveFingerprintEntries() {
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestFingerprintEntries;

  listOfFingerprintEntries.clear();

  return executeAction(action);
}

Nuki::CmdResult NukiBle::retrieveAuthorizationEntries(const uint16_t offset, const uint16_t count) {
  NukiLock::Action action;
  unsigned char payload[4] = {0};
  memcpy(payload, &offset, 2);
  memcpy(&payload[2], &count, 2);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestAuthorizationEntries;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  listOfAuthorizationEntries.clear();

  return executeAction(action);
}

void NukiBle::getAuthorizationEntries(std::list<AuthorizationEntry>* requestedAuthorizationEntries) {
  requestedAuthorizationEntries->clear();
  std::list<AuthorizationEntry>::iterator it = listOfAuthorizationEntries.begin();
  while (it != listOfAuthorizationEntries.end()) {
    requestedAuthorizationEntries->push_back(*it);
    it++;
  }
}

Nuki::CmdResult NukiBle::addAuthorizationEntry(NewAuthorizationEntry newAuthorizationEntry) {
  //TODO verify data validity
  NukiLock::Action action;
  unsigned char payload[sizeof(NewAuthorizationEntry)] = {0};
  memcpy(payload, &newAuthorizationEntry, sizeof(NewAuthorizationEntry));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::AuthorizationDatInvite;
  memcpy(action.payload, &payload, sizeof(NewAuthorizationEntry));
  action.payloadLen = sizeof(NewAuthorizationEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "addAuthorizationEntry, payloadlen: %d", sizeof(NewAuthorizationEntry));
      printBuffer(action.payload, sizeof(NewAuthorizationEntry), false, "addAuthorizationEntry content: ", debugNukiHexData);
      NukiLock::logNewAuthorizationEntry(newAuthorizationEntry, debugNukiReadableData);
    }
  }
  return result;
}

Nuki::CmdResult NukiBle::deleteAuthorizationEntry(uint32_t id) {
  NukiLock::Action action;
  unsigned char payload[4] = {0};
  memcpy(payload, &id, 4);

  action.cmdType = CommandType::CommandWithChallengeAndPin;
  action.command = Command::RemoveUserAuthorization;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  return executeAction(action);
}

Nuki::CmdResult NukiBle::updateAuthorizationEntry(UpdatedAuthorizationEntry updatedAuthorizationEntry) {
  //TODO verify data validity
  NukiLock::Action action;
  unsigned char payload[sizeof(UpdatedAuthorizationEntry)] = {0};
  memcpy(payload, &updatedAuthorizationEntry, sizeof(UpdatedAuthorizationEntry));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateAuthorization;
  memcpy(action.payload, &payload, sizeof(UpdatedAuthorizationEntry));
  action.payloadLen = sizeof(UpdatedAuthorizationEntry);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "addAuthorizationEntry, payloadlen: %d", sizeof(UpdatedAuthorizationEntry));
      printBuffer(action.payload, sizeof(UpdatedAuthorizationEntry), false, "updatedKeypad content: ", debugNukiHexData);
      NukiLock::logUpdatedAuthorizationEntry(updatedAuthorizationEntry, debugNukiReadableData);
    }
  }
  return result;
}

uint16_t NukiBle::getLogEntryCount() {
  return logEntryCount;
}

Nuki::CmdResult NukiBle::setSecurityPin(const uint16_t newSecurityPin) {
  NukiLock::Action action;
  unsigned char payload[2] = {0};
  memcpy(payload, &newSecurityPin, 2);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetSecurityPin;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    pinCode = newSecurityPin;
    saveCredentials();
  }
  return result;
}

Nuki::CmdResult NukiBle::setUltraPin(const uint32_t newSecurityPin) {
  NukiLock::Action action;
  unsigned char payload[4] = {0};
  memcpy(payload, &newSecurityPin, 4);

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::SetSecurityPin;
  memcpy(action.payload, &payload, sizeof(payload));
  action.payloadLen = sizeof(payload);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    ultraPinCode = newSecurityPin;
    saveCredentials();
  }
  return result;
}

Nuki::CmdResult NukiBle::verifySecurityPin() {
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::VerifySecurityPin;
  action.payloadLen = 0;

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "Verify security pin code success");
    }
  }
  return result;
}

Nuki::CmdResult NukiBle::requestCalibration() {
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestCalibration;
  action.payloadLen = 0;

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "Calibration executed");
    }
  }
  return result;
}

Nuki::CmdResult NukiBle::requestReboot() {
  NukiLock::Action action;

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::RequestReboot;
  action.payloadLen = 0;

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "Reboot executed");
    }
  }
  return result;
}

Nuki::CmdResult NukiBle::updateTime(TimeValue time) {
  NukiLock::Action action;
  unsigned char payload[sizeof(TimeValue)] = {0};
  memcpy(payload, &time, sizeof(TimeValue));

  action.cmdType = Nuki::CommandType::CommandWithChallengeAndPin;
  action.command = Command::UpdateTime;
  memcpy(action.payload, &payload, sizeof(TimeValue));
  action.payloadLen = sizeof(TimeValue);

  Nuki::CmdResult result = executeAction(action);
  if (result == Nuki::CmdResult::Success) {
    if (debugNukiReadableData) {
      ESP_LOGD("NukiBle", "Time set: %d-%d-%d %d:%d:%d", time.year, time.month, time.day, time.hour, time.minute, time.second);
    }
  }
  return result;
}

bool NukiBle::saveSecurityPincode(const uint16_t pinCode) {
  if (!securityPincodePref.save(&pinCode) || !esphome::global_preferences->sync()) {
    ESP_LOGE("NukiBle", "ERROR: saveSecurityPincode failed");
    return false;
  }

  this->pinCode = pinCode;
  return true;
}

bool NukiBle::saveUltraPincode(const uint32_t pinCode, bool save) {
  if (save) {
    if (!ultraPincodePref.save(&pinCode) || !esphome::global_preferences->sync()) {
      ESP_LOGE("NukiBle", "ERROR: saveUltraPincode failed");
      return false;
    }
  }

  this->ultraPinCode = pinCode;
  return true;
}

void NukiBle::saveCredentials() {
  unsigned char currentBleAddress[6];
  unsigned char storedBleAddress[6];
  uint16_t defaultPincode = 0;
  currentBleAddress[0] = bleAddress.getVal()[5];
  currentBleAddress[1] = bleAddress.getVal()[4];
  currentBleAddress[2] = bleAddress.getVal()[3];
  currentBleAddress[3] = bleAddress.getVal()[2];
  currentBleAddress[4] = bleAddress.getVal()[1];
  currentBleAddress[5] = bleAddress.getVal()[0];

  bleAddressPref.load(&storedBleAddress);

  bool isUltraLock = isLockUltra();
  isUltraPref.save(&isUltraLock);

  if (isLockUltra()) {
    ultraPincodePref.save(&ultraPinCode);
  } else {
    if (compareCharArray(currentBleAddress, storedBleAddress, 6)) {
      //only store earlier retreived pin code if address is the same
      //otherwise it is a different/new lock
      securityPincodePref.save(&pinCode);
    } else {
      securityPincodePref.save(&defaultPincode);
    }
  }

  if (bleAddressPref.save(&currentBleAddress)
      && secretKeyPref.save(&secretKeyK)
      && authIdPref.save(&authorizationId)
      && esphome::global_preferences->sync()
    ) {
    if (debugNukiConnect) {
      ESP_LOGD("NukiBle", "Credentials saved:");
      printBuffer(secretKeyK, sizeof(secretKeyK), false, SECRET_KEY_STORE_NAME, debugNukiHexData);
      printBuffer(currentBleAddress, 6, false, BLE_ADDRESS_STORE_NAME, debugNukiHexData);
      printBuffer(authorizationId, sizeof(authorizationId), false, AUTH_ID_STORE_NAME, debugNukiHexData);

      if (isLockUltra()) {
        ESP_LOGD("NukiBle", "pincode: %d", (unsigned int)ultraPinCode);
      } else {
        ESP_LOGD("NukiBle", "pincode: %d", pinCode);
      }
    }
  } else {
    logMessage("ERROR saving credentials", 1);
  }
}

uint16_t NukiBle::getSecurityPincode() {
  uint16_t storedPincode = 0000;
  if (securityPincodePref.load(&storedPincode)) {
    return storedPincode;
  }
  return 0;
}

uint32_t NukiBle::getUltraPincode() {
  uint32_t storedPincode = 000000;
  if (ultraPincodePref.load(&storedPincode)) {
    return storedPincode;
  }
  return 0;
}

void NukiBle::getMacAddress(char* macAddress) {
  unsigned char buf[6];
  if (bleAddressPref.load(&buf)) {
    BLEAddress address = BLEAddress(buf, 0);
    sprintf(macAddress, "%s", address.toString().c_str());
  }
}

bool NukiBle::retrieveCredentials() {
  //TODO check on empty (invalid) credentials?
  unsigned char buff[6];

  if (bleAddressPref.load(&buff)
    && secretKeyPref.load(&secretKeyK)
    && authIdPref.load(&authorizationId)
   ) {
    bleAddress = BLEAddress(buff, 0);

    if (debugNukiConnect) {
      ESP_LOGI("NukiBle", "[%s] Credentials retrieved:", deviceName.c_str());
      printBuffer(secretKeyK, sizeof(secretKeyK), false, SECRET_KEY_STORE_NAME, debugNukiHexData);
      ESP_LOGD("NukiBle", "bleAddress: %s", bleAddress.toString().c_str());
      printBuffer(authorizationId, sizeof(authorizationId), false, AUTH_ID_STORE_NAME, debugNukiHexData);
    }

    if (isCharArrayEmpty(secretKeyK, sizeof(secretKeyK)) || isCharArrayEmpty(authorizationId, sizeof(authorizationId))) {
      ESP_LOGW("NukiBle", "secret key OR authorizationId is empty: not paired");
      return false;
    }

    bool isUltraLock = false;
    isUltraPref.load(&isUltraLock);
    smartLockUltra = isUltraLock;

    if (isLockUltra()) {
      ultraPincodePref.load(&ultraPinCode);

      if (ultraPinCode == 0) {
        ESP_LOGW("NukiBle", "Pincode is 000000, probably not defined");
      }
    } else {
      securityPincodePref.load(&pinCode);

      if (pinCode == 0) {
        ESP_LOGW("NukiBle", "Pincode is 0000, probably not defined");
      }
    }
  } else {
    ESP_LOGE("NukiBle", "No credentials found - not paired yet!");
    return false;
  }
  return true;
}

void NukiBle::deleteCredentials() {
  unsigned char emptySecretKeyK[32] = {0x00};
  unsigned char emptyAuthorizationId[4] = {0x00};
  bool isUltraLock = false;
  secretKeyPref.save(&emptySecretKeyK);
  authIdPref.save(&emptyAuthorizationId);
  isUltraPref.save(&isUltraLock);
  esphome::global_preferences->sync();

  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "Credentials deleted");
  }
}

PairingState NukiBle::pairStateMachine(const PairingState nukiPairingState) {
  switch (nukiPairingState) {
    case PairingState::InitPairing: {
      memset(challengeNonceK, 0, sizeof(challengeNonceK));
      memset(remotePublicKey, 0, sizeof(remotePublicKey));
      receivedStatus = 0xff;
      timeNow = (esp_timer_get_time() / 1000);
      nukiPairingResultState = PairingState::ReqRemPubKey;
    }
    case PairingState::ReqRemPubKey: {
      //Request remote public key (Sent message should be 0100030027A7)
      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "##################### REQUEST REMOTE PUBLIC KEY #########################");
      }
      unsigned char buff[sizeof(Command)];
      uint16_t cmd = (uint16_t)Command::PublicKey;
      memcpy(buff, &cmd, sizeof(Command));
      sendPlainMessage(Command::RequestData, buff, sizeof(Command));
      nukiPairingResultState = PairingState::RecRemPubKey;
    }
    case PairingState::RecRemPubKey: {
      if (isCharArrayNotEmpty(remotePublicKey, sizeof(remotePublicKey))) {
        nukiPairingResultState = PairingState::SendPubKey;
      }
      break;
    }
    case PairingState::SendPubKey: {
      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "##################### SEND CLIENT PUBLIC KEY #########################");
      }
      sendPlainMessage(Command::PublicKey, myPublicKey, sizeof(myPublicKey));
      nukiPairingResultState = PairingState::GenKeyPair;
    }
    case PairingState::GenKeyPair: {
      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "##################### CALCULATE DH SHARED KEY s #########################");
      }
      unsigned char sharedKeyS[32] = {0x00};
      crypto_scalarmult_curve25519(sharedKeyS, myPrivateKey, remotePublicKey);
      printBuffer(sharedKeyS, sizeof(sharedKeyS), false, "Shared key s", debugNukiHexData);

      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "##################### DERIVE LONG TERM SHARED SECRET KEY k #########################");
      }
      unsigned char in[16];
      memset(in, 0, 16);
      unsigned char sigma[] = "expand 32-byte k";
      crypto_core_hsalsa20(secretKeyK, in, sharedKeyS, sigma);
      printBuffer(secretKeyK, sizeof(secretKeyK), false, "Secret key k", debugNukiHexData);
      nukiPairingResultState = PairingState::CalculateAuth;
    }
    case PairingState::CalculateAuth: {
      if (isCharArrayNotEmpty(challengeNonceK, sizeof(challengeNonceK))) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "##################### CALCULATE/VERIFY AUTHENTICATOR #########################");
        }
        //concatenate local public key, remote public key and receive challenge data
        unsigned char hmacPayload[96];
        memcpy(&hmacPayload[0], myPublicKey, sizeof(myPublicKey));
        memcpy(&hmacPayload[32], remotePublicKey, sizeof(remotePublicKey));
        memcpy(&hmacPayload[64], challengeNonceK, sizeof(challengeNonceK));
        printBuffer((uint8_t*)hmacPayload, sizeof(hmacPayload), false, "Concatenated data r", debugNukiHexData);
        crypto_auth_hmacsha256(authenticator, hmacPayload, sizeof(hmacPayload), secretKeyK);
        printBuffer(authenticator, sizeof(authenticator), false, "HMAC 256 result", debugNukiHexData);
        memset(challengeNonceK, 0, sizeof(challengeNonceK));
        nukiPairingResultState = PairingState::SendAuth;
      }
      break;
    }
    case PairingState::SendAuth: {
      if (debugNukiConnect) {
        ESP_LOGD("NukiBle", "##################### SEND AUTHENTICATOR #########################");
      }
      sendPlainMessage(Command::AuthorizationAuthenticator, authenticator, sizeof(authenticator));
      ultraAuthInfoCommandReceived = false;
      nukiPairingResultState = PairingState::SendAuthData;
    }
    case PairingState::SendAuthData: {
      if (isLockUltra()) {
        if (ultraAuthInfoCommandReceived) {
          ultraAuthInfoCommandReceived = false;

          if (debugNukiConnect) {
            ESP_LOGD("NukiBle", "##################### SEND AUTHORIZATION DATA (ULTRA) #########################");
          }

          unsigned char authorizationDataId[4] = {};
          unsigned char authorizationDataName[32] = {};
          authorizationDataId[0] = (deviceId >> (8 * 0)) & 0xff;
          authorizationDataId[1] = (deviceId >> (8 * 1)) & 0xff;
          authorizationDataId[2] = (deviceId >> (8 * 2)) & 0xff;
          authorizationDataId[3] = (deviceId >> (8 * 3)) & 0xff;
          memcpy(authorizationDataName, deviceName.c_str(), deviceName.size());

          //compose and send message
          unsigned char authorizationDataMessage[40];
          memcpy(&authorizationDataMessage[0], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationDataMessage[4], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationDataMessage[36], &ultraPinCode, 4);

          encryptPairing = true;
          sendEncryptedMessage(Command::AuthorizationData, authorizationDataMessage, sizeof(authorizationDataMessage));
          nukiPairingResultState = PairingState::RecStatus;
        }
      } else {
        if (isCharArrayNotEmpty(challengeNonceK, sizeof(challengeNonceK))) {
          if (debugNukiConnect) {
            ESP_LOGD("NukiBle", "##################### SEND AUTHORIZATION DATA #########################");
          }
          unsigned char authorizationData[101] = {};
          unsigned char authorizationDataIdType[1] = {(unsigned char)authorizationIdType };
          unsigned char authorizationDataId[4] = {};
          unsigned char authorizationDataName[32] = {};
          unsigned char authorizationDataNonce[32] = {};
          authorizationDataId[0] = (deviceId >> (8 * 0)) & 0xff;
          authorizationDataId[1] = (deviceId >> (8 * 1)) & 0xff;
          authorizationDataId[2] = (deviceId >> (8 * 2)) & 0xff;
          authorizationDataId[3] = (deviceId >> (8 * 3)) & 0xff;
          memcpy(authorizationDataName, deviceName.c_str(), deviceName.size());
          generateNonce(authorizationDataNonce, sizeof(authorizationDataNonce), debugNukiHexData);

          //calculate authenticator of message to send
          memcpy(&authorizationData[0], authorizationDataIdType, sizeof(authorizationDataIdType));
          memcpy(&authorizationData[1], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationData[5], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationData[37], authorizationDataNonce, sizeof(authorizationDataNonce));
          memcpy(&authorizationData[69], challengeNonceK, sizeof(challengeNonceK));
          crypto_auth_hmacsha256(authenticator, authorizationData, sizeof(authorizationData), secretKeyK);

          //compose and send message
          unsigned char authorizationDataMessage[101];
          memcpy(&authorizationDataMessage[0], authenticator, sizeof(authenticator));
          memcpy(&authorizationDataMessage[32], authorizationDataIdType, sizeof(authorizationDataIdType));
          memcpy(&authorizationDataMessage[33], authorizationDataId, sizeof(authorizationDataId));
          memcpy(&authorizationDataMessage[37], authorizationDataName, sizeof(authorizationDataName));
          memcpy(&authorizationDataMessage[69], authorizationDataNonce, sizeof(authorizationDataNonce));

          memset(challengeNonceK, 0, sizeof(challengeNonceK));
          sendPlainMessage(Command::AuthorizationData, authorizationDataMessage, sizeof(authorizationDataMessage));
          nukiPairingResultState = PairingState::SendAuthIdConf;
        }
      }
      break;
    }
    case PairingState::SendAuthIdConf: {
      if (isCharArrayNotEmpty(authorizationId, sizeof(authorizationId))) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "##################### SEND AUTHORIZATION ID confirmation #########################");
        }
        unsigned char confirmationData[36] = {};

        //calculate authenticator of message to send
        memcpy(&confirmationData[0], authorizationId, sizeof(authorizationId));
        memcpy(&confirmationData[4], challengeNonceK, sizeof(challengeNonceK));
        crypto_auth_hmacsha256(authenticator, confirmationData, sizeof(confirmationData), secretKeyK);

        //compose and send message
        unsigned char confirmationDataMessage[36];
        memcpy(&confirmationDataMessage[0], authenticator, sizeof(authenticator));
        memcpy(&confirmationDataMessage[32], authorizationId, sizeof(authorizationId));
        sendPlainMessage(Command::AuthorizationIdConfirmation, confirmationDataMessage, sizeof(confirmationDataMessage));
        nukiPairingResultState = PairingState::RecStatus;
      }
      break;
    }
    case PairingState::RecStatus: {
      if (receivedStatus == 0) {
        if (debugNukiConnect) {
          ESP_LOGD("NukiBle", "####################### PAIRING DONE ###############################################");
        }
        nukiPairingResultState = PairingState::Success;
      }
      break;
    }
    default: {
      ESP_LOGE("NukiBle", "Unknown pairing status");
      nukiPairingResultState = PairingState::Timeout;
    }
  }

  if ((esp_timer_get_time() / 1000) - timeNow > PAIRING_TIMEOUT) {
    ESP_LOGW("NukiBle", "Pairing timeout");
    nukiPairingResultState = PairingState::Timeout;
  }

  return nukiPairingResultState;
}

bool NukiBle::sendEncryptedMessage(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen) {
  /*
  #     ADDITIONAL DATA (not encr)      #                    PLAIN DATA (encr)                             #
  #  nonce  # auth identifier # msg len # authorization identifier # command identifier # payload #  crc   #
  # 24 byte #    4 byte       # 2 byte  #      4 byte              #       2 byte       #  n byte # 2 byte #
  */

  //compose plain data
  unsigned char plainData[6 + payloadLen] = {};
  unsigned char plainDataWithCrc[8 + payloadLen] = {};

  if(encryptPairing) {
    plainData[0] = (deviceId >> (8 * 0)) & 0xff;
    plainData[1] = (deviceId >> (8 * 1)) & 0xff;
    plainData[2] = (deviceId >> (8 * 2)) & 0xff;
    plainData[3] = (deviceId >> (8 * 3)) & 0xff;
  } else {
    memcpy(&plainData[0], &authorizationId, sizeof(authorizationId));
  }
  memcpy(&plainData[4], &commandIdentifier, sizeof(commandIdentifier));
  memcpy(&plainData[6], payload, payloadLen);

  //get crc over plain data
  uint16_t dataCrc = calculateCrc((uint8_t*)plainData, 0, sizeof(plainData));

  memcpy(&plainDataWithCrc[0], &plainData, sizeof(plainData));
  memcpy(&plainDataWithCrc[sizeof(plainData)], &dataCrc, sizeof(dataCrc));

  if (debugNukiHexData) {
    ESP_LOGD("NukiBle", "payloadlen: %d", payloadLen);
    ESP_LOGD("NukiBle", "sizeof(plainData): %d", sizeof(plainData));
    ESP_LOGD("NukiBle", "CRC: %02x", dataCrc);
  }
  printBuffer((uint8_t*)plainDataWithCrc, sizeof(plainDataWithCrc), false, "Plain data with CRC: ", debugNukiHexData);

  //compose additional data
  unsigned char additionalData[30] = {};
  generateNonce(sentNonce, sizeof(sentNonce), debugNukiHexData);

  memcpy(&additionalData[0], sentNonce, sizeof(sentNonce));

  if(encryptPairing) {
    additionalData[24] = (deviceId >> (8 * 0)) & 0xff;
    additionalData[25] = (deviceId >> (8 * 1)) & 0xff;
    additionalData[26] = (deviceId >> (8 * 2)) & 0xff;
    additionalData[27] = (deviceId >> (8 * 3)) & 0xff;
  } else {
    memcpy(&additionalData[24], authorizationId, sizeof(authorizationId));
  }

  //Encrypt plain data
  unsigned char plainDataEncr[ sizeof(plainDataWithCrc) + crypto_secretbox_MACBYTES] = {0};
  int encrMsgLen = encode(plainDataEncr, plainDataWithCrc, sizeof(plainDataWithCrc), sentNonce, secretKeyK);

  if (encrMsgLen >= 0) {
    int16_t length = sizeof(plainDataEncr);
    memcpy(&additionalData[28], &length, 2);

    printBuffer((uint8_t*)additionalData, 30, false, "Additional data: ", debugNukiHexData);
    printBuffer((uint8_t*)secretKeyK, sizeof(secretKeyK), false, "Encryption key (secretKey): ", debugNukiHexData);
    printBuffer((uint8_t*)plainDataEncr, sizeof(plainDataEncr), false, "Plain data encrypted: ", debugNukiHexData);

    //compose complete message
    unsigned char dataToSend[sizeof(additionalData) + sizeof(plainDataEncr)] = {};
    memcpy(&dataToSend[0], additionalData, sizeof(additionalData));
    memcpy(&dataToSend[30], plainDataEncr, sizeof(plainDataEncr));

    if(encryptPairing) {
      if (connectBle(bleAddress, true)) {
        printBuffer((uint8_t*)dataToSend, sizeof(dataToSend), false, "Sending encrypted pairing message", debugNukiHexData);
        encryptPairing = false;
        recieveEncrypted = true;
        return pGdioCharacteristic->writeValue((uint8_t*)dataToSend, sizeof(dataToSend), true);
      } else {
        ESP_LOGW("NukiBle", "Send encr msg failed due to unable to connect");
      }
    } else {
      if (connectBle(bleAddress, false)) {
        printBuffer((uint8_t*)dataToSend, sizeof(dataToSend), false, "Sending encrypted message", debugNukiHexData);
        return pUsdioCharacteristic->writeValue((uint8_t*)dataToSend, sizeof(dataToSend), true);
      } else {
        ESP_LOGW("NukiBle", "Send encr msg failed due to unable to connect");
      }
    }
  } else {
    ESP_LOGW("NukiBle", "Send msg failed due to encryption fail");
  }
  return false;
}

bool NukiBle::sendPlainMessage(Command commandIdentifier, const unsigned char* payload, const uint8_t payloadLen) {
  /*
  #                PLAIN DATA                   #
  #command identifier  #   payload   #   crc    #
  #      2 byte        #   n byte    #  2 byte  #
  */

  //compose data
  char dataToSend[200];
  memcpy(&dataToSend, &commandIdentifier, sizeof(commandIdentifier));
  memcpy(&dataToSend[2], payload, payloadLen);
  uint16_t dataCrc = calculateCrc((uint8_t*)dataToSend, 0, payloadLen + 2);

  memcpy(&dataToSend[2 + payloadLen], &dataCrc, sizeof(dataCrc));
  printBuffer((uint8_t*)dataToSend, payloadLen + 4, false, "Sending plain message", debugNukiHexData);
  if (debugNukiHexData) {
    ESP_LOGD("NukiBle", "Command identifier: %02x, CRC: %04x", (unsigned int)commandIdentifier, dataCrc);
  }

  if (connectBle(bleAddress, true)) {
    return pGdioCharacteristic->writeValue((uint8_t*)dataToSend, payloadLen + 4, true);
  } else {
    ESP_LOGW("NukiBle", "Send plain msg failed due to unable to connect");
  }
  return false;
}

bool NukiBle::registerOnGdioChar() {
  // Obtain a reference to the KeyTurner Pairing service
  if (pKeyturnerPairingService == nullptr) {
    if (isLockUltra()) {
      pKeyturnerPairingService = pClient->getService(pairingServiceUltraUUID);
    } else {
      pKeyturnerPairingService = pClient->getService(pairingServiceUUID);
    }
    if (pKeyturnerPairingService != nullptr) {
      if (pGdioCharacteristic == nullptr) {
        //Obtain reference to GDIO char
        if (isLockUltra()) {
          pGdioCharacteristic = pKeyturnerPairingService->getCharacteristic(gdioUltraUUID);
        } else {
          pGdioCharacteristic = pKeyturnerPairingService->getCharacteristic(gdioUUID);
        }
      }
    }
  }

  if (pKeyturnerPairingService != nullptr) {
    if (pGdioCharacteristic != nullptr) {
      if (pGdioCharacteristic->canIndicate()) {
        if(!pGdioCharacteristic->subscribe(false, callback, true)) {
          ESP_LOGW("NukiBle", "Unable to subscribe to GDIO characteristic");
          refreshServices = true;
          disconnect();
          return false;
        }
        if (debugNukiCommunication) {
          ESP_LOGD("NukiBle", "GDIO characteristic registered");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        return true;
      } else {
        if (debugNukiCommunication) {
          ESP_LOGD("NukiBle", "GDIO characteristic canIndicate false, stop connecting");
        }
        refreshServices = true;
        disconnect();
        return false;
      }
    } else {
      ESP_LOGW("NukiBle", "Unable to get GDIO characteristic");
      refreshServices = true;
      disconnect();
      return false;
    }
  } else {
    ESP_LOGW("NukiBle", "Unable to get keyturner pairing service");
    refreshServices = true;
    disconnect();
    return false;
  }
  return false;
}

bool NukiBle::registerOnUsdioChar() {
  // Obtain a reference to the KeyTurner service
  if (pKeyturnerDataService == nullptr) {
    pKeyturnerDataService = pClient->getService(deviceServiceUUID);
    if (pKeyturnerDataService != nullptr) {
      if (pUsdioCharacteristic == nullptr) {
        //Obtain reference to GDIO char
        pUsdioCharacteristic = pKeyturnerDataService->getCharacteristic(userDataUUID);
      }
    }
  }

  if (pKeyturnerDataService != nullptr) {
    if (pUsdioCharacteristic != nullptr) {
      if (pUsdioCharacteristic->canIndicate()) {
        if(!pUsdioCharacteristic->subscribe(false, callback, true)) {
          ESP_LOGW("NukiBle", "Unable to subscribe to USDIO characteristic");
          refreshServices = true;
          disconnect();
          return false;
        }
        if (debugNukiCommunication) {
          ESP_LOGD("NukiBle", "USDIO characteristic registered");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        return true;
      } else {
        if (debugNukiCommunication) {
          ESP_LOGD("NukiBle", "USDIO characteristic canIndicate false, stop connecting");
        }
        refreshServices = true;
        disconnect();
        return false;
      }
    } else {
      ESP_LOGW("NukiBle", "Unable to get USDIO characteristic");
      refreshServices = true;
      disconnect();
      return false;
    }
  } else {
    ESP_LOGW("NukiBle", "Unable to get keyturner data service");
    refreshServices = true;
    disconnect();
    return false;
  }

  return false;
}

void NukiBle::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* recData, size_t length, bool isNotify) {
  vTaskDelay(pdMS_TO_TICKS(100));

  lastHeartbeat = (esp_timer_get_time() / 1000);

  if (debugNukiCommunication) {
    ESP_LOGD("NukiBle", "Notify callback for characteristic: %s of length: %d", pBLERemoteCharacteristic->getUUID().toString().c_str(), length);
  }
  printBuffer((uint8_t*)recData, length, false, "Received data", debugNukiHexData);

  if (pBLERemoteCharacteristic->getUUID() == gdioUUID || (pBLERemoteCharacteristic->getUUID() == gdioUltraUUID && (!recieveEncrypted || length < 24))) {
    //handle not encrypted msg
    uint16_t returnCode = ((uint16_t)recData[1] << 8) | recData[0];
    crcCheckOke = crcValid(recData, length, debugNukiCommunication);
    if (crcCheckOke) {
      unsigned char plainData[200];
      memcpy(plainData, &recData[2], length - 4);
      handleReturnMessage((Command)returnCode, plainData, length - 4);
    }
  } else if (pBLERemoteCharacteristic->getUUID() == userDataUUID || (pBLERemoteCharacteristic->getUUID() == gdioUltraUUID && recieveEncrypted)) {
    if (pBLERemoteCharacteristic->getUUID() == gdioUltraUUID) {
      recieveEncrypted = false;
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
    decode(decrData, encrData, encrMsgLen, recNonce, secretKeyK);

    if (debugNukiCommunication) {
      ESP_LOGD("NukiBle", "Received encrypted msg, len: %d", encrMsgLen);
    }
    printBuffer(recNonce, sizeof(recNonce), false, "received nonce", debugNukiHexData);
    printBuffer(recAuthorizationId, sizeof(recAuthorizationId), false, "Received AuthorizationId", debugNukiHexData);
    printBuffer(encrData, sizeof(encrData), false, "Rec encrypted data", debugNukiHexData);
    printBuffer(decrData, sizeof(decrData), false, "Decrypted data", debugNukiHexData);

    crcCheckOke = crcValid(decrData, sizeof(decrData), debugNukiCommunication);
    if (crcCheckOke) {
      uint16_t returnCode = 0;
      memcpy(&returnCode, &decrData[4], 2);
      unsigned char payload[sizeof(decrData) - 8];
      memcpy(&payload, &decrData[6], sizeof(payload));
      handleReturnMessage((Command)returnCode, payload, sizeof(payload));
    }
  }
}

void NukiBle::handleReturnMessage(Command returnCode, unsigned char* data, uint16_t dataLen) {
  switch (returnCode) {
    case Command::RequestData : {
      if (debugNukiCommunication) {
        ESP_LOGD("NukiBle", "requestData");
      }
      break;
    }
    case Command::PublicKey : {
      memcpy(remotePublicKey, data, 32);
      printBuffer(remotePublicKey, sizeof(remotePublicKey), false,  "Remote public key", debugNukiHexData);
      break;
    }
    case Command::Challenge : {
      memcpy(challengeNonceK, data, 32);
      printBuffer((uint8_t*)data, dataLen, false, "Challenge", debugNukiHexData);
      break;
    }
    case Command::AuthorizationAuthenticator : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationAuthenticator", debugNukiHexData);
      break;
    }
    case Command::AuthorizationData : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationData", debugNukiHexData);
      break;
    }
    case Command::AuthorizationId : {
      unsigned char lockId[16];
      printBuffer((uint8_t*)data, dataLen, false, "authorizationId data", debugNukiHexData);
      if (isLockUltra()) {
        memcpy(authorizationId, &data[0], 4);
        memcpy(lockId, &data[4], sizeof(lockId));
        receivedStatus = 0;
      } else {
        memcpy(authorizationId, &data[32], 4);
        memcpy(lockId, &data[36], sizeof(lockId));
        memcpy(challengeNonceK, &data[52], sizeof(challengeNonceK));
      }
      printBuffer(authorizationId, sizeof(authorizationId), false, AUTH_ID_STORE_NAME, debugNukiHexData);
      printBuffer(lockId, sizeof(lockId), false, "lockId", debugNukiHexData);
      break;
    }
    case Command::AuthorizationEntry : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationEntry", debugNukiHexData);
      AuthorizationEntry authEntry;
      memcpy(&authEntry, data, dataLen);
      listOfAuthorizationEntries.push_back(authEntry);
      if (debugNukiReadableData) {
        NukiLock::logAuthorizationEntry(authEntry, true);
      }
      break;
    }
    case Command::Status : {
      printBuffer((uint8_t*)data, dataLen, false, "status", debugNukiHexData);
      receivedStatus = data[0];
      if (debugNukiCommunication) {
        if (receivedStatus == 0) {
          ESP_LOGD("NukiBle", "command COMPLETE");
        } else if (receivedStatus == 1) {
          ESP_LOGD("NukiBle", "command ACCEPTED");
        }
      }
      break;
    }
    case Command::OpeningsClosingsSummary : {
      printBuffer((uint8_t*)data, dataLen, false, "openingsClosingsSummary", debugNukiHexData);
      ESP_LOGW("NukiBle", "NOT IMPLEMENTED ONLY FOR NUKI v1"); //command is not available on Nuki v2 (only on Nuki v1)
      break;
    }
    case Command::ErrorReport : {
      ESP_LOGE("NukiBle", "Error: %02x for command: %02x:%02x", data[0], data[2], data[1]);
      memcpy(&errorCode, &data[0], sizeof(errorCode));
      logErrorCode(data[0]);
      if ((uint8_t)data[0] == (uint8_t)0x21) {
        if (eventHandler) {
          eventHandler->notify(EventType::ERROR_BAD_PIN);
        }
      }
      break;
    }
    case Command::AuthorizationIdConfirmation : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationIdConfirmation", debugNukiHexData);
      break;
    }
    case Command::AuthorizationIdInvite : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationIdInvite", debugNukiHexData);
      break;
    }
    case Command::AuthorizationInfo : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationInfo", debugNukiHexData);
      ultraAuthInfoCommandReceived = true;
      break;
    }
    case Command::AuthorizationEntryCount : {
      printBuffer((uint8_t*)data, dataLen, false, "authorizationEntryCount", debugNukiHexData);
      uint16_t count = 0;
      memcpy(&count, data, 2);
      ESP_LOGD("NukiBle", "authorizationEntryCount: %d", count);
      break;
    }
    case Command::LogEntryCount : {
      memcpy(&loggingEnabled, data, sizeof(logEntryCount));
      memcpy(&logEntryCount, &data[1], sizeof(logEntryCount));
      if (debugNukiReadableData) {
        ESP_LOGD("NukiBle", "Logging enabled: %d, total nr of log entries: %d", loggingEnabled, logEntryCount);
      }
      printBuffer((uint8_t*)data, dataLen, false, "logEntryCount", debugNukiHexData);
      break;
    }
    case Command::TimeControlEntryCount : {
      printBuffer((uint8_t*)data, dataLen, false, "timeControlEntryCount", debugNukiHexData);
      break;
    }
    case Command::KeypadCodeId : {
      printBuffer((uint8_t*)data, dataLen, false, "keypadCodeId", debugNukiHexData);
      break;
    }
    case Command::KeypadCodeCount : {
      memcpy(&nrOfKeypadCodes, data, 2);
      keypadCodeCountReceived = true;
      printBuffer((uint8_t*)data, dataLen, false, "keypadCodeCount", debugNukiHexData);
      if (debugNukiReadableData) {
        uint16_t count = 0;
        memcpy(&count, data, 2);
        ESP_LOGD("NukiBle", "keyPadCodeCount: %d", count);
      }

      break;
    }
    case Command::FingerprintEntry : {
      FingerprintEntry fingerprintEntry;
      memcpy(&fingerprintEntry, data, dataLen);
      listOfFingerprintEntries.push_back(fingerprintEntry);

      printBuffer((uint8_t*)data, dataLen, false, "fingerprintEntry", debugNukiHexData);
      if (debugNukiReadableData) {
        NukiLock::logFingerprintEntry(fingerprintEntry, true);
      }
      break;
    }
    case Command::KeypadCode : {
      KeypadEntry keypadEntry;
      memcpy(&keypadEntry, data, dataLen);
      listOfKeyPadEntries.push_back(keypadEntry);
      nrOfReceivedKeypadCodes++;

      printBuffer((uint8_t*)data, dataLen, false, "keypadCode", debugNukiHexData);
      if (debugNukiReadableData) {
        NukiLock::logKeypadEntry(keypadEntry, true);
      }
      break;
    }
    case Command::KeypadAction : {
      printBuffer((uint8_t*)data, dataLen, false, "keypadAction", debugNukiHexData);
      break;
    }
    default:
      ESP_LOGE("NukiBle", "UNKNOWN RETURN COMMAND: %04x", (unsigned int)returnCode);
  }
}

void NukiBle::onConnect(BLEClient*) {
  extendDisconnectTimeout();
  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "BLE connected");
  }
};

void NukiBle::onDisconnect(BLEClient*, int reason)
{
  countDisconnects = 0;
  if (debugNukiConnect) {
    ESP_LOGD("NukiBle", "BLE disconnected");
  }
  countDisconnects = 0;
};

void NukiBle::setEventHandler(SmartlockEventHandler* handler) {
  eventHandler = handler;
}

const bool NukiBle::isPairedWithLock() const {
  return isPaired;
};

const bool NukiBle::isLockUltra() const {
  return smartLockUltra;
};

int NukiBle::getRssi() const {
  return rssi;
}

int64_t NukiBle::getLastReceivedBeaconTs() const {
  return lastReceivedBeaconTs;
}

int64_t NukiBle::getLastHeartbeat() {
  return lastHeartbeat;
}

const BLEAddress NukiBle::getBleAddress() const {
  return bleAddress;
}

void NukiBle::setDebugConnect(bool enable) {
  debugNukiConnect = enable;
}

void NukiBle::setDebugCommunication(bool enable) {
  debugNukiCommunication = enable;
}

void NukiBle::setDebugReadableData(bool enable) {
  debugNukiReadableData = enable;
}

void NukiBle::setDebugHexData(bool enable) {
  debugNukiHexData = enable;
}

void NukiBle::setDebugCommand(bool enable) {
  debugNukiCommand = enable;
}

void NukiBle::logMessage(const char* message, int level) {
  switch (level) {
    case 1:
      ESP_LOGE("NukiBle", "%s", message);
      break;
    case 2:
      ESP_LOGW("NukiBle", "%s", message);
      break;
    case 3:
      ESP_LOGI("NukiBle", "%s", message);
      break;
    case 4:
    default:
      ESP_LOGD("NukiBle", "%s", message);
      break;
  }
}

void NukiBle::logMessageVar(const char* message, unsigned int var, int level) {
  switch (level) {
    case 1:
      ESP_LOGE("NukiBle", "%s - Details: %u", message, var);
      break;
    case 2:
      ESP_LOGW("NukiBle", "%s - Details: %u", message, var);
      break;
    case 3:
      ESP_LOGI("NukiBle", "%s - Details: %u", message, var);
      break;
    case 4:
    default:
      ESP_LOGD("NukiBle", "%s - Details: %u", message, var);
      break;
  }
}

void NukiBle::logMessageVar(const char* message, const char* var, int level) {
  switch (level) {
    case 1:
      ESP_LOGE("NukiBle", "%s - Details: %s", message, var);
      break;
    case 2:
      ESP_LOGW("NukiBle", "%s - Details: %s", message, var);
      break;
    case 3:
      ESP_LOGI("NukiBle", "%s - Details: %s", message, var);
      break;
    case 4:
    default:
      ESP_LOGD("NukiBle", "%s - Details: %s", message, var);
      break;
  }
}

} // namespace Nuki