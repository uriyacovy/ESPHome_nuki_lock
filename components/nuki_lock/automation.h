#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "nuki_lock.h"

namespace esphome {
namespace nuki_lock {

// Actions
template<typename... Ts>
class NukiLockUnpairAction : public Action<Ts...>, public Parented<NukiLockComponent> {
    public:
        void play(const Ts&... x) override { this->parent_->unpair(); }
};

template<typename... Ts>
class NukiLockPairingModeAction : public Action<Ts...>, public Parented<NukiLockComponent> {
    TEMPLATABLE_VALUE(bool, pairing_mode)

    public:
        void play(const Ts&... x) override { this->parent_->set_pairing_mode(this->pairing_mode_.value(x...)); }
};

template<typename... Ts>
class NukiLockSecurityPinAction : public Action<Ts...>, public Parented<NukiLockComponent> {
    TEMPLATABLE_VALUE(uint32_t, new_pin)

    public:
        void play(const Ts&... x) override { this->parent_->set_security_pin(this->new_pin_.value(x...)); }
};

// Callbacks
class PairingModeOnTrigger : public Trigger<> {
    public:
        explicit PairingModeOnTrigger(NukiLockComponent *parent) {
            parent->add_pairing_mode_on_callback([this]() { this->trigger(); });
        }
};

class PairingModeOffTrigger : public Trigger<> {
    public:
        explicit PairingModeOffTrigger(NukiLockComponent *parent) {
            parent->add_pairing_mode_off_callback([this]() { this->trigger(); });
        }
};

class PairedTrigger : public Trigger<> {
    public:
        explicit PairedTrigger(NukiLockComponent *parent) {
            parent->add_paired_callback([this]() { this->trigger(); });
        }
};

} //namespace nuki_lock
} //namespace esphome