
#include "Screen.h"

// Screen resolution is 540 x 960
// 16 levels of Gray.

Screen::Screen() : _wifiManager(nullptr), _rtc(nullptr), _settings(nullptr), _server(nullptr) {
}

void Screen::setWifiManager(WifiManager* wifiManager) { _wifiManager = wifiManager; }
void Screen::setRtc(SensorPCF8563* rtc) { _rtc = rtc; }
void Screen::setSettings(Settings* settings) { _settings = settings; }
void Screen::setServer(ServerManager* server) { _server = server; }

void Screen::init() {
  log_i("Init Screen");
  // Initialize the framebuffer and EPD47 display
  framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
  if (!framebuffer) {
    log_e("alloc memory failed !!!");
    while (1);
  }
  memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
  epd_init();
  epd_poweron();
  refresh(true);
}

void Screen::writeTitre(const String &text, int y) {
  writeText((GFXfont *)&arial24, text, TextAlignMode::Center, 0, y);
}

void Screen::writeText(const String &text, TextAlignMode mode, int x, int y) {
  writeText((GFXfont *)&arial12, text, mode, x, y);
}

void Screen::writeText(GFXfont *font, const String &text, TextAlignMode mode, int x, int y) {
  int x0 = x;
  int y0 = y;
  int x1, y1, w, h, newX;
  
  epd_poweron();
  //epd_clear_area(area);
  int charHeight, charWidth;
  getCharDimensions(font, &charWidth, &charHeight);
  get_text_bounds(font, text.c_str(), &x0, &y0, &x1, &y1, &w, &h, NULL);
  log_d("bounds: x %d y %d x1 %d y1 %d h %d w %d", x0, y0, x1, y1, h, w);
  
  y += charHeight;

  // Adjust the width to prevent the last character from being cut off
  w += 9;
  // "mode" is relative to the "x" value, not to 0
  switch (mode) {
    case TextAlignMode::Left:
      newX = x;
      break;
    case TextAlignMode::Center:
      newX = (EPD_WIDTH - w + x) / 2;
      break;
    case TextAlignMode::Right:
      newX = EPD_WIDTH - w;
      break;
  }

  charHeight += 4;
  int areaX = max(0, newX - charWidth / 2) - 2;
  int areaWidth = w + charWidth;
  // Ensure the area does not exceed the display boundaries
  if (areaX + areaWidth > EPD_WIDTH) {
    areaWidth = EPD_WIDTH - areaX;
  } 
  
  Rect_t area = {
    .x = areaX,
    .y = y - charHeight + 4,
    .width = areaWidth,
    .height = charHeight + 4,
  };
  
  epd_clear_area(area);
  write_mode(font, text.c_str(), &newX, &y, NULL, BLACK_ON_WHITE, NULL);
  epd_poweroff_all();
}


void Screen::getCharDimensions(GFXfont *font, int32_t *charWidth, int32_t *charHeight) {
  String Pp = String("Gg");
  int x0 = 0;
  int y0 = 0;
  int x1, y1;

  get_text_bounds(font, Pp.c_str(), &x0, &y0, &x1, &y1, charWidth, charHeight, NULL);
  // half the width of "Pp" is a good approximation of the average character width, and the height is the same for all characters
  *charWidth = *charWidth / 2;
  log_d("Width: %d, Height: %d", *charWidth, *charHeight);

}

void Screen::drawButton(int x, int y, int width, int height, const String &label) {
    // Code to draw a button with the specified dimensions and label
}

void Screen::refresh(bool fullRefresh) {
  if (fullRefresh) {
    log_i("Performing full screen refresh");
    epd_poweron();
    epd_clear_area(epd_full_screen());
    epd_poweroff();
  } 
  writeTitre("Aqua Monitor", 10);   
  refreshDateTime();
  refreshAPInfo();
  refreshSTAInfo();
}

void Screen::refreshAPInfo() {
  if (_wifiManager == nullptr) return;
  String apSSID = _wifiManager->getAPSSID();
  String apIP = _wifiManager->getAPIP();
  writeText(apSSID, Screen::TextAlignMode::Left, 10, 465);
  writeText(apIP, Screen::TextAlignMode::Left, 10, 500);
}

void Screen::refreshSTAInfo() {
  if (_wifiManager == nullptr || !_wifiManager->isConnected()) return;
  String homeSSID = _wifiManager->getStationSSID();
  String homeIP = _wifiManager->getStationIP();
  writeText(homeSSID, Screen::TextAlignMode::Right, 790, 465);
  writeText(homeIP, Screen::TextAlignMode::Right, 790, 500);
}

void Screen::refreshDateTime() {
  if (_rtc == nullptr) return;
  struct tm timeinfo;
  _rtc->getDateTime(&timeinfo);
  if (timeinfo.tm_year > 120) {  // Year must be >= 2020 (1900 + 120)
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", &timeinfo);
    writeText(String(timeStr), TextAlignMode::Right, 790, 10);
  } else {
    log_w("RTC time not yet synchronized, skipping display update");
  }
}