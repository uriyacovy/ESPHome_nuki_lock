#pragma once

#include "NukiConstants.h"
#include "NukiDataTypes.h"

#include <cstring>
#include <cstdint>
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Nuki {
template<typename TDeviceAction>
Nuki::CmdResult NukiBle::executeAction(const TDeviceAction action) {
  if(!isPairedWithLock()) {
    if (debugNukiConnect) {
      logMessage("************************ CHECK PAIRED ************************");
    }
    if (retrieveCredentials()) {
      if (debugNukiConnect) {
        logMessage("Credentials retrieved from preferences, ready for commands");
      }
    } else {
      if (debugNukiConnect) {
        logMessage("Credentials NOT retrieved from preferences, first pair with the lock");
      }
      return Nuki::CmdResult::NotPaired;
    }
  }

  if (debugNukiCommunication) {
    logMessageVar("Start executing", (unsigned int)action.command);
  }

  while (1) {
    extendDisconnectTimeout();
      
    Nuki::CmdResult result;
    if (action.cmdType == Nuki::CommandType::Command) {
      result = cmdStateMachine(action);
    }
    else if (action.cmdType == Nuki::CommandType::CommandWithChallenge) {
      result = cmdChallStateMachine(action);
    }
    else if (action.cmdType == Nuki::CommandType::CommandWithChallengeAndAccept) {
      result = cmdChallAccStateMachine(action);
    }
    else if (action.cmdType == Nuki::CommandType::CommandWithChallengeAndPin) {
      result = cmdChallStateMachine(action, true);
    }
    else {
      logMessage("Unknown cmd type", 2);        
      disconnect();
      return Nuki::CmdResult::Failed;
    }
    if (result != Nuki::CmdResult::Working) {
      if (result == Nuki::CmdResult::Error || result == Nuki::CmdResult::Failed) {
        disconnect();
      }
      return result;
    }
    #ifndef NUKI_NO_WDT_RESET
    esp_task_wdt_reset();
    #endif
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return Nuki::CmdResult::Failed;
}

template <typename TDeviceAction>
Nuki::CmdResult NukiBle::cmdStateMachine(const TDeviceAction action) {
  extendDisconnectTimeout();  
  switch (nukiCommandState) {
    case CommandState::Idle: {
      if (debugNukiCommunication) {
        logMessageVar("************************ SENDING COMMAND ************************", (unsigned int)action.command);
      }
      lastMsgCodeReceived = Command::Empty;

      if (sendEncryptedMessage(Command::RequestData, action.payload, action.payloadLen)) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::CmdSent;
      } else {
        if (debugNukiCommunication) {
          logMessage("************************ SENDING COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ COMMAND FAILED TIMEOUT************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived != Command::ErrorReport && lastMsgCodeReceived != Command::Empty) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND DONE ************************");
        }
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Success;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode != 69) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode == 69) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND FAILED LOCK BUSY ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Lock_Busy;
      }
    }
    break;
    default: {
      logMessage("Unknown request command state", 2);
      disconnect();
      return Nuki::CmdResult::Failed;
      break;
    }
  }
  return Nuki::CmdResult::Working;
}

