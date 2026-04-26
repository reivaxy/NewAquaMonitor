/**
 *  HTTPS client helper class
 *  Xavier Grosjean 2026
 *  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License
 */

#include "HttpsClient.h"

int HttpsClient::send(const char* method, const char* url, const char* payload) {
  // Log URL without query params to avoid leaking secrets
  const char* q = strchr(url, '?');
  if (q) {
    char urlBase[q - url + 1];
    memcpy(urlBase, url, q - url);
    urlBase[q - url] = '\0';
    log_i("HttpsClient::send %s %s", method, urlBase);
  } else {
    log_i("HttpsClient::send %s %s", method, url);
  }

  WiFiClientSecure client;
  // Skip certificate verification — acceptable for internal/trusted endpoints.
  // To enforce CA validation, call client.setCACert(ca_cert) before begin().
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    log_e("HttpsClient: http.begin failed for url: %s", url);
    return -1;
  }

  http.addHeader("Content-Type", "application/json");

  int httpCode = -1;
  if (strcmp(method, "GET") == 0) {
    httpCode = http.GET();
  } else if (strcmp(method, "POST") == 0) {
    httpCode = http.POST(payload ? payload : "");
  } else if (strcmp(method, "PUT") == 0) {
    httpCode = http.PUT(payload ? payload : "");
  } else if (strcmp(method, "PATCH") == 0) {
    httpCode = http.PATCH(payload ? payload : "");
  } else if (strcmp(method, "DELETE") == 0) {
    httpCode = http.sendRequest("DELETE", payload ? payload : "");
  } else {
    log_e("HttpsClient: unsupported method: %s\n", method);
    http.end();
    return -1;
  }

  if (httpCode < 0) {
    log_e("HttpsClient: request failed, error: %s", http.errorToString(httpCode).c_str());
  } else {
    log_i("HttpsClient: response code: %d", httpCode);
  }

  http.end();
  return httpCode;
}
