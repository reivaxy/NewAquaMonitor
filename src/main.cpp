#include "main.h"
#include <Wire.h>
#include <SensorPCF8563.hpp>
#include <esp_system.h>


Settings settings;
SensorPCF8563 rtc;
WifiManager wifiManager(&settings, &rtc);
ServerManager server(&settings, 80);
Screen screen;
Firebase firebase(&settings, rtc);


uint32_t lastTimeUpdate = 0;
uint32_t lastScreenRefresh = 0;

time_t now = millis();
int counter = 0;


void sendModuleInfo() {
    JsonDocument doc;
    JsonObject jsonBufferRoot = doc.to<JsonObject>();

    jsonBufferRoot["type"] = "AquaMonitor";
    jsonBufferRoot["ip"] = wifiManager.getStationIP();
    #ifdef GIT_REV
    jsonBufferRoot["version"] = GIT_REV;
    #endif
    firebase.setCommonFields(jsonBufferRoot);
   
    firebase.differRecord(MESSAGE_MODULE, jsonBufferRoot);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);  // Wait for Serial to be ready, max 3 seconds
  settings.loadSettings();
  // Initialize RTC
  Wire.begin(BOARD_SDA, BOARD_SCL);
  rtc.begin(Wire);   
  screen.setWifiManager(&wifiManager);
  screen.setRtc(&rtc);
  screen.setSettings(&settings);
  screen.setServer(&server);
  screen.init();
  wifiManager.setScreen(&screen);
  wifiManager.begin();
  server.setScreen(&screen);
  server.begin();
  firebase.init(WiFi.macAddress().c_str());
  sendModuleInfo(); 
}


void loop() {
  // Feed the watchdog regularly to prevent resets
  esp_task_wdt_reset();
  
  wifiManager.run();
  server.handleClient();
  firebase.loop();
  
  if (millis() - now > 4000) {
    counter++;
    now = millis();
    log_i("Counter: %d", counter);
    //screen.writeText(String(counter), Screen::TextAlignMode::Center, 0, 400);
  }
  
  // Refresh screen every hour
  if (millis() - lastScreenRefresh > 3600000) {
    lastScreenRefresh = millis();
    screen.refresh(true);
  }
  
  // Update time display every minute
  if (millis() - lastTimeUpdate > 60000) {
    lastTimeUpdate = millis();
    screen.refreshDateTime();
  }
}

