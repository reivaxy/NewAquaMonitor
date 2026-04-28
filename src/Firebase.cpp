/**
 *  Firebase recording class
 *  Xavier Grosjean 2022
 *  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License
 */

#include "Firebase.h"
#include "HttpsClient.h"
#include <exception>
#include <esp_system.h>
#include <time.h>

static const char* getResetReasonStr() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "Power-on";
    case ESP_RST_SW:        return "Software restart";
    case ESP_RST_PANIC:     return "Panic/exception";
    case ESP_RST_INT_WDT:   return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:  return "Task watchdog";
    case ESP_RST_WDT:       return "Other watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
    case ESP_RST_BROWNOUT:  return "Brownout";
    default:                return "Unknown (USB Upload?)";
  }
}


#define PING_PERIOD 1000 * 60 * 5UL // 5mn

#define URL_MAX_LENGTH_WITHOUT_SECRET 200

Firebase::Firebase(Settings* settings, SensorPCF8563& rtc) : rtc(rtc) {
  this->settings = settings;
  *this->macAddrStr = 0;
  for (int i=0; i < MAX_DIFFERED_MESSAGES_COUNT; i++) {
    differedMessages[i] = NULL;
  }
}

   
void Firebase::init(const char* macAddrStr) {
  log_i("Firebase::init: %s\n", macAddrStr);
  strcpy(this->macAddrStr, macAddrStr);
  lastSendPing = 0;
  if (strlen(getFirebaseUrl()) > 10) {
    initialized = true;
  }
}

void Firebase::disable() {
  log_i("Firebase::disable\n");
  initialized = false;
}

void Firebase::loop() {
  if (!initialized) {
    return ;
  }

  handleDifferedMessages();

  // send a ping every PING_PERIOD
  RTC_DateTime dt = rtc.getDateTime();
  struct tm t = {};
  t.tm_year = dt.getYear() - 1900;
  t.tm_mon  = dt.getMonth() - 1;
  t.tm_mday = dt.getDay();
  t.tm_hour = dt.getHour();
  t.tm_min  = dt.getMinute();
  t.tm_sec  = dt.getSecond();
  unsigned long timeNow = (unsigned long)mktime(&t) * 1000UL;
  if (XUtils::isElapsedDelay(timeNow, &lastSendPing, PING_PERIOD)) {
    JsonDocument doc;
    JsonObject jsonBufferRoot = doc.to<JsonObject>();
    // Let's flag the first ping after boot
    if (sendInitPing) {
      jsonBufferRoot["init"] = true;
      jsonBufferRoot["init_reason"] = getResetReasonStr();
      sendInitPing = false;
    }
    jsonBufferRoot["failed_msg"] = failedMessageCount;
    jsonBufferRoot["retried_msg"] = retriedMessageCount;
    jsonBufferRoot["lost_msg"] = lostMessageCount;
    differMessage(MESSAGE_PING, jsonBufferRoot);
  }

}

void Firebase::setCommonFields(JsonObject jsonDoc) {
  log_i("Firebase::setCommonFields\n");

  // jsonDoc["lang"] = XIOT_LANG;
  jsonDoc["name"] = settings->getString("module_name", "AquaMonitorProto");
  jsonDoc["mac"] = macAddrStr;
  jsonDoc["date"] = getDateStr();
  jsonDoc["heap_size"] = ESP.getFreeHeap();

}

void Firebase::differRecord(MessageType type, JsonObject jsonBufferRoot) {
  log_i("Firebase::differRecord\n");
  setCommonFields(jsonBufferRoot);
  size_t size = measureJson(jsonBufferRoot);
  log_i("Firebase json buffer required size %d\n", size);
  char* serialized = (char *)malloc(size + 1);
  serializeJson(jsonBufferRoot, serialized, size + 1);
  differMessage(type, serialized);
  free(serialized);
}

