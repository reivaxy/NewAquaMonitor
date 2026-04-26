#include "ServerManager.h"
#include "Screen.h"

ServerManager::ServerManager(Settings* settings, uint16_t port) 
  : _settings(settings), _port(port), _webServer(nullptr) {
  _webServer = new WebServer(port);
}

ServerManager::~ServerManager() {
  stop();
  if (_webServer) {
    delete _webServer;
    _webServer = nullptr;
  }
}

void ServerManager::begin() {
  if (!_webServer) return;
  
  _webServer->on("/", HTTP_GET, [this]() { handleRoot(); });
  _webServer->on("/apis/settings", HTTP_GET, [this]() { handleSettingsGet(); });
  _webServer->on("/apis/settings", HTTP_POST, [this]() { handleSettingsPost(); });
  _webServer->on("/apis/settings", HTTP_DELETE, [this]() { handleSettingsDelete(); });
  _webServer->on("/apis/refresh", HTTP_POST, [this]() { handleRefresh(); });
  _webServer->on("/apis/restart", HTTP_POST, [this]() { handleRestart(); });
  _webServer->onNotFound([this]() { handleNotFound(); });
  
  _webServer->begin();
  log_i("Web Server started on port %d", _port);
}

void ServerManager::stop() {
  if (_webServer) {
    _webServer->stop();
  }
}

void ServerManager::handleClient() {
  if (_webServer) {
    _webServer->handleClient();
  }
}

void ServerManager::handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Aqua Monitor</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }
    .container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
    h1 { color: #333; }
    .api-link { display: inline-block; margin: 10px 0; padding: 10px 15px; background-color: #007bff; color: white; text-decoration: none; border-radius: 4px; }
    .api-link:hover { background-color: #0056b3; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Aqua Monitor</h1>
    <p>Welcome to the Aqua Monitor web interface.</p>
    <h2>Available APIs</h2>
    <ul>
      <li><a href="/apis/settings" class="api-link">GET /apis/settings</a> - Get all settings as JSON</li>
      <li><code>POST /apis/settings</code> - Update settings (e.g., sWifi_SSID=MyNetwork&iUpdate_Interval=60)</li>
    </ul>
  </div>
</body>
</html>
  )";
  
  _webServer->send(200, "text/html", html);
}

void ServerManager::handleSettingsGet() {
  String json = _settings->getSettingJsonWithoutSecrets();
  _webServer->send(200, "application/json", json);
}

void ServerManager::handleSettingsPost() {
  // Check if we have arguments
  if (_webServer->args() == 0) {
    _webServer->send(400, "application/json", "{\"error\":\"No parameters provided\"}");
    return;
  }
  
  // Process each argument
  for (uint8_t i = 0; i < _webServer->args(); i++) {
    String argName = _webServer->argName(i);
    String argValue = _webServer->arg(i);
    
    if (argName.length() < 2) continue;
    
    _settings->updateSetting((char*)(argName + "=" + argValue).c_str(), false);
  }
  _settings->saveSettings();
  
  String json = _settings->getSettingJsonWithoutSecrets();
  _webServer->send(200, "application/json", json);
}

void ServerManager::handleSettingsDelete() {
  String name = _webServer->arg("name");
  if (name.length() == 0) {
    _webServer->send(400, "application/json", "{\"error\":\"Missing required parameter: name\"}");
    return;
  }

  if (_settings->deleteSetting(name.c_str())) {
    String json = _settings->getSettingJsonWithoutSecrets();
    _webServer->send(200, "application/json", json);
  } else {
    _webServer->send(404, "application/json", "{\"error\":\"Setting not found\"}");
  }
}

void ServerManager::setScreen(Screen* screen) {
  _screen = screen;
}

void ServerManager::handleRefresh() {
  if (_screen != nullptr) {
    _screen->refresh(true);
    _webServer->send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    _webServer->send(503, "application/json", "{\"error\":\"Screen not available\"}");
  }
}

void ServerManager::handleRestart() {
  _webServer->send(200, "application/json", "{\"status\":\"restarting\"}");
  delay(200);  // Give the response time to be sent
  ESP.restart();
}

void ServerManager::handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += _webServer->uri();
  message += "\nMethod: ";
  message += (_webServer->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += _webServer->args();
  message += "\n";
  
  for (uint8_t i = 0; i < _webServer->args(); i++) {
    message += " " + _webServer->argName(i) + ": " + _webServer->arg(i) + "\n";
  }
  
  _webServer->send(404, "text/plain", message);
}
