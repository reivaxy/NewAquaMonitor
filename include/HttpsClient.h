/**
 *  HTTPS client helper class
 *  Xavier Grosjean 2026
 *  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License
 */

#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

class HttpsClient {
public:
  /**
   * Send an HTTPS request.
   * @param method  HTTP method string: "GET", "POST", "PUT", "PATCH", "DELETE"
   * @param url     Full URL including scheme (https://)
   * @param payload Request body (may be nullptr for GET/DELETE)
   * @return        HTTP response code, or a negative value on connection error
   */
  static int send(const char* method, const char* url, const char* payload = nullptr);
};