int Firebase::sendEvent(const char* type, const char* logMessage) {
  log_i("Firebase::sendEvent char*, char*\n");
  char url[URL_MAX_LENGTH_WITHOUT_SECRET];
  sprintf_P(url, URL_FORMAT, getFirebaseUrl(), type);

  // If the message string contains Json, send it as it is
  if (strncmp(logMessage, "{\"", 2) == 0) {
    return sendToFirebase("POST", url, logMessage);
  } else {
    JsonDocument doc;
    JsonObject jsonBufferRoot = doc.to<JsonObject>();
    jsonBufferRoot["message"] = logMessage;
    setCommonFields(jsonBufferRoot);
    return sendToFirebase("POST", url, jsonBufferRoot);
  }
}

char* Firebase::getDateStr() {
  RTC_DateTime dt = rtc.getDateTime();
  int h = dt.getHour();
  int mi = dt.getMinute();
  int s = dt.getSecond();
  int d = dt.getDay();
  int mo = dt.getMonth() - 1;
  int y = dt.getYear();

  sprintf_P(date, YEAR_FIRST_DATE_TIME_FORMAT, y, mo, d, h, mi, s);
  return date;
}

int Firebase::sendToFirebase(const char* method, const char* url, JsonObject jsonBufferRoot) {
  log_i("Firebase::sendToFirebase char* char* JsonObject\n");
  size_t size = measureJson(jsonBufferRoot);
  log_i("Firebase json buffer required size %d\n", size);
  char* serialized = (char *)malloc(size + 1);
  serializeJson(jsonBufferRoot, serialized, size + 1);
  log_i("Firebase json buffer used size %d\n", strlen(serialized));
  int result = sendToFirebase(method, url, serialized);
  free(serialized);
  return result;
}

int Firebase::sendToFirebase(const char* method, const char* url, const char* payload) {
  log_i("Firebase::sendToFirebase char* char* char* %s %s\n", method, url);
  //security. Should be useless since tested ealier to avoid useless json creation.
  if (!initialized) {
    return -1;
  }

// Debugging stuff
// Serial.println(payload);
// return -1;

  char urlWithSecret[URL_MAX_LENGTH_WITHOUT_SECRET + FIREBASE_SECRET_MAX_LENGTH];
  log_i("Max size for url with secrets: %d\n", URL_MAX_LENGTH_WITHOUT_SECRET + FIREBASE_SECRET_MAX_LENGTH);
  const char *token = settings->getString("firebase_token", "");
  // to disable token usage, set it to small dummy string
  if (strlen(token) > 10) {
    sprintf_P(urlWithSecret, URL_WITH_SECRET_FORMAT, url, token);
  } else {
    strcpy(urlWithSecret, url);
  }
  
  int httpCode = HttpsClient::send(method, urlWithSecret, payload);
  return httpCode;
}

// Trying to send a message while processing an incoming request is suspected to crash the module
// => handling a pile of messages that will be processed later, with retries.
void Firebase::differLog(JsonObject jsonDoc) {
  log_i("Firebase::differLog JsonObject");
  differMessage(MESSAGE_LOG, jsonDoc);
}

void Firebase::differMessage(MessageType type, JsonObject jsonBufferRoot) {
  log_i("Firebase::differMessage MessageType %d", type);
  setCommonFields(jsonBufferRoot);
  size_t size = measureJson(jsonBufferRoot);
  log_i("Firebase json buffer required size %d", size);
  char* serialized = (char *)malloc(size + 1);
  serializeJson(jsonBufferRoot, serialized, size + 1);
  log_i("Firebase json buffer used size %d", strlen(serialized));
  //log_i("Firebase::differMessage payload %s\", serialized);
  differMessage(type, serialized);
  free(serialized);
}

void Firebase::differLog(const char* message) {
  log_i("Firebase::differMessage const char*");
  differMessage(MESSAGE_LOG, message);
}

void Firebase::differAlert(const char* message) {
  log_i("Firebase::differAlert const char*");
  differMessage(MESSAGE_ALERT, message);
}

const char* Firebase::getFirebaseUrl() {
  return settings->getString("firebase_url", "");
}

