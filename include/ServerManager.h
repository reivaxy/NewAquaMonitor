#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "Settings.h"

class Screen;  // forward declaration

class ServerManager {
public:
  ServerManager(Settings* settings, uint16_t port = 80);
  ~ServerManager();
  
  void begin();
  void stop();
  void handleClient();
  void setScreen(Screen* screen);
  
private:
  Settings* _settings;
  Screen* _screen = nullptr;
  WebServer* _webServer;
  uint16_t _port;
  
  void handleRoot();
  void handleSettingsGet();
  void handleSettingsPost();
  void handleSettingsDelete();
  void handleRefresh();
  void handleRestart();
  void handleNotFound();
};
