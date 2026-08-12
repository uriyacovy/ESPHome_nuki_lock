#pragma once

#include "nuki_ble.h"
#include "nuki_lock_protocol_constants.h"
#include "nuki_lock_protocol_utils.h"

#include <vector>
#include <cstdint>
#include <cstring>
#include <string>

#include "esphome/core/log.h"


namespace esphome::nuki_lock {
class NukiLock : public NukiBle {
  public:
    NukiLock(const std::string& device_name, const uint32_t device_id);


    /**
     * @brief Sends lock action cmd via BLE to the lock
     *
     * @param lock_action
     * @param nukiAppId 0 = App, 1 = Bridge, 2 = Fob, 3 = Keypad
     * @param flags optional
     * @param nameSuffix optional
     * @param nameSuffixLen len of nameSuffix if used ('\0' included, maximum 19)
     * @return CmdResult
     */
    CmdResult lock_action(const LockAction lock_action, const uint32_t nukiAppId = 1, const uint8_t flags = 0,
                              const char* nameSuffix = nullptr, const uint8_t nameSuffixLen = 0);

    /**
     * @brief Send a keypad action entry to the lock via BLE
     * @param source 0x00 = arrow key, 0x01 = code
     * @param code The code that has been entered on the keypad
     * @param keypad_action The action to be executed
     */
    CmdResult keypad_action(KeypadActionSource source, uint32_t code, KeypadAction keypad_action);

    /**
     * @brief Requests keyturner state from Lock via BLE
     *
     * @param retrievedKeyTurnerState Nuki api based datatype to store the retrieved keyturnerstate
     */
    CmdResult request_key_turner_state(KeyTurnerState* retrievedKeyTurnerState);

    /**
     * @brief Gets the last keyturner state stored on the esp
     *
     * @param retrievedKeyTurnerState Nuki api based datatype to store the retrieved keyturnerstate
     */
    void retrieve_key_tuner_state(KeyTurnerState* retrievedKeyTurnerState);

    
    /**
     * @brief Requests battery status from Lock via BLE
     *
     * @param retrievedBatteryReport Nuki api based datatype to store the retrieved battery status
     */
    CmdResult request_battery_report(BatteryReport* retrievedBatteryReport);

    /**
     * @brief Reports the (emulated) door sensor state to the lock via BLE (0x0092).
     *        Frame: [deviceState=0x10][0x00][doorFlag: 0x00=closed, >=0x01=open][0x00]
     *        + challenge nonce (appended by the challenge state machine), no PIN.
     *        The first successful send creates the lock's door-sensor accessory record.
     * @param door_open true = open, false = closed
     * @return CmdResult
     */
    CmdResult report_door_sensor_state(const bool door_open);


    /**
     * @brief Requests config from Lock via BLE
     *
     * @param retrievedConfig Nuki api based datatype to store the retrieved config
     */
    CmdResult request_config(Config* retrievedConfig);

    /**
     * @brief Requests advanced config from Lock via BLE
     *
     * @param retrievedAdvancedConfig Nuki api based datatype to store the retrieved advanced config
     */
    CmdResult request_advanced_config(AdvancedConfig* retrievedAdvancedConfig);

    /**
     * @brief Request the lock via BLE to send the internal log entries
     *
     * @param startIndex Startindex of first log msg to be send
     * @param count The number of log entries to be read, starting at the specified start index.
     * @param sortOrder The desired sort order
     * @param totalCount true if a Log Entry Count is requested from the lock
     */
    CmdResult retrieve_internal_log_entries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount);

    /**
     * @brief Get the Internal Log Entries stored on the esp. Only available after executing retrieve_internal_log_entries.
     *
     * @param requestedInternalLogEntries list to store the returned internal log entries
     */
    void get_internal_log_entries(std::vector<InternalLogEntry>* requestedInternalLogEntries);

    /**
     * @brief Gets the current config from the lock, updates the name parameter and sends the
     * new config to the lock via BLE
     *
     * @param name max 32 character name
     */
    CmdResult set_name(const std::string& name);

    /**
     * @brief Gets the current config from the lock, updates the latitude parameter and sends the new
     * config to the lock via BLE
     *
     * @param degrees the desired latitude
     */
    CmdResult set_latitude(const float degrees);

    /**
     * @brief Gets the current config from the lock, updates the longitude parameter and sends the new
     * config to the lock via BLE
     *
     * @param degrees the desired longitude
     */
    CmdResult set_longitude(const float degrees);