template <typename TDeviceAction>
Nuki::CmdResult NukiBle::cmdChallStateMachine(const TDeviceAction action, const bool sendPinCode) {
  extendDisconnectTimeout();
  switch (nukiCommandState) {
    case CommandState::Idle: {
      if (debugNukiCommunication) {
        logMessage("************************ SENDING CHALLENGE ************************");
      }
      lastMsgCodeReceived = Command::Empty;
      unsigned char payload[sizeof(Command)] = {0x04, 0x00};  //challenge

      if (sendEncryptedMessage(Command::RequestData, payload, sizeof(Command))) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::ChallengeSent;
      } else {
        if (debugNukiCommunication) {
          logMessage("************************ SENDING CHALLENGE FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      }
      break;
    }
    case CommandState::ChallengeSent: {
      if (debugNukiCommunication) {
        logMessage("************************ RECEIVING CHALLENGE RESPONSE************************");
      }
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ COMMAND FAILED TIMEOUT ************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived == Command::Challenge) {
        nukiCommandState = CommandState::ChallengeRespReceived;
        lastMsgCodeReceived = Command::Empty;
      }
      break;
    }
    case CommandState::ChallengeRespReceived: {
      if (debugNukiCommunication) {
        logMessageVar("************************ SENDING COMMAND ************************", (unsigned int)action.command);
      }
      lastMsgCodeReceived = Command::Empty;
      crcCheckOke = false;
      //add received challenge nonce to payload
      uint8_t payloadLen = action.payloadLen + sizeof(challengeNonceK);
      if (sendPinCode) {
        if (isLockUltra()) {
          payloadLen = payloadLen + 4;  
        } else {
          payloadLen = payloadLen + 2;
        }
      }
      unsigned char payload[payloadLen];
      memcpy(payload, action.payload, action.payloadLen);
      memcpy(&payload[action.payloadLen], challengeNonceK, sizeof(challengeNonceK));
      if (sendPinCode) {
        if (isLockUltra()) {
          memcpy(&payload[action.payloadLen + sizeof(challengeNonceK)], &ultraPinCode, 4);
        } else {
          memcpy(&payload[action.payloadLen + sizeof(challengeNonceK)], &pinCode, 2);
        }        
      }

      if (sendEncryptedMessage(action.command, payload, payloadLen)) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::CmdSent;
      } else {
        if (debugNukiCommunication) {
          logMessage("************************ SENDING COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if (debugNukiCommunication) {
        logMessage("************************ RECEIVING DATA ************************");
      }
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ COMMAND FAILED TIMEOUT ************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode != 69) {
        if (debugNukiCommunication) {
            logMessage("************************ SENDING COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode == 69) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND FAILED LOCK BUSY ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Lock_Busy;
      } else if (crcCheckOke) {
        if (debugNukiCommunication) {
          logMessage("************************ DATA RECEIVED ************************");
        }
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::Success;
      }
      break;
    }
    default:
      logMessage("Unknown request command state", 2);
      disconnect();
      return Nuki::CmdResult::Failed;
      break;
  }
  return Nuki::CmdResult::Working;
}

template <typename TDeviceAction>
Nuki::CmdResult NukiBle::cmdChallAccStateMachine(const TDeviceAction action) {
  extendDisconnectTimeout();
  switch (nukiCommandState) {
    case CommandState::Idle: {
      if (debugNukiCommunication) {
        logMessage("************************ SENDING CHALLENGE ************************");
      }
      lastMsgCodeReceived = Command::Empty;
      unsigned char payload[sizeof(Command)] = {0x04, 0x00};  //challenge

      if (sendEncryptedMessage(Command::RequestData, payload, sizeof(Command))) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::ChallengeSent;
      } else {
        if (debugNukiCommunication) {
          logMessage("************************ SENDING CHALLENGE FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      }
      break;
    }
    case CommandState::ChallengeSent: {
      if (debugNukiCommunication) {
        logMessage("************************ RECEIVING CHALLENGE RESPONSE************************");
      }
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ COMMAND FAILED TIMEOUT ************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived == Command::Challenge) {
        nukiCommandState = CommandState::ChallengeRespReceived;
        lastMsgCodeReceived = Command::Empty;
      }
      break;
    }
    case CommandState::ChallengeRespReceived: {
      if (debugNukiCommunication) {
        logMessageVar("************************ SENDING COMMAND ************************", (unsigned int)action.command);
      }
      lastMsgCodeReceived = Command::Empty;
      //add received challenge nonce to payload
      uint8_t payloadLen = action.payloadLen + sizeof(challengeNonceK);
      unsigned char payload[payloadLen];
      memcpy(payload, action.payload, action.payloadLen);
      memcpy(&payload[action.payloadLen], challengeNonceK, sizeof(challengeNonceK));

      if (sendEncryptedMessage(action.command, payload, action.payloadLen + sizeof(challengeNonceK))) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::CmdSent;
      } else {
        if (debugNukiCommunication) {
          logMessage("************************ SENDING COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      }
      break;
    }
    case CommandState::CmdSent: {
      if (debugNukiCommunication) {
        logMessage("************************ RECEIVING ACCEPT ************************");
      }
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ ACCEPT FAILED TIMEOUT ************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived == Command::Status && (CommandStatus)receivedStatus == CommandStatus::Accepted) {
        timeNow = (esp_timer_get_time() / 1000);
        nukiCommandState = CommandState::CmdAccepted;
        lastMsgCodeReceived = Command::Empty;
      } else if (lastMsgCodeReceived == Command::Status && (CommandStatus)receivedStatus == CommandStatus::Complete) {
        //accept was skipped on lock because ie unlock command when lock allready unlocked?
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND SUCCESS (SKIPPED) ************************");
        }
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Success;
      }
      break;
    }
    case CommandState::CmdAccepted: {
      if (debugNukiCommunication) {
        logMessage("************************ RECEIVING COMPLETE ************************");
      }
      if ((esp_timer_get_time() / 1000) - timeNow > commandTimeoutDuration) {
        logMessage("************************ COMMAND FAILED TIMEOUT ************************", 2);
        disconnect();
        nukiCommandState = CommandState::Idle;
        return Nuki::CmdResult::TimeOut;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode != 69) {
        if (debugNukiCommunication) {
            logMessage("************************ SENDING COMMAND FAILED ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Failed;
      } else if (lastMsgCodeReceived == Command::ErrorReport && errorCode == 69) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND FAILED LOCK BUSY ************************");
        }
        disconnect();
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Lock_Busy;
      } else if ((CommandStatus)lastMsgCodeReceived == CommandStatus::Complete) {
        if (debugNukiCommunication) {
          logMessage("************************ COMMAND SUCCESS ************************");
        }
        nukiCommandState = CommandState::Idle;
        lastMsgCodeReceived = Command::Empty;
        return Nuki::CmdResult::Success;
      }
      break;
    }
    default:
      logMessage("Unknown request command state", 2);
      disconnect();
      return Nuki::CmdResult::Failed;
      break;
  }
  return Nuki::CmdResult::Working;
}
  
}