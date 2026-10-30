#pragma once
/**
 * @file nuki_accessories.h
 * Definitions of data types and helper functions for Nuki accessories paired with the lock
 * (Keypad/Keypad 2, fingerprint entries, accessory info) - these are separate physical
 * peripherals, not the lock itself, kept in their own file even though their data flows
 * through NukiBle/NukiLock's BLE connection to the lock.
 *
 * Created: 2022
 * License: GNU GENERAL PUBLIC LICENSE (see LICENSE)
 *
 * This library implements the communication from an ESP32 via BLE to a Nuki smart lock.
 * Based on the Nuki Smart Lock API V2.2.1
 * https://developer.nuki.io/page/nuki-smart-lock-api-2/2/
 *
 */

#include <cstdint>

namespace esphome::nuki_lock {

struct __attribute__((packed)) NewKeypadEntry {
  uint32_t code;  //needs to be 6 digits
  uint8_t name[20];
  uint8_t timeLimited;
  uint16_t allowedFromYear;
  uint8_t allowedFromMonth;
  uint8_t allowedFromDay;
  uint8_t allowedFromHour;
  uint8_t allowedFromMin;
  uint8_t allowedFromSec;
  uint16_t allowedUntilYear;
  uint8_t allowedUntilMonth;
  uint8_t allowedUntilDay;
  uint8_t allowedUntilHour;
  uint8_t allowedUntilMin;
  uint8_t allowedUntilSec;
  // bit 7  6  5  4  3  2  1  0
  //     -  M  T  W  T  F  S  S
  uint8_t allowedWeekdays;
  uint8_t allowedFromTimeHour;
  uint8_t allowedFromTimeMin;
  uint8_t allowedUntilTimeHour;
  uint8_t allowedUntilTimeMin;
};

struct __attribute__((packed)) KeypadEntry {
  uint16_t codeId;
  uint32_t code;
  uint8_t name[20];
  uint8_t enabled;
  uint16_t dateCreatedYear;
  uint8_t dateCreatedMonth;
  uint8_t dateCreatedDay;
  uint8_t dateCreatedHour;
  uint8_t dateCreatedMin;
  uint8_t dateCreatedSec;
  uint16_t dateLastActiveYear;
  uint8_t dateLastActiveMonth;
  uint8_t dateLastActiveDay;
  uint8_t dateLastActiveHour;
  uint8_t dateLastActiveMin;
  uint8_t dateLastActiveSec;
  uint16_t lockCount;
  uint8_t timeLimited;
  uint16_t allowedFromYear;
  uint8_t allowedFromMonth;
  uint8_t allowedFromDay;
  uint8_t allowedFromHour;
  uint8_t allowedFromMin;
  uint8_t allowedFromSec;
  uint16_t allowedUntilYear;
  uint8_t allowedUntilMonth;
  uint8_t allowedUntilDay;
  uint8_t allowedUntilHour;
  uint8_t allowedUntilMin;
  uint8_t allowedUntilSec;

  // bit 7  6  5  4  3  2  1  0
  //     -  M  T  W  T  F  S  S
  uint8_t allowedWeekdays;
  uint8_t allowedFromTimeHour;
  uint8_t allowedFromTimeMin;
  uint8_t allowedUntilTimeHour;
  uint8_t allowedUntilTimeMin;
};

struct __attribute__((packed)) UpdatedKeypadEntry {
  uint16_t codeId;
  uint32_t code;
  uint8_t name[20];
  uint8_t enabled;
  uint8_t timeLimited;
  uint16_t allowedFromYear;
  uint8_t allowedFromMonth;
  uint8_t allowedFromDay;
  uint8_t allowedFromHour;
  uint8_t allowedFromMin;
  uint8_t allowedFromSec;
  uint16_t allowedUntilYear;
  uint8_t allowedUntilMonth;
  uint8_t allowedUntilDay;
  uint8_t allowedUntilHour;
  uint8_t allowedUntilMin;
  uint8_t allowedUntilSec;
  // bit 7  6  5  4  3  2  1  0
  //     -  M  T  W  T  F  S  S
  uint8_t allowedWeekdays;
  uint8_t allowedFromTimeHour;
  uint8_t allowedFromTimeMin;
  uint8_t allowedUntilTimeHour;
  uint8_t allowedUntilTimeMin;
};

struct __attribute__((packed)) FingerprintEntry {
  uint8_t fingerprintId[32];
  uint16_t keypadCodeId = 0;
  uint8_t name[20];
};

struct __attribute__((packed)) Keypad2Config {
  uint8_t updatePending = 0;
  uint8_t ledBrightness = 0;
  uint8_t batteryType = 0;
  uint8_t buttonMode = 0;
  uint8_t lockAction = 0;
};

struct __attribute__((packed)) AccessoryInfo {
  uint16_t dateYear = 0;
  uint8_t dateMonth = 0;
  uint8_t dateDay = 0;
  uint8_t dateHour = 0;
  uint8_t dateMinute = 0;
  uint8_t dateSecond = 0;
  uint32_t accessoryNukiId = 0;
  uint8_t accessoryType = 0;
  unsigned char firmwareVersion[3] = {0, 0 , 0};
  unsigned char hardwareRevision[2] = {0, 0};
  uint8_t productVariantDifferentiator = 0;
  uint16_t mostRecentBatteryVoltage = 0;
  uint8_t mostRecentTemperature = 0;
  //mostRecentEventData
};

void log_new_keypad_entry(NewKeypadEntry newKeypadEntry, bool debug = false);
void log_keypad_entry(KeypadEntry keypadEntry, bool debug = false);
void log_updated_keypad_entry(UpdatedKeypadEntry updatedKeypadEntry, bool debug = false);
void log_fingerprint_entry(FingerprintEntry fingerprintEntry, bool debug = false);
void log_keypad2_config(Keypad2Config keypad2Config, bool debug = false);
void log_accessory_info(AccessoryInfo accessoryInfo, bool debug = false);

}  // namespace esphome::nuki_lock