    /**
     * @brief Gets the current config from the lock, updates the auto unlatch parameter and sends the new
     * config to the lock via BLE
     *
     * @param enable true if auto unlatch should be enabled in general.
     */
    CmdResult enable_auto_unlatch(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the given fob action parameter and sends the new
     * config to the lock via BLE
     *
     * @param fobActionNr the fob action to change (1 = single press, 2 = double press, 3 = triple press)
     * @param fobAction the desired fob action setting
     */
    CmdResult set_fob_action(const uint8_t fobActionNr, const uint8_t fobAction);

    /**
     * @brief Gets the current config from the lock, updates the dst parameter and sends the new
     * config to the lock via BLE
     *
     * @param enable The desired daylight saving time mode. false disabled, true european
     */
    CmdResult enable_dst(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the timezone offset parameter and
     * sends the new config to the lock via BLE
     *
     * @param minutes The timezone offset (UTC) in minutes
     */
    CmdResult set_time_zone_offset(const int16_t minutes);

    /**
     * @brief Gets the current config from the lock, updates the timezone id parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeZoneId 	The id of the current timezone or 0xFFFF if timezones are not supported
     */
    CmdResult set_time_zone_id(const TimeZoneId timeZoneId);

    /**
     * @brief Gets the current config from the lock, updates the enable button parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if button enabled
     */
    CmdResult enable_button(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the unlocked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the unlocked position
     */
    CmdResult set_unlocked_position_offset_degrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the locked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the locked position
     */
    CmdResult set_locked_position_offset_degrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the single locked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the single locked position
     */
    CmdResult set_single_locked_position_offset_degrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the unlocked to locked transition offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the position where transition from unlocked to locked happens
     */
    CmdResult set_unlocked_to_locked_transition_offset_degrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the lock n go timeout parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeout the desired timeout for lock ‘n’ go
     */
    CmdResult set_lock_ngo_timeout(const uint8_t timeout);

    /**
     * @brief Gets the current config from the lock, updates the detached cylinder parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if detached cylinder enabled (Flag that indicates that the inner side of the used cylinder is detached from
     * the outer side and therefore the Smart Lock won’t recognize if someone operates the door by using a key)
     */
    CmdResult enable_detached_cylinder(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the unlatch duration parameter and sends the
     * new config to the lock via BLE
     *
     * @param duration the desired duration in seconds for holding the latch in unlatched position
     */
    CmdResult set_unlatch_duration(const uint8_t duration);

    /**
     * @brief Gets the current config from the lock, updates the auto lock timeout parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeout the desired timeout until the smart lock relocks itself after it has been unlocked
     */
    CmdResult set_auto_lock_time_out(const uint8_t timeout);

    /**
     * @brief Gets the current config from the lock, updates the night mode parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode enabled
     */
    CmdResult enable_night_mode(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the night mode start time parameter and sends the
     * new config to the lock via BLE
     *
     * @param starttime the desired night mode start time
     */
    CmdResult set_night_mode_start_time(unsigned char starttime[2]);

    /**
     * @brief Gets the current config from the lock, updates the night mode end time parameter and sends the
     * new config to the lock via BLE
     *
     * @param endtime the desired night mode end time
     */
    CmdResult set_night_mode_end_time(unsigned char endtime[2]);

    /**
     * @brief Gets the current config from the lock, updates the night mode auto lock parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode auto lock enabled
     */
    CmdResult enable_night_mode_auto_lock(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the night mode auto unlock parameter and sends the
     * new config to the lock via BLE
     *
     * @param disable true if night mode auto unlock disabled
     */
    CmdResult disable_night_mode_auto_unlock(const bool disable);

    /**
     * @brief Gets the current config from the lock, updates the night mode immediate lock on start parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode immediate lock on start enabled
     */
    CmdResult enable_night_mode_immediate_lock_on_start(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the single button press action
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    CmdResult set_single_button_press_action(const ButtonPressAction action);

    /**
     * @brief Gets the current advanced config from the lock, updates the double button press action
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a double button press
     */
    CmdResult set_double_button_press_action(const ButtonPressAction action);

    /**
     * @brief Gets the current advanced config from the lock, updates the battery type parameter and
     * sends the new advanced config to the lock via BLE
     *
     * @param type 	The type of the batteries present in the smart lock.
     */
    CmdResult set_battery_type(const BatteryType type);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable battery type
     * detection parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if the automatic detection of the battery type is enabled
     */
    CmdResult enable_auto_battery_type_detection(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the disable autounlock
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param disable true if auto unlock should be disabled in general.
     */
    CmdResult disable_auto_unlock(const bool disable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable autolock
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if auto lock should be enabled in general.
     */
    CmdResult enable_auto_lock(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable immediate
     * autolock parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if auto lock should be performed immediately after the door has
     * been closed (requires active door sensor)
     */
    CmdResult enable_immediate_auto_lock(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable auto update
     * parameter and sends the new advanced config to the lock via BLE
     * (Updating the firmware requires the Nuki app. CAUTION: updating FW could cause breaking changes)
     *
     * @param enable true if automatic firmware updates should be enabled
     */
    CmdResult enable_auto_update(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the motor speed
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    CmdResult set_motor_speed(const MotorSpeed speed);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable slow speed during NightMode
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    CmdResult enable_slow_speed_during_night_mode(const bool enable);

    /**
     * @brief Sets the lock ability to pair with other devices (can be used to prevent unauthorized pairing)
     * Gets the current config from the lock, updates the pairing parameter and sends the new config to the lock via BLE
     * (CAUTION: if pairing is set to false and credentials are deleted a factory reset of the lock needs to be performed
     * before pairing is possible again)
     *
     * @param enable true if allowed to pair with other devices
     */
    CmdResult enable_pairing(const bool enable);

    /**
     * @brief Gets the lock current config wrt pairing with other devices
     */
    bool pairing_enabled();

    /**
     * @brief Gets the current config from the lock, updates the whether or not the flashing
     * LED should be enabled to signal an unlocked door. And sends the new config to the lock via BLE
     *
     * @param enable true if led enabled
     */
    CmdResult enable_led_flash(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the LED brightness parameter and
     * sends the new config to the lock via BLE
     *
     * @param level The LED brightness level. Possible values are 0 to 5 0 = off, …, 5 = max
     */
    CmdResult set_led_brightness(const uint8_t level);

    /**
     * @brief Gets the current config from the lock, updates the LED brightness parameter
     * and sends the new config to the lock via BLE
     *
     * @param enable true if only a single lock should be performed
     */
    CmdResult enable_single_lock(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the advertising frequency parameter
     * and sends the new config to the lock via BLE
     *
     * @param mode 0x00 Automatic, 0x01 Normal, 0x02 Slow, 0x03 Slowest (~400ms till ~1s)
     */
    CmdResult set_advertising_mode(const AdvertisingMode mode);

    /**
     * @brief Sends a new time(d) control entry via BLE to the lock.
     * This entry is independant of keypad or authorization entries, it will execute the
     * defined action at the defined time in the newTimeControlEntry
     *
     * @param newTimecontrolEntry Nuki api based datatype to send
     */
    CmdResult add_time_control_entry(NewTimeControlEntry newTimecontrolEntry);

    /**
     * @brief Sends an updated time(d) control entry via BLE to the lock.
     * (see add_time_control_entry())
     *
     * @param TimeControlEntry Nuki api based datatype to send.
     * The ID can be retrieved via retrieve_time_control_entries()
     */
    CmdResult update_time_control_entry(TimeControlEntry TimeControlEntry);

    /**
     * @brief Deletes a time(d) control entry via BLE to the lock.
     * (see add_time_control_entry())
     *
     * @param entryId The ID to be deleted, can be retrieved via retrieve_time_control_entries()
     */
    CmdResult remove_time_control_entry(uint8_t entryId);

    /**
     * @brief Request the lock via BLE to send the existing time control entries
     *
     */
    CmdResult retrieve_time_control_entries();

    /**
     * @brief Get the time control entries stored on the esp (after executing retrieve_time_control_entries())
     *
     * @param timeControlEntries list to store the returned time control entries
     */
    void get_time_control_entries(std::vector<TimeControlEntry>* timeControlEntries);

    /**
     * @brief Get the Log Entries stored on the esp. Only available after executing retreiveLogEntries.
     *
     * @param requestedLogEntries list to store the returned log entries
     */
    void get_log_entries(std::vector<LogEntry>* requestedLogEntries);

    /**
     * @brief Request the lock via BLE to send the log entries
     *
     * @param startIndex Startindex of first log msg to be send
     * @param count The number of log entries to be read, starting at the specified start index.
     * @param sortOrder The desired sort order
     * @param totalCount true if a Log Entry Count is requested from the lock
     */
    CmdResult retrieve_log_entries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder,
                                      const bool totalCount);

    /**
     * @brief Retrieve information about an accessory
     *
     * @param accessoryType The accessory type to retrieve information about
     */
    CmdResult get_accessory_info(const uint8_t accessoryType);

    /**
     * @brief Scan for WiFi networks
     *
     * @param scanDurationSeconds Amount of seconds to scan for WiFi networks
     */
    CmdResult scan_wifi(uint8_t scanDurationSeconds = 10);

    /**
     * @brief Get the Wifi scan entries stored on the esp (after executing scan_wifi)
     *
     * @param wifiScanEntries list to store the returned Wifi scan entries
     */
    void get_wifi_scan_entries(std::vector<WifiScanEntry>* wifiScanEntries);

    /**
     * @brief Returns battery critical state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool is_battery_critical();

    /**
     * @brief Returns door sensor battery critical state in case this is supported
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool is_door_sensor_battery_critical();
    
    /**
     * @brief Returns keypad battery critical state in case this is supported
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool is_keypad_battery_critical();

    /**
     * @brief Returns battery charging state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if charging
     */
    bool is_battery_charging();

    /**
     * @brief Returns battery charge percentage state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return percentage
     */
    uint8_t get_battery_perc();

    /**
     * @brief Get the Last Error code received from the lock
     */
    const ErrorCode get_last_error() const;

    virtual void log_error_code(uint8_t error_code) override;

    // How long a previously-fetched Config/AdvancedConfig may be reused by
    // begin_config_write()/begin_advanced_config_write() instead of fetching a fresh one
    // before writing a single changed field back. Trades a small risk of overwriting a
    // change made elsewhere (the Nuki app, another authorized device) in the meantime for
    // fewer BLE round-trips on frequently-changed settings. 0 always fetches fresh.
    void set_config_cache_ttl(uint32_t config_cache_ttl_ms) { this->config_cache_ttl_ms_ = config_cache_ttl_ms; }

  protected:
    void handle_return_message(Command returnCode, unsigned char* data, uint16_t dataLen) override;

    CmdResult set_config(NewConfig newConfig);
    CmdResult set_from_config(const Config config);
    CmdResult set_advanced_config(NewAdvancedConfig newAdvancedConfig);
    void create_new_config(const Config* oldConfig, NewConfig* newConfig);
    void create_new_advanced_config(const AdvancedConfig* oldConfig, NewAdvancedConfig* newConfig);
    CmdResult set_from_advanced_config(const AdvancedConfig config);

    // Read-modify-write helpers shared by every Config/AdvancedConfig setter - see the
    // doc comment on begin_config_write()'s definition in nuki_lock_protocol.cpp.
    CmdResult begin_config_write(Config** out_config);
    CmdResult commit_config_write();
    CmdResult begin_advanced_config_write(AdvancedConfig** out_config);
    CmdResult commit_advanced_config_write();

    KeyTurnerState key_turner_state_;
    BatteryReport battery_report_;
    // No count notification precedes these entries, so the final size isn't known up
    // front - std::vector still avoids std::list's per-node allocation churn.
    std::vector<TimeControlEntry> list_of_time_control_entries_;
    std::vector<LogEntry> list_of_log_entries_;
    std::vector<InternalLogEntry> list_of_internal_log_entries_;
    std::vector<WifiScanEntry> list_of_wifi_scan_entries_;

    Config config_;
    AdvancedConfig advanced_config_;

    // Holds the config snapshot between begin_config_write()/begin_advanced_config_write()
    // fetching it and commit_config_write()/commit_advanced_config_write() sending the
    // modified copy back, across however many repeated calls that takes - separate from
    // config_/advanced_config_ above, which track the lock's last-reported state instead.
    bool config_write_pending_ = false;
    Config config_write_buffer_;
    bool advanced_config_write_pending_ = false;
    AdvancedConfig advanced_config_write_buffer_;

    // Set whenever config_/advanced_config_ is freshly populated (handle_return_message()'s
    // Command::Config/Command::AdvancedConfig cases) - lets begin_config_write()/
    // begin_advanced_config_write() decide whether that snapshot is still within
    // config_cache_ttl_ms_ and can be reused instead of fetching a fresh one. -1 means
    // "never fetched yet".
    int64_t config_fetched_at_ms_ = -1;
    int64_t advanced_config_fetched_at_ms_ = -1;
    uint32_t config_cache_ttl_ms_ = 0;

    MqttConfig mqtt_config_;
    MqttConfigForMigration mqtt_config_for_migration_;
    WifiConfig wifi_config_;
    AccessoryInfo accessory_info_;
    WifiConfigForMigration wifi_config_for_migration_;
    Keypad2Config keypad2_config_;
    GeneralStatistics general_statistics_;
    DailyStatistics daily_statistics_;
    DoorSensorConfig door_sensor_config_;
};

}  // namespace esphome::nuki_lock