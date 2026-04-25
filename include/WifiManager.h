#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <SensorPCF8563.hpp>
#include <esp_task_wdt.h>
#include "Settings.h"

class Screen;  // forward declaration to avoid circular dependency

class WifiManager {
public:
  WifiManager(Settings* settings, SensorPCF8563* rtc);
  ~WifiManager();
  
  void begin();
  void run();
  void stop();
  bool isConnected();
  String getAPIP();
  String getStationIP();
  String getAPSSID();
  String getStationSSID();
  void setScreen(Screen* screen);
  
private:
  Settings* _settings;
  SensorPCF8563* _rtc;
  Screen* _screen = nullptr;
    
  static WifiManager* _instance;
  
  void startAccessPoint();
  void connectToHome();
  void onWiFiGotIP();
  void onTimeAvailable(timeval *t);

  static void timeavailable(timeval *t);
  static void WiFiEventHandler(WiFiEvent_t event);
};
