#pragma once

#include "NukiBle.h"
#include "NukiLockConstants.h"
#include "NukiLockUtils.h"

#include <list>
#include <cstdint>
#include <cstring>
#include <string>

#include "esp_log.h"


namespace NukiLock {
class NukiLock : public Nuki::NukiBle {
  public:
    NukiLock(const std::string& deviceName, const uint32_t deviceId);


    /**
     * @brief Sends lock action cmd via BLE to the lock
     *
     * @param lockAction
     * @param nukiAppId 0 = App, 1 = Bridge, 2 = Fob, 3 = Keypad
     * @param flags optional
     * @param nameSuffix optional
     * @param nameSuffixLen len of nameSuffix if used ('\0' included, maximum 19)
     * @return Nuki::CmdResult
     */
    Nuki::CmdResult lockAction(const LockAction lockAction, const uint32_t nukiAppId = 1, const uint8_t flags = 0,
                              const char* nameSuffix = nullptr, const uint8_t nameSuffixLen = 0);

    /**
     * @brief Send a keypad action entry to the lock via BLE
     * @param source 0x00 = arrow key, 0x01 = code
     * @param code The code that has been entered on the keypad
     * @param keypadAction The action to be executed
     */
    Nuki::CmdResult keypadAction(KeypadActionSource source, uint32_t code, KeypadAction keypadAction);

    /**
     * @brief Requests keyturner state from Lock via BLE
     *
     * @param retrievedKeyTurnerState Nuki api based datatype to store the retrieved keyturnerstate
     */
    Nuki::CmdResult requestKeyTurnerState(KeyTurnerState* retrievedKeyTurnerState);

    /**
     * @brief Gets the last keyturner state stored on the esp
     *
     * @param retrievedKeyTurnerState Nuki api based datatype to store the retrieved keyturnerstate
     */
    void retrieveKeyTunerState(KeyTurnerState* retrievedKeyTurnerState);

    
    /**
     * @brief Requests battery status from Lock via BLE
     *
     * @param retrievedBatteryReport Nuki api based datatype to store the retrieved battery status
     */
    Nuki::CmdResult requestBatteryReport(BatteryReport* retrievedBatteryReport);


    /**
     * @brief Requests config from Lock via BLE
     *
     * @param retrievedConfig Nuki api based datatype to store the retrieved config
     */
    Nuki::CmdResult requestConfig(Config* retrievedConfig);

    /**
     * @brief Requests advanced config from Lock via BLE
     *
     * @param retrievedAdvancedConfig Nuki api based datatype to store the retrieved advanced config
     */
    Nuki::CmdResult requestAdvancedConfig(AdvancedConfig* retrievedAdvancedConfig);

    /**
     * @brief Request the lock via BLE to send the internal log entries
     *
     * @param startIndex Startindex of first log msg to be send
     * @param count The number of log entries to be read, starting at the specified start index.
     * @param sortOrder The desired sort order
     * @param totalCount true if a Log Entry Count is requested from the lock
     */
    Nuki::CmdResult retrieveInternalLogEntries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder, bool const totalCount);

    /**
     * @brief Get the Internal Log Entries stored on the esp. Only available after executing retrieveInternalLogEntries.
     *
     * @param requestedInternalLogEntries list to store the returned internal log entries
     */
    void getInternalLogEntries(std::list<InternalLogEntry>* requestedInternalLogEntries);

    /**
     * @brief Gets the current config from the lock, updates the name parameter and sends the
     * new config to the lock via BLE
     *
     * @param name max 32 character name
     */
    Nuki::CmdResult setName(const std::string& name);

    /**
     * @brief Gets the current config from the lock, updates the latitude parameter and sends the new
     * config to the lock via BLE
     *
     * @param degrees the desired latitude
     */
    Nuki::CmdResult setLatitude(const float degrees);

    /**
     * @brief Gets the current config from the lock, updates the longitude parameter and sends the new
     * config to the lock via BLE
     *
     * @param degrees the desired longitude
     */
    Nuki::CmdResult setLongitude(const float degrees);

    /**
     * @brief Gets the current config from the lock, updates the auto unlatch parameter and sends the new
     * config to the lock via BLE
     *
     * @param enable true if auto unlatch should be enabled in general.
     */
    Nuki::CmdResult enableAutoUnlatch(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the given fob action parameter and sends the new
     * config to the lock via BLE
     *
     * @param fobActionNr the fob action to change (1 = single press, 2 = double press, 3 = triple press)
     * @param fobAction the desired fob action setting
     */
    Nuki::CmdResult setFobAction(const uint8_t fobActionNr, const uint8_t fobAction);

    /**
     * @brief Gets the current config from the lock, updates the dst parameter and sends the new
     * config to the lock via BLE
     *
     * @param enable The desired daylight saving time mode. false disabled, true european
     */
    Nuki::CmdResult enableDst(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the timezone offset parameter and
     * sends the new config to the lock via BLE
     *
     * @param minutes The timezone offset (UTC) in minutes
     */
    Nuki::CmdResult setTimeZoneOffset(const int16_t minutes);

    /**
     * @brief Gets the current config from the lock, updates the timezone id parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeZoneId 	The id of the current timezone or 0xFFFF if timezones are not supported
     */
    Nuki::CmdResult setTimeZoneId(const TimeZoneId timeZoneId);

    /**
     * @brief Gets the current config from the lock, updates the enable button parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if button enabled
     */
    Nuki::CmdResult enableButton(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the unlocked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the unlocked position
     */
    Nuki::CmdResult setUnlockedPositionOffsetDegrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the locked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the locked position
     */
    Nuki::CmdResult setLockedPositionOffsetDegrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the single locked position offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the single locked position
     */
    Nuki::CmdResult setSingleLockedPositionOffsetDegrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the unlocked to locked transition offset degrees parameter and sends the
     * new config to the lock via BLE
     *
     * @param degrees the desired offset that alters the position where transition from unlocked to locked happens
     */
    Nuki::CmdResult setUnlockedToLockedTransitionOffsetDegrees(const int16_t degrees);

    /**
     * @brief Gets the current config from the lock, updates the lock n go timeout parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeout the desired timeout for lock ‘n’ go
     */
    Nuki::CmdResult setLockNgoTimeout(const uint8_t timeout);

    /**
     * @brief Gets the current config from the lock, updates the detached cylinder parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if detached cylinder enabled (Flag that indicates that the inner side of the used cylinder is detached from
     * the outer side and therefore the Smart Lock won’t recognize if someone operates the door by using a key)
     */
    Nuki::CmdResult enableDetachedCylinder(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the unlatch duration parameter and sends the
     * new config to the lock via BLE
     *
     * @param duration the desired duration in seconds for holding the latch in unlatched position
     */
    Nuki::CmdResult setUnlatchDuration(const uint8_t duration);

    /**
     * @brief Gets the current config from the lock, updates the auto lock timeout parameter and sends the
     * new config to the lock via BLE
     *
     * @param timeout the desired timeout until the smart lock relocks itself after it has been unlocked
     */
    Nuki::CmdResult setAutoLockTimeOut(const uint8_t timeout);

    /**
     * @brief Gets the current config from the lock, updates the night mode parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode enabled
     */
    Nuki::CmdResult enableNightMode(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the night mode start time parameter and sends the
     * new config to the lock via BLE
     *
     * @param starttime the desired night mode start time
     */
    Nuki::CmdResult setNightModeStartTime(unsigned char starttime[2]);

    /**
     * @brief Gets the current config from the lock, updates the night mode end time parameter and sends the
     * new config to the lock via BLE
     *
     * @param endtime the desired night mode end time
     */
    Nuki::CmdResult setNightModeEndTime(unsigned char endtime[2]);

    /**
     * @brief Gets the current config from the lock, updates the night mode auto lock parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode auto lock enabled
     */
    Nuki::CmdResult enableNightModeAutoLock(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the night mode auto unlock parameter and sends the
     * new config to the lock via BLE
     *
     * @param disable true if night mode auto unlock disabled
     */
    Nuki::CmdResult disableNightModeAutoUnlock(const bool disable);

    /**
     * @brief Gets the current config from the lock, updates the night mode immediate lock on start parameter and sends the
     * new config to the lock via BLE
     *
     * @param enable true if night mode immediate lock on start enabled
     */
    Nuki::CmdResult enableNightModeImmediateLockOnStart(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the single button press action
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    Nuki::CmdResult setSingleButtonPressAction(const ButtonPressAction action);

    /**
     * @brief Gets the current advanced config from the lock, updates the double button press action
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a double button press
     */
    Nuki::CmdResult setDoubleButtonPressAction(const ButtonPressAction action);

    /**
     * @brief Gets the current advanced config from the lock, updates the battery type parameter and
     * sends the new advanced config to the lock via BLE
     *
     * @param type 	The type of the batteries present in the smart lock.
     */
    Nuki::CmdResult setBatteryType(const BatteryType type);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable battery type
     * detection parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if the automatic detection of the battery type is enabled
     */
    Nuki::CmdResult enableAutoBatteryTypeDetection(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the disable autounlock
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param disable true if auto unlock should be disabled in general.
     */
    Nuki::CmdResult disableAutoUnlock(const bool disable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable autolock
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if auto lock should be enabled in general.
     */
    Nuki::CmdResult enableAutoLock(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable immediate
     * autolock parameter and sends the new advanced config to the lock via BLE
     *
     * @param enable true if auto lock should be performed immediately after the door has
     * been closed (requires active door sensor)
     */
    Nuki::CmdResult enableImmediateAutoLock(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable auto update
     * parameter and sends the new advanced config to the lock via BLE
     * (Updating the firmware requires the Nuki app. CAUTION: updating FW could cause breaking changes)
     *
     * @param enable true if automatic firmware updates should be enabled
     */
    Nuki::CmdResult enableAutoUpdate(const bool enable);

    /**
     * @brief Gets the current advanced config from the lock, updates the motor speed
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    Nuki::CmdResult setMotorSpeed(const MotorSpeed speed);

    /**
     * @brief Gets the current advanced config from the lock, updates the enable slow speed during NightMode
     * parameter and sends the new advanced config to the lock via BLE
     *
     * @param action the deired action for a single button press
     */
    Nuki::CmdResult enableSlowSpeedDuringNightMode(const bool enable);

    /**
     * @brief Sets the lock ability to pair with other devices (can be used to prevent unauthorized pairing)
     * Gets the current config from the lock, updates the pairing parameter and sends the new config to the lock via BLE
     * (CAUTION: if pairing is set to false and credentials are deleted a factory reset of the lock needs to be performed
     * before pairing is possible again)
     *
     * @param enable true if allowed to pair with other devices
     */
    Nuki::CmdResult enablePairing(const bool enable);

    /**
     * @brief Gets the lock current config wrt pairing with other devices
     */
    bool pairingEnabled();

    /**
     * @brief Gets the current config from the lock, updates the whether or not the flashing
     * LED should be enabled to signal an unlocked door. And sends the new config to the lock via BLE
     *
     * @param enable true if led enabled
     */
    Nuki::CmdResult enableLedFlash(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the LED brightness parameter and
     * sends the new config to the lock via BLE
     *
     * @param level The LED brightness level. Possible values are 0 to 5 0 = off, …, 5 = max
     */
    Nuki::CmdResult setLedBrightness(const uint8_t level);

    /**
     * @brief Gets the current config from the lock, updates the LED brightness parameter
     * and sends the new config to the lock via BLE
     *
     * @param enable true if only a single lock should be performed
     */
    Nuki::CmdResult enableSingleLock(const bool enable);

    /**
     * @brief Gets the current config from the lock, updates the advertising frequency parameter
     * and sends the new config to the lock via BLE
     *
     * @param mode 0x00 Automatic, 0x01 Normal, 0x02 Slow, 0x03 Slowest (~400ms till ~1s)
     */
    Nuki::CmdResult setAdvertisingMode(const AdvertisingMode mode);

    /**
     * @brief Sends a new time(d) control entry via BLE to the lock.
     * This entry is independant of keypad or authorization entries, it will execute the
     * defined action at the defined time in the newTimeControlEntry
     *
     * @param newTimecontrolEntry Nuki api based datatype to send
     */
    Nuki::CmdResult addTimeControlEntry(NewTimeControlEntry newTimecontrolEntry);

    /**
     * @brief Sends an updated time(d) control entry via BLE to the lock.
     * (see addTimeControlEntry())
     *
     * @param TimeControlEntry Nuki api based datatype to send.
     * The ID can be retrieved via retrieveTimeControlEntries()
     */
    Nuki::CmdResult updateTimeControlEntry(TimeControlEntry TimeControlEntry);

    /**
     * @brief Deletes a time(d) control entry via BLE to the lock.
     * (see addTimeControlEntry())
     *
     * @param entryId The ID to be deleted, can be retrieved via retrieveTimeControlEntries()
     */
    Nuki::CmdResult removeTimeControlEntry(uint8_t entryId);

    /**
     * @brief Request the lock via BLE to send the existing time control entries
     *
     */
    Nuki::CmdResult retrieveTimeControlEntries();

    /**
     * @brief Get the time control entries stored on the esp (after executing retrieveTimeControlEntries())
     *
     * @param timeControlEntries list to store the returned time control entries
     */
    void getTimeControlEntries(std::list<TimeControlEntry>* timeControlEntries);

    /**
     * @brief Get the Log Entries stored on the esp. Only available after executing retreiveLogEntries.
     *
     * @param requestedLogEntries list to store the returned log entries
     */
    void getLogEntries(std::list<LogEntry>* requestedLogEntries);

    /**
     * @brief Request the lock via BLE to send the log entries
     *
     * @param startIndex Startindex of first log msg to be send
     * @param count The number of log entries to be read, starting at the specified start index.
     * @param sortOrder The desired sort order
     * @param totalCount true if a Log Entry Count is requested from the lock
     */
    Nuki::CmdResult retrieveLogEntries(const uint32_t startIndex, const uint16_t count, const uint8_t sortOrder,
                                      const bool totalCount);

    /**
     * @brief Retrieve information about an accessory
     *
     * @param accessoryType The accessory type to retrieve information about
     */
    Nuki::CmdResult getAccessoryInfo(const uint8_t accessoryType);

    /**
     * @brief Scan for WiFi networks
     *
     * @param scanDurationSeconds Amount of seconds to scan for WiFi networks
     */
    Nuki::CmdResult scanWifi(uint8_t scanDurationSeconds = 10);

    /**
     * @brief Get the Wifi scan entries stored on the esp (after executing scanWifi)
     *
     * @param wifiScanEntries list to store the returned Wifi scan entries
     */
    void getWifiScanEntries(std::list<WifiScanEntry>* wifiScanEntries);

    /**
     * @brief Returns battery critical state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool isBatteryCritical();

    /**
     * @brief Returns door sensor battery critical state in case this is supported
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool isDoorSensorBatteryCritical();
    
    /**
     * @brief Returns keypad battery critical state in case this is supported
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if critical
     */
    bool isKeypadBatteryCritical();

    /**
     * @brief Returns battery charging state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return true if charging
     */
    bool isBatteryCharging();

    /**
     * @brief Returns battery charge percentage state parsed from the battery state byte (battery critical byte)
     *
     * Note that `retrieveOpenerState()` needs to be called first to retrieve the needed data
     *
     * @return percentage
     */
    uint8_t getBatteryPerc();

    /**
     * @brief Get the Last Error code received from the lock
     */
    const ErrorCode getLastError() const;

    virtual void logErrorCode(uint8_t errorCode) override;

  protected:
    void handleReturnMessage(Command returnCode, unsigned char* data, uint16_t dataLen) override;


  private:
    Nuki::CmdResult setConfig(NewConfig newConfig);
    Nuki::CmdResult setFromConfig(const Config config);
    Nuki::CmdResult setAdvancedConfig(NewAdvancedConfig newAdvancedConfig);
    void createNewConfig(const Config* oldConfig, NewConfig* newConfig);
    void createNewAdvancedConfig(const AdvancedConfig* oldConfig, NewAdvancedConfig* newConfig);
    Nuki::CmdResult setFromAdvancedConfig(const AdvancedConfig config);

    KeyTurnerState keyTurnerState;
    BatteryReport batteryReport;
    std::list<TimeControlEntry> listOfTimeControlEntries;
    std::list<LogEntry> listOfLogEntries;
    std::list<InternalLogEntry> listOfInternalLogEntries;
    std::list<WifiScanEntry> listOfWifiScanEntries;

    Config config;
    AdvancedConfig advancedConfig;
    MqttConfig mqttConfig;
    MqttConfigForMigration mqttConfigForMigration;
    WifiConfig wifiConfig;
    AccessoryInfo accessoryInfo;
    WifiConfigForMigration wifiConfigForMigration;
    Keypad2Config keypad2Config;
    GeneralStatistics generalStatistics;
    DailyStatistics dailyStatistics;
    DoorSensorConfig doorSensorConfig;
};

}