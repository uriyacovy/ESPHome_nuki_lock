// Hybrid mode: combines the lock's own official MQTT implementation (over WiFi/Thread) with
// BLE, like Nuki Hub does (https://github.com/technyon/nuki_hub/blob/master/HYBRID.md).
//
// The lock publishes its state changes to "nuki/<NUKI ID in hex>/..." itself, so they arrive
// immediately without a BLE connection - also when paired as app, where the BLE advertisement
// doesn't signal state changes. Every final state change still triggers a BLE status update
// for the details MQTT doesn't provide. Lock actions can optionally be sent over MQTT too
// ("lockAction" topic, needs "Allow locking" in the Nuki app's MQTT settings) and are retried
// over BLE if the lock doesn't confirm them in time. While the lock's MQTT connection is
// offline, everything falls back to BLE.

#include "esphome/core/defines.h"

#ifdef USE_NUKI_LOCK_HYBRID

#include "nuki_lock.h"

#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/components/mqtt/mqtt_client.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace esphome::nuki_lock {

static const char *const TAG = "nuki_lock.hybrid";

void NukiLockComponent::hybrid_set_topic_prefix(const char *topic_prefix) {
    this->hybrid_topic_prefix_len_ = std::min(strlen(topic_prefix), HYBRID_TOPIC_PREFIX_MAX_LEN);
    memcpy(this->hybrid_topic_prefix_, topic_prefix, this->hybrid_topic_prefix_len_);
    this->hybrid_topic_prefix_[this->hybrid_topic_prefix_len_] = '\0';
    snprintf(this->hybrid_lock_action_topic_, sizeof(this->hybrid_lock_action_topic_), "%s/lockAction", this->hybrid_topic_prefix_);
}

void NukiLockComponent::hybrid_on_config() {
    if (this->hybrid_topic_prefix_len_ == 0) {
        // Same topic path as the lock uses: "nuki/" + Nuki ID in upper case hex
        char prefix[sizeof("nuki/FFFFFFFF")];
        snprintf(prefix, sizeof(prefix), "nuki/%X", (unsigned int)this->nuki_lock_config_.nukiId);
        this->hybrid_set_topic_prefix(prefix);
        ESP_LOGI(TAG, "Using official MQTT topics %s/...", prefix);
    }
    this->hybrid_subscribe();
}

void NukiLockComponent::hybrid_subscribe() {
    if (this->hybrid_subscribed_ || mqtt::global_mqtt_client == nullptr) {
        return;
    }

    // ESPHome keeps the subscription (it stores the topic as std::string, so this one copy
    // is unavoidable) and renews it after every reconnect. Retained values (the lock
    // publishes its state retained) arrive right after subscribing.
    char topic[HYBRID_TOPIC_PREFIX_MAX_LEN + sizeof("/+")];
    snprintf(topic, sizeof(topic), "%s/+", this->hybrid_topic_prefix_);
    mqtt::global_mqtt_client->subscribe(topic,
        [this](const std::string &topic, const std::string &payload) {
            // Only the part after "<prefix>/" is of interest, compare it in place
            if (topic.size() > this->hybrid_topic_prefix_len_ + 1) {
                this->hybrid_on_message(topic.c_str() + this->hybrid_topic_prefix_len_ + 1, payload.c_str());
            }
        }, 1);
    this->hybrid_subscribed_ = true;
}

void NukiLockComponent::hybrid_loop() {
    // Without a configured prefix, subscribing waits for the Nuki ID from the config, see
    // hybrid_on_config(). With one, subscribe right away.
    if (!this->hybrid_subscribed_ && this->hybrid_topic_prefix_len_ > 0) {
        this->hybrid_subscribe();
    }

    if (this->hybrid_action_pending_ &&
        millis() - this->hybrid_action_sent_time_ > this->hybrid_fallback_timeout_millis_) {
        this->hybrid_fallback_to_ble("not confirmed in time");
    }

    // While the lock's MQTT connection is offline, poll the state over BLE now and then:
    // when paired as app, BLE advertisements don't signal state changes.
    if (this->hybrid_subscribed_ && !this->hybrid_connected_ && this->hybrid_offline_update_interval_ > 0 &&
        millis() - this->hybrid_last_offline_update_time_ > this->hybrid_offline_update_interval_ * 1000) {
        this->hybrid_last_offline_update_time_ = millis();
        ESP_LOGD(TAG, "Official MQTT offline, requesting status over BLE");
        this->status_update_ = true;
    }
}

