"""Exercise the actual command state machine with a fake clock and BLE transport.

Run with: python3 -m unittest discover -s tests -v
Requires a C++ compiler; no ESPHome or connected lock is needed.
"""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]

HARNESS = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include "nuki_ble_constants.h"
#define ESP_LOGD(...) ((void) 0)
#define ESP_LOGW(...) ((void) 0)

namespace esphome::nuki_lock {
static int64_t now_ms = 100;
int64_t esp_timer_get_time() { return now_ms * 1000; }
struct Action {
  Command command = Command::LockAction;
  uint8_t payload[1] = {0};
  uint8_t payloadLen = 1;
};
class NukiBle {
 public:
  template <typename TDeviceAction>
  CmdResult cmd_chall_acc_state_machine(TDeviceAction action);
  void extend_disconnect_timeout() {}
  bool send_encrypted_message(Command, unsigned char *, uint8_t) { return true; }
  bool connected() const { return true; }
  void disconnect() { disconnected = true; }
  void clear_seen_responses_() {
    status_accepted_seen_ = false;
    status_complete_seen_ = false;
    error_report_seen_ = false;
  }
  // What NukiBle::handle_return_message() and its overrides do with a message
  void receive(Command command, uint8_t value = 0) {
    if (command == Command::Status) {
      received_status_ = value;
      if (static_cast<CommandStatus>(value) == CommandStatus::Accepted) status_accepted_seen_ = true;
      if (static_cast<CommandStatus>(value) == CommandStatus::Complete) status_complete_seen_ = true;
    } else if (command == Command::ErrorReport) {
      error_code_ = value;
      error_report_seen_ = true;
    }
    last_msg_code_received_ = command;
  }
  bool debug_nuki_communication_ = false;
  bool notify_registered_ = true;
  bool disconnected = false;
  CommandState nuki_command_state_ = CommandState::CmdSent;
  Command last_msg_code_received_ = Command::Empty;
  uint8_t received_status_ = 0xff;
  bool status_accepted_seen_ = false;
  bool status_complete_seen_ = false;
  bool error_report_seen_ = false;
  uint8_t error_code_ = 0;
  uint8_t challenge_nonce_k_[32] = {};
  int64_t time_now_ = 100;
  uint32_t command_timeout_duration_ = 3000;
  uint32_t action_timeout_duration_ = 10000;
};
'''

SCENARIOS = r'''
}
using namespace esphome::nuki_lock;
int main(int argc, char **argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  const uint8_t accepted = static_cast<uint8_t>(CommandStatus::Accepted);
  const uint8_t complete = static_cast<uint8_t>(CommandStatus::Complete);
  NukiBle ble;
  Action action;
  if (scenario == "complete_without_accept") {
    ble.receive(Command::Status, complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    return 0;
  }
  if (scenario == "refused" || scenario == "refused_busy") {
    // An error instead of "accepted": the command is over, not a wait for the timeout
    ble.receive(Command::ErrorReport, scenario == "refused_busy" ? 69 : 1);
    const auto expected = scenario == "refused_busy" ? CmdResult::Lock_Busy : CmdResult::Failed;
    assert(ble.cmd_chall_acc_state_machine(action) == expected);
    assert(ble.disconnected);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    return 0;
  }
  if (scenario == "nothing_received") {
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
    assert(ble.nuki_command_state_ == CommandState::CmdSent);
    // No "accepted" within the command timeout
    now_ms += 3001;
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::TimeOut);
    assert(ble.disconnected);
    return 0;
  }
  ble.receive(Command::Status, accepted);
  if (scenario == "accept_then_state" || scenario == "accept_complete_between_steps") {
    // The device pushes its state right after "accepted", before the next step looks
    ble.receive(Command::KeyturnerStates);
  }
  if (scenario == "accept_complete_between_steps") {
    ble.receive(Command::Status, complete);
    ble.receive(Command::KeyturnerStates);
  }
  assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  assert(ble.nuki_command_state_ == CommandState::CmdAccepted);
  if (scenario == "wait_for_completion") {
    for (int i = 0; i < 5; ++i) {
      now_ms += 50;
      assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
      assert(!ble.disconnected);
    }
    ble.receive(Command::Status, complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    assert(!ble.disconnected);
  } else if (scenario == "immediate_completion") {
    ble.receive(Command::Status, complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(!ble.disconnected);
  } else if (scenario == "accept_then_state") {
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
    ble.receive(Command::Status, complete);
    ble.receive(Command::KeyturnerStates);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(!ble.disconnected);
  } else if (scenario == "accept_complete_between_steps") {
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    assert(!ble.disconnected);
  } else if (scenario == "unrelated_message") {
    ble.receive(Command::KeyturnerStates);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  } else if (scenario == "duplicate_accept") {
    ble.receive(Command::Status, accepted);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  } else if (scenario == "timeout") {
    // An accepted action gets action_timeout_duration_ to complete, not the command timeout
    now_ms += 3001;
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
    now_ms += 7000;
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::TimeOut);
    assert(ble.disconnected);
    assert(ble.nuki_command_state_ == CommandState::Idle);
  } else if (scenario == "error" || scenario == "busy") {
    ble.receive(Command::ErrorReport, scenario == "busy" ? 69 : 1);
    const auto expected = scenario == "busy" ? CmdResult::Lock_Busy : CmdResult::Failed;
    assert(ble.cmd_chall_acc_state_machine(action) == expected);
    assert(ble.disconnected);
    assert(ble.nuki_command_state_ == CommandState::Idle);
  } else {
    assert(false && "Unknown test scenario");
  }
}
'''


class CommandCompletionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("c++")
        if compiler is None:
            raise unittest.SkipTest("A C++ compiler is required")
        cls.workspace = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.workspace.cleanup)
        directory = Path(cls.workspace.name)
        header = Path(os.environ.get(
            "NUKI_BLE_HEADER", ROOT / "components/nuki_lock/nuki_ble.h"
        )).read_text()
        start = header.index(
            "template <typename TDeviceAction>\n"
            "CmdResult NukiBle::cmd_chall_acc_state_machine"
        )
        method = header[start:header.rindex("}  // namespace")]
        source = directory / "command_completion.cpp"
        source.write_text(HARNESS + method + SCENARIOS)
        cls.binary = directory / "command_completion"
        result = subprocess.run([
            compiler, "-std=gnu++17", "-Wall", "-Wextra", "-Werror", "-Wno-vla",
            "-I", str(ROOT / "components/nuki_lock"), str(source),
            "-o", str(cls.binary),
        ], capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(result.stderr)

    def test_response_sequences(self):
        for scenario in (
            "wait_for_completion", "immediate_completion", "complete_without_accept",
            "unrelated_message", "duplicate_accept", "timeout", "error", "busy",
            "accept_then_state", "accept_complete_between_steps", "refused", "refused_busy",
            "nothing_received",
        ):
            with self.subTest(scenario=scenario):
                result = subprocess.run(
                    [str(self.binary), scenario], capture_output=True, text=True
                )
                self.assertEqual(result.returncode, 0, result.stderr)
