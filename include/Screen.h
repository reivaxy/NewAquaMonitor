#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <epd_driver.h>
#include <firasans.h>
#include <SensorPCF8563.hpp>
#include "fonts/arial12.h"
#include "fonts/arial24.h"
#include "Settings.h"
#include "WifiManager.h"
#include "ServerManager.h"

class Screen {
public:
    enum class TextAlignMode {
        Left,
        Center,
        Right
    };
    Screen();
    void setWifiManager(WifiManager* wifiManager);
    void setRtc(SensorPCF8563* rtc);
    void setSettings(Settings* settings);
    void setServer(ServerManager* server);
    void init();
    void writeTitre(const String &text, int y);
    void writeText(const String &text, TextAlignMode mode, int x, int y);
    void writeText(GFXfont *font, const String &text, TextAlignMode mode, int x, int y);
    void drawButton(int x, int y, int width, int height, const String &label);
    void refresh(bool fullRefresh = false);
    void refreshDateTime();
    void refreshNetworkInfo();
    
    private:
    WifiManager* _wifiManager;
    SensorPCF8563* _rtc;
    Settings* _settings;
    ServerManager* _server;
    uint8_t *framebuffer;
    void getCharDimensions(GFXfont *font, int32_t *charWidth, int32_t *charHeight);
};
 