bool NukiLockComponent::hybrid_send_lock_action(LockAction action) {
    if (!this->hybrid_send_actions_ || !this->hybrid_connected_ ||
        mqtt::global_mqtt_client == nullptr || !mqtt::global_mqtt_client->is_connected()) {
        return false;
    }

    // The lockAction topic only takes the action itself, flags and name suffix need BLE
    if (this->force_ || this->auto_unlock_ || !this->name_suffix_.empty()) {
        return false;
    }

    char payload[4];
    const int payload_len = snprintf(payload, sizeof(payload), "%u", (unsigned int)action);
    if (!mqtt::global_mqtt_client->publish(this->hybrid_lock_action_topic_, payload, payload_len, 1, false)) {
        ESP_LOGW(TAG, "Publishing lock action failed, using BLE");
        return false;
    }

    ESP_LOGI(TAG, "Lock action %u sent over official MQTT", (unsigned int)action);
    this->publish_assumed_lock_state(action);

    if (this->hybrid_ble_fallback_) {
        this->hybrid_action_pending_ = true;
        this->hybrid_pending_action_ = action;
        this->hybrid_action_sent_time_ = millis();
    }
    return true;
}

void NukiLockComponent::hybrid_fallback_to_ble(const char *reason) {
    this->hybrid_action_pending_ = false;
    ESP_LOGW(TAG, "Lock action %u over official MQTT %s, retrying over BLE",
        (unsigned int)this->hybrid_pending_action_, reason);
    this->lock_action_ = this->hybrid_pending_action_;
    this->action_attempts_ = this->command_retries_;
}

void NukiLockComponent::hybrid_set_connected(bool connected) {
    if (connected != this->hybrid_connected_) {
        ESP_LOGI(TAG, "Official MQTT of the lock is %s", connected ? "online" : "offline");
    }
    this->hybrid_connected_ = connected;
    if (!connected) {
        // Start the offline polling interval from now
        this->hybrid_last_offline_update_time_ = millis();
        if (this->hybrid_action_pending_) {
            this->hybrid_fallback_to_ble("lost the lock's connection");
        }
    }

    #ifdef USE_BINARY_SENSOR
    if (this->hybrid_connected_binary_sensor_ != nullptr) {
        this->hybrid_connected_binary_sensor_->publish_state(connected);
    }
    #endif
}

void NukiLockComponent::hybrid_on_message(const char *key, const char *payload) {
    const bool flag = strcmp(payload, "true") == 0;
    const optional<int> value = parse_number<int>(payload);

    ESP_LOGV(TAG, "Official MQTT %s: %s", key, payload);

    char str[50] = {0};

    if (strcmp(key, "connected") == 0) {
        this->hybrid_set_connected(flag);
    } else if (strcmp(key, "state") == 0) {
        if (!value.has_value()) {
            return;
        }
        LockState state = (LockState)*value;
        lock_state_to_string(state, str);
        ESP_LOGI(TAG, "Lock state: %s (%d)", str, *value);
        this->publish_state(nuki_to_lock_state(state));
        this->hybrid_action_pending_ = false;

        // The topic only carries the lock state; fetch the rest (battery, door sensor, ...)
        // over BLE once the lock reached its final state.
        if (state != LockState::Locking && state != LockState::Unlocking && state != LockState::Calibration) {
            this->status_update_ = true;
        }
    } else if (strcmp(key, "commandResponse") == 0) {
        if (!value.has_value()) {
            return;
        }
        if (*value == 0) {
            this->hybrid_action_pending_ = false;
        } else {
            ESP_LOGW(TAG, "Lock action over official MQTT failed (%d)", *value);
            if (this->hybrid_action_pending_) {
                this->hybrid_fallback_to_ble("failed");
            }
        }
    } else if (strcmp(key, "lockActionEvent") == 0) {
        // "<lock action>,<trigger>,<auth id>,<code id>,<context>"
        unsigned int action = 0, trigger = 0, auth_id = 0, code_id = 0, context = 0;
        if (sscanf(payload, "%u,%u,%u,%u,%u", &action, &trigger, &auth_id, &code_id, &context) < 2) {
            return;
        }
        this->hybrid_action_pending_ = false;

        #ifdef USE_TEXT_SENSOR
        if (this->last_lock_action_text_sensor_ != nullptr) {
            lock_action_to_string((LockAction)action, str);
            this->last_lock_action_text_sensor_->publish_state(str);
        }
        if (this->last_lock_action_trigger_text_sensor_ != nullptr) {
            memset(str, 0, sizeof(str));
            trigger_to_string((NukiTrigger)trigger, str);
            this->last_lock_action_trigger_text_sensor_->publish_state(str);
        }
        if (this->last_unlock_user_text_sensor_ != nullptr) {
            if ((NukiTrigger)trigger == NukiTrigger::Manual) {
                this->last_unlock_user_text_sensor_->publish_state("Manual");
            } else if (const char *name = this->get_auth_name(auth_id)) {
                this->last_unlock_user_text_sensor_->publish_state(name);
            }
        }
        #endif

        if (this->send_events_) {
            this->event_log_update_ = true;
        }
    } else if (strcmp(key, "doorsensorState") == 0) {
        if (!value.has_value()) {
            return;
        }
        DoorSensorState door_sensor_state = (DoorSensorState)*value;
        #ifdef USE_BINARY_SENSOR
        if (this->door_sensor_binary_sensor_ != nullptr) {
            if (door_sensor_state != DoorSensorState::Unavailable) {
                this->door_sensor_binary_sensor_->publish_state(nuki_doorsensor_to_binary(door_sensor_state));
            } else {
                this->door_sensor_binary_sensor_->invalidate_state();
            }
        }
        #endif
        #ifdef USE_TEXT_SENSOR
        if (this->door_sensor_state_text_sensor_ != nullptr) {
            door_sensor_state_to_string(door_sensor_state, str);
            this->door_sensor_state_text_sensor_->publish_state(str);
        }
        #endif
    }
    #ifdef USE_BINARY_SENSOR
    else if (strcmp(key, "batteryCritical") == 0) {
        if (this->battery_critical_binary_sensor_ != nullptr) {
            this->battery_critical_binary_sensor_->publish_state(flag);
        }
    } else if (strcmp(key, "batteryCharging") == 0) {
        if (this->battery_charging_binary_sensor_ != nullptr) {
            this->battery_charging_binary_sensor_->publish_state(flag);
        }
    } else if (strcmp(key, "keypadBatteryCritical") == 0) {
        if (this->keypad_battery_critical_binary_sensor_ != nullptr) {
            this->keypad_battery_critical_binary_sensor_->publish_state(flag);
        }
    } else if (strcmp(key, "doorsensorBatteryCritical") == 0) {
        if (this->door_sensor_battery_critical_binary_sensor_ != nullptr) {
            this->door_sensor_battery_critical_binary_sensor_->publish_state(flag);
        }
    }
    #endif
    #ifdef USE_SENSOR
    else if (strcmp(key, "batteryChargeState") == 0) {
        if (this->battery_level_sensor_ != nullptr && value.has_value()) {
            this->battery_level_sensor_->publish_state(*value);
        }
    }
    #endif
}

}  // namespace esphome::nuki_lock

#endif  // USE_NUKI_LOCK_HYBRID
