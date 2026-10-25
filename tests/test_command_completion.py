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
  bool debug_nuki_communication_ = false;
  bool notify_registered_ = true;
  bool disconnected = false;
  CommandState nuki_command_state_ = CommandState::CmdSent;
  Command last_msg_code_received_ = Command::Status;
  uint8_t received_status_ = static_cast<uint8_t>(CommandStatus::Accepted);
  uint8_t error_code_ = 0;
  uint8_t challenge_nonce_k_[32] = {};
  int64_t time_now_ = 100;
  uint32_t command_timeout_duration_ = 10000;
};
'''

SCENARIOS = r'''
}
using namespace esphome::nuki_lock;
int main(int argc, char **argv) {
  assert(argc == 2);
  const std::string scenario = argv[1];
  NukiBle ble;
  Action action;
  if (scenario == "complete_without_accept") {
    ble.received_status_ = static_cast<uint8_t>(CommandStatus::Complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    return 0;
  }
  assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  assert(ble.nuki_command_state_ == CommandState::CmdAccepted);
  if (scenario == "wait_for_completion") {
    for (int i = 0; i < 5; ++i) {
      now_ms += 50;
      assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
      assert(!ble.disconnected);
    }
    ble.last_msg_code_received_ = Command::Status;
    ble.received_status_ = static_cast<uint8_t>(CommandStatus::Complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(ble.nuki_command_state_ == CommandState::Idle);
    assert(!ble.disconnected);
  } else if (scenario == "immediate_completion") {
    ble.last_msg_code_received_ = Command::Status;
    ble.received_status_ = static_cast<uint8_t>(CommandStatus::Complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Success);
    assert(!ble.disconnected);
  } else if (scenario == "unrelated_message") {
    ble.last_msg_code_received_ = Command::KeyturnerStates;
    ble.received_status_ = static_cast<uint8_t>(CommandStatus::Complete);
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  } else if (scenario == "duplicate_accept") {
    ble.last_msg_code_received_ = Command::Status;
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::Working);
  } else if (scenario == "timeout") {
    now_ms += 10001;
    assert(ble.cmd_chall_acc_state_machine(action) == CmdResult::TimeOut);
    assert(ble.disconnected);
    assert(ble.nuki_command_state_ == CommandState::Idle);
  } else if (scenario == "error" || scenario == "busy") {
    ble.last_msg_code_received_ = Command::ErrorReport;
    ble.error_code_ = scenario == "busy" ? 69 : 1;
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
        ):
            with self.subTest(scenario=scenario):
                result = subprocess.run(
                    [str(self.binary), scenario], capture_output=True, text=True
                )
                self.assertEqual(result.returncode, 0, result.stderr)
