#include "WifiManager.h"
#include "Screen.h"
#include <esp_sntp.h>
#include <esp_task_wdt.h>

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const char *time_zone = "CEST-2";

WifiManager* WifiManager::_instance = nullptr;

WifiManager::WifiManager(Settings* settings, SensorPCF8563* rtc) : _settings(settings), _rtc(rtc) {
  _instance = this;
}

void WifiManager::setScreen(Screen* screen) {
  _screen = screen;
}

WifiManager::~WifiManager() {
  stop();
}

void WifiManager::begin() {
  // Register WiFi event listener
  WiFi.onEvent(WiFiEventHandler);

  // Configure NTP
  configTzTime(time_zone, ntpServer1, ntpServer2);
  sntp_set_time_sync_notification_cb(timeavailable);

  // Start access point
  startAccessPoint();
  
  // Try to connect to home SSID if configured
  connectToHome();

}

void WifiManager::run() {
  // Handle any pending WiFi operations
  // This allows WiFi to process events in the background
}

void WifiManager::stop() {
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

}

bool WifiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getAPIP() {
  return WiFi.softAPIP().toString();
}

String WifiManager::getStationIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "Not connected";
}

String WifiManager::getAPSSID() {
  return WiFi.softAPSSID();
}

String WifiManager::getStationSSID() {
  return WiFi.SSID();
}

void WifiManager::startAccessPoint() {
  const char* apSSID = _settings->getString("ap_ssid", "AquaMonitor");
  const char* apPassword = _settings->getString("ap_password", "password");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword);
  
  log_i("Access Point started");
  log_i("AP SSID: %s", apSSID);
  log_i("AP IP address: %s", WiFi.softAPIP().toString().c_str());
  _screen->refreshAPInfo();
}

void WifiManager::connectToHome() {
  const char* homeSSID = _settings->getString("home_ssid", "");
  const char* homePassword = _settings->getString("home_password", "");
  
  // Only attempt connection if home SSID is configured
  if (strlen(homePassword) > 0 && strlen(homeSSID) > 0) {
    log_i("Connecting to home network...");
    log_i("Home SSID: %s", homeSSID);
    
    WiFi.begin(homeSSID, homePassword);
    
    // Wait for connection with watchdog feeding to prevent timeout
    int timeout = 0;
    int maxAttempts = 40;  // ~10 seconds at 250ms intervals
    while (WiFi.status() != WL_CONNECTED && timeout < maxAttempts) {
      esp_task_wdt_reset();  // Feed watchdog during connection attempt
      delay(250);
      timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      log_i("Successfully connected to home network");
    } else {
      log_w("Failed to connect to home network, continuing with AP mode");
    }
  } else {
    log_i("No home network configured");
  }
}

void WifiManager::WiFiEventHandler(WiFiEvent_t event) {
  log_i("WiFi event: Got event %d", event);
  if (_instance != nullptr && event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    _instance->onWiFiGotIP();
  }
}


void WifiManager::onWiFiGotIP() {
  log_i("Connected to home network");
  log_i("IP address: %s", WiFi.localIP().toString().c_str());
  if (_screen != nullptr) {
    _screen->refreshSTAInfo();
  }
}

void WifiManager::timeavailable(struct timeval *t)
{
  if (_instance != nullptr) {
    _instance->onTimeAvailable(t);
  }
}

void WifiManager::onTimeAvailable(struct timeval *t)
{
  log_i("Got time adjustment from NTP!");
  time_t now = t->tv_sec;
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  if (_rtc != nullptr) {
    _rtc->setDateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    log_i("RTC synchronized with NTP time: %s", ctime(&now));
  }
  if (_screen != nullptr) {
    _screen->refreshDateTime();
  }
}

