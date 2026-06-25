#pragma once

/**
 * @file nuki_ble_utils.h
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

#include "nuki_ble_constants.h"
#include <bitset>
#include <cstdint>
#include <cstring>
#include <string>

#include "esphome/core/log.h"

namespace esphome::nuki_lock {

void print_buffer(const uint8_t* buff, const uint8_t size, const bool asChars, const char* header, bool debug = false);
bool is_char_array_not_empty(unsigned char* array, uint16_t len);
bool is_char_array_empty(unsigned char* array, uint16_t len);
bool compare_char_array(unsigned char* a, unsigned char* b, uint8_t len);
int encode(unsigned char* output, unsigned char* input, unsigned long long len, unsigned char* nonce, unsigned char* keyS);
int decode(unsigned char* output, unsigned char* input, unsigned long long len, unsigned char* nonce, unsigned char* keyS);
void generate_nonce(unsigned char* hexArray, uint8_t nrOfBytes, bool debug = false);

unsigned int calculate_crc(uint8_t data[], uint8_t start, uint16_t length);
bool crc_valid(uint8_t* pData, uint16_t length, bool debug = false);

void log_authorization_entry(AuthorizationEntry authorizationEntry, bool debug = false);
void log_new_authorization_entry(NewAuthorizationEntry newAuthorizationEntry, bool debug = false);
void log_updated_authorization_entry(UpdatedAuthorizationEntry updatedAuthorizationEntry, bool debug = false);

/**
 * @brief Translate a bitset<N> into Nuki weekdays int
 *
 * @tparam N
 * @param bitset with bitset[0] = Monday ... bitset[7] = Sunday
 * @return uint8_t with bit6 = Monday ... bit0 = Sunday
 */
template<std::size_t N>
uint8_t get_weekdays_int_from_bitset(const std::bitset<N> bits);
}  // namespace esphome::nuki_lock