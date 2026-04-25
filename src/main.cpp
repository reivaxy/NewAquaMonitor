#include "main.h"
#include <Wire.h>
#include <SensorPCF8563.hpp>
#include <esp_system.h>


Settings settings;
SensorPCF8563 rtc;
WifiManager wifiManager(&settings, &rtc);
ServerManager server(&settings, 80);
Screen screen;


uint32_t lastTimeUpdate = 0;
uint32_t lastScreenRefresh = 0;

time_t now = millis();
int counter = 0;

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
}

void loop() {
  // Feed the watchdog regularly to prevent resets
  esp_task_wdt_reset();
  
  wifiManager.run();
  server.handleClient();
  
  if (millis() - now > 4000) {
    counter++;
    now = millis();
    log_i("Counter: %d", counter);
    //screen.writeText(String(counter), Screen::TextAlignMode::Center, 0, 400);
  }
  
  // Refresh screen every 10 minutes
  if (millis() - lastScreenRefresh > 600000) {
    lastScreenRefresh = millis();
    screen.refresh(true);
  }
  
  // Update time display every minute
  if (millis() - lastTimeUpdate > 60000) {
    lastTimeUpdate = millis();
    screen.refreshDateTime();
  }
}