void Firebase::differMessage(MessageType type, const char* message) {
  log_i("Firebase::differMessage MessageType, const char*");
  Message **differedMessagePile = differedMessages;
  int count = 0;
  while (count < MAX_DIFFERED_MESSAGES_COUNT) {
    if (*differedMessagePile == NULL) {
      // Do not keep ping messages if not connected of message queue has more than 2 messages
      if ((type == MESSAGE_PING) && (!initialized || (count > 2))) {
        return;
      }
      *differedMessagePile = new Message(type, message);
      log_i("Message added at position %d", count);
      break;
    }
    differedMessagePile ++;
    count ++;
  }
  if (count == MAX_DIFFERED_MESSAGES_COUNT) {
    log_i("Differed message pile full, message lost");    
    lostMessageCount ++;
  }
}

void Firebase::handleDifferedMessages() {
  // log_i("Firebase::handleDifferedMessages\n"); too frequent for that !!
  if (!initialized || differedMessages[0] == NULL) {
    return;
  }
  unsigned long timeNow = millis();
  if(!XUtils::isElapsedDelay(timeNow, &lastHandledDifferedMessage, currentDifferedMessageDelay)) {
    return;
  }
  int httpCode = -1;
  differedMessages[0]->retryCount ++;
  differedMessages[0]->initialTime = timeNow;
  log_i("Sending message, try number %d", differedMessages[0]->retryCount);
  char url[URL_MAX_LENGTH_WITHOUT_SECRET];
  switch(differedMessages[0]->type) {
    case MESSAGE_LOG:
      httpCode = sendEvent("log", differedMessages[0]->message);
      break;

    case MESSAGE_ALERT:
      httpCode = sendEvent("alert", differedMessages[0]->message);
      break;

    case MESSAGE_PING:
      sprintf_P(url, PING_URL_FORMAT, getFirebaseUrl());    
      httpCode = sendToFirebase("POST", url, differedMessages[0]->message);
      break;

    case MESSAGE_MODULE:
      sprintf_P(url, MODULE_URL_FORMAT, getFirebaseUrl(), macAddrStr);    
      httpCode = sendToFirebase("PUT", url, differedMessages[0]->message);
      break;

    case MESSAGE_CUSTOM:
    case RECORD_CUSTOM:
    default:
      Serial.println("Message type not supported yet\n");
  }
  boolean deleteMessage = false;
  // in case of failure, do not keep ping message in the queue: all other messages are more important.
  if ((httpCode != 200) && (differedMessages[0]->type != MESSAGE_PING)) {
    if (differedMessages[0]->retryCount >= MAX_RETRY_MESSAGE_COUNT) {
      log_i("Differed message not sent, but already retried %d times: deleting", MAX_RETRY_MESSAGE_COUNT);
      deleteMessage = true;
      failedMessageCount ++;
    } else {
      log_i("Differed message not sent, will retry");
      retriedMessageCount ++;
      // Increase the retry delay if not already max
      if (currentDifferedMessageDelay < MAX_RETRY_DELAY) {
        currentDifferedMessageDelay += 250;
      }
    }
  } else {
    deleteMessage = true;
  }

  if (deleteMessage) {
    // Reset the retry delay
    currentDifferedMessageDelay = HANDLE_DIFFERED_MESSAGES_DEFAULT_DELAY_MS;
    // Shift the message fifo queue
    delete(differedMessages[0]);
    differedMessages[0] = NULL;
    for (int i=0; i < MAX_DIFFERED_MESSAGES_COUNT - 1; i++) {
      differedMessages[i] = differedMessages[i+1];
    }
    differedMessages[MAX_DIFFERED_MESSAGES_COUNT - 1] = NULL;
    // No more messages to send ?
    if (differedMessages[0] == NULL) {
      log_i("No more differed messages");
      failedMessageCount = 0;
      retriedMessageCount = 0;
      lostMessageCount = 0;
    }
  }
   
}