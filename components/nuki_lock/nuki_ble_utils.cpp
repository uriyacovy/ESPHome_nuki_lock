#include "nuki_ble_utils.h"

#include <sodium.h>
#include "Crc16.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <bitset>

#include "esphome/core/log.h"
#include "esp_system.h"
#include "esp_random.h"

namespace esphome::nuki_lock {

static const char *const TAG = "NukiBle";

void print_buffer(const uint8_t* buff, const uint8_t size, const bool asChars, const char* header, bool debug) {
  if (debug) {
    char tmp[16];

    if (strlen(header) > 0) {
      ESP_LOGI(TAG, "%s: ", header);
    }

    for (int i = 0; i < size; i++) {
      if (asChars) {
        ESP_LOGI(TAG, "%c", (char)buff[i]);
      } else {
        sprintf(tmp, "%02x", buff[i]);
        ESP_LOGI(TAG, "%s ", tmp);
      }
    }
    
    ESP_LOGI(TAG, "\n");
  }
}

bool is_char_array_not_empty(unsigned char* array, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    if (array[i] != 0) {
      return true;
    }
  }
  return false;
}

bool is_char_array_empty(unsigned char* array, uint16_t len) {
  for (size_t i = 0; i < len; i++) {
    if (array[i] != 0) {
      return false;
    }
  }
  return true;
}

bool compare_char_array(unsigned char* a, unsigned char* b, uint8_t len) {
  for (int i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

int encode(unsigned char* output, unsigned char* input, unsigned long long len, unsigned char* nonce, unsigned char* keyS) {
  int result = crypto_secretbox_easy(output, input, len, nonce, keyS);

  if (result) {
    ESP_LOGD(TAG, "Encryption failed (length %llu, given result %i)\n", len, result);
    return -1;
  }
  return len;
}

int decode(unsigned char* output, unsigned char* input, unsigned long long len, unsigned char* nonce, unsigned char* keyS) {

  int result = crypto_secretbox_open_easy(output, input, len, nonce, keyS);

  if (result) {
    ESP_LOGW(TAG, "Decryption failed (length %llu, given result %i)\n", len, result);
    return -1;
  }
  return len;
}

void generate_nonce(unsigned char* hexArray, uint8_t nrOfBytes, bool debug) {
  for(uint8_t i = 0; i < nrOfBytes; i++) {
      hexArray[i] = (unsigned char)(esp_random() & 0xFF);
  }
  print_buffer((uint8_t*)hexArray, nrOfBytes, false, "Nonce", debug);
}

unsigned int calculate_crc(uint8_t* data, uint8_t start, uint16_t length) {
  Crc16 crcObj;
  crcObj.clearCrc();
  // CCITT-False:	width=16 poly=0x1021 init=0xffff refin=false refout=false xorout=0x0000 check=0x29b1
  return crcObj.fastCrc(data, start, length, false, false, 0x1021, 0xffff, 0x0000, 0x8000, 0xffff);
}

bool crc_valid(uint8_t* pData, uint16_t length, bool debug) {
  uint16_t receivedCrc = ((uint16_t)pData[length - 1] << 8) | pData[length - 2];
  uint16_t dataCrc = calculate_crc(pData, 0, length - 2);

  if (!(receivedCrc == dataCrc)) {
    ESP_LOGE(TAG, "CRC CHECK FAILED!");
    return false;
  }
  if (debug) {
    ESP_LOGD(TAG, "CRC CHECK OK");
  }
  return true;
}

void log_authorization_entry(AuthorizationEntry authorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "id:%d", (unsigned int)authorizationEntry.authId);
    ESP_LOGD(TAG, "idType:%d", (unsigned int)authorizationEntry.idType);
    ESP_LOGD(TAG, "name:%s", (const char*)authorizationEntry.name);
    ESP_LOGD(TAG, "enabled:%d", (unsigned int)authorizationEntry.enabled);
    ESP_LOGD(TAG, "remoteAllowed:%d", (unsigned int)authorizationEntry.remoteAllowed);
    ESP_LOGD(TAG, "createdYear:%d", (unsigned int)authorizationEntry.createdYear);
    ESP_LOGD(TAG, "createdMonth:%d", (unsigned int)authorizationEntry.createdMonth);
    ESP_LOGD(TAG, "createdDay:%d", (unsigned int)authorizationEntry.createdDay);
    ESP_LOGD(TAG, "createdHour:%d", (unsigned int)authorizationEntry.createdHour);
    ESP_LOGD(TAG, "createdMin:%d", (unsigned int)authorizationEntry.createdMinute);
    ESP_LOGD(TAG, "createdSec:%d", (unsigned int)authorizationEntry.createdSecond);
    ESP_LOGD(TAG, "lastactYear:%d", (unsigned int)authorizationEntry.lastActYear);
    ESP_LOGD(TAG, "lastactMonth:%d", (unsigned int)authorizationEntry.lastActMonth);
    ESP_LOGD(TAG, "lastactDay:%d", (unsigned int)authorizationEntry.lastActDay);
    ESP_LOGD(TAG, "lastactHour:%d", (unsigned int)authorizationEntry.lastActHour);
    ESP_LOGD(TAG, "lastactMin:%d", (unsigned int)authorizationEntry.lastActMinute);
    ESP_LOGD(TAG, "lastactSec:%d", (unsigned int)authorizationEntry.lastActSecond);
    ESP_LOGD(TAG, "lockCount:%d", (unsigned int)authorizationEntry.lockCount);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)authorizationEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)authorizationEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)authorizationEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)authorizationEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)authorizationEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)authorizationEntry.allowedFromMinute);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)authorizationEntry.allowedFromSecond);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)authorizationEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)authorizationEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)authorizationEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)authorizationEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)authorizationEntry.allowedUntilMinute);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)authorizationEntry.allowedUntilSecond);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)authorizationEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)authorizationEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)authorizationEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)authorizationEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)authorizationEntry.allowedUntilTimeMin);
  }
}

void log_new_authorization_entry(NewAuthorizationEntry newAuthorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "name:%s", (const char*)newAuthorizationEntry.name);
    ESP_LOGD(TAG, "idType:%d", (unsigned int)newAuthorizationEntry.idType);
    ESP_LOGD(TAG, "remoteAllowed:%d", (unsigned int)newAuthorizationEntry.remoteAllowed);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)newAuthorizationEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)newAuthorizationEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)newAuthorizationEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)newAuthorizationEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)newAuthorizationEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)newAuthorizationEntry.allowedFromMinute);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)newAuthorizationEntry.allowedFromSecond);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)newAuthorizationEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)newAuthorizationEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)newAuthorizationEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)newAuthorizationEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)newAuthorizationEntry.allowedUntilMinute);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)newAuthorizationEntry.allowedUntilSecond);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)newAuthorizationEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)newAuthorizationEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)newAuthorizationEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)newAuthorizationEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)newAuthorizationEntry.allowedUntilTimeMin);
  }
}

void log_updated_authorization_entry(UpdatedAuthorizationEntry updatedAuthorizationEntry, bool debug) {
  if (debug) {
    ESP_LOGD(TAG, "id:%d", (unsigned int)updatedAuthorizationEntry.authId);
    ESP_LOGD(TAG, "name:%s", (const char*)updatedAuthorizationEntry.name);
    ESP_LOGD(TAG, "enabled:%d", (unsigned int)updatedAuthorizationEntry.enabled);
    ESP_LOGD(TAG, "remoteAllowed:%d", (unsigned int)updatedAuthorizationEntry.remoteAllowed);
    ESP_LOGD(TAG, "timeLimited:%d", (unsigned int)updatedAuthorizationEntry.timeLimited);
    ESP_LOGD(TAG, "allowedFromYear:%d", (unsigned int)updatedAuthorizationEntry.allowedFromYear);
    ESP_LOGD(TAG, "allowedFromMonth:%d", (unsigned int)updatedAuthorizationEntry.allowedFromMonth);
    ESP_LOGD(TAG, "allowedFromDay:%d", (unsigned int)updatedAuthorizationEntry.allowedFromDay);
    ESP_LOGD(TAG, "allowedFromHour:%d", (unsigned int)updatedAuthorizationEntry.allowedFromHour);
    ESP_LOGD(TAG, "allowedFromMin:%d", (unsigned int)updatedAuthorizationEntry.allowedFromMinute);
    ESP_LOGD(TAG, "allowedFromSec:%d", (unsigned int)updatedAuthorizationEntry.allowedFromSecond);
    ESP_LOGD(TAG, "allowedUntilYear:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilYear);
    ESP_LOGD(TAG, "allowedUntilMonth:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilMonth);
    ESP_LOGD(TAG, "allowedUntilDay:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilDay);
    ESP_LOGD(TAG, "allowedUntilHour:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilHour);
    ESP_LOGD(TAG, "allowedUntilMin:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilMinute);
    ESP_LOGD(TAG, "allowedUntilSec:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilSecond);
    ESP_LOGD(TAG, "allowedWeekdays:%d", (unsigned int)updatedAuthorizationEntry.allowedWeekdays);
    ESP_LOGD(TAG, "allowedFromTimeHour:%d", (unsigned int)updatedAuthorizationEntry.allowedFromTimeHour);
    ESP_LOGD(TAG, "allowedFromTimeMin:%d", (unsigned int)updatedAuthorizationEntry.allowedFromTimeMin);
    ESP_LOGD(TAG, "allowedUntilTimeHour:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilTimeHour);
    ESP_LOGD(TAG, "allowedUntilTimeMin:%d", (unsigned int)updatedAuthorizationEntry.allowedUntilTimeMin);
  }
}

template<std::size_t N>
uint8_t get_weekdays_int_from_bitset(const std::bitset<N> bits) {
  uint8_t result = 0;
  for (auto idx = 0; idx < 7 && idx < N ; idx++) {
    result |= bits[idx] << (7 - idx);
  }
  return result;
}
}  // namespace esphome::nuki_lock