
#include "Settings.h"

// Define secret keys
const char* Settings::SECRET_KEYS[] = {
  "home_password",
  "ap_password"
};
const size_t Settings::SECRET_KEYS_COUNT = 2;

Settings::Settings() {
} 

void Settings::loadSettings() {
  preferences.begin("settings", false);
  String json = preferences.getString("settingsJson", "{}");
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    log_i("Failed to parse settings JSON, using defaults");
    doc.clear();
  } else {
    log_i("Settings loaded from JSON %s", getSettingJsonWithoutSecrets());
  }
  preferences.end();
}

String Settings::getSettingJson() {
  String json;
  serializeJson(doc, json);
  return json;
}

String Settings::getSettingJsonWithoutSecrets() {
  // Create a new document and copy all entries, masking secrets
  JsonDocument filteredDoc;
  
  for (JsonPair pair : doc.as<JsonObject>()) {
    const char* key = pair.key().c_str();
    bool isSecret = false;
    
    for (size_t i = 0; i < SECRET_KEYS_COUNT; i++) {
      if (strcmp(key, SECRET_KEYS[i]) == 0) {
        isSecret = true;
        break;
      }
    }
    
    if (isSecret) {
      filteredDoc[key] = "****";
    } else {
      filteredDoc[key] = pair.value();
    }
  }
  
  String json;
  serializeJson(filteredDoc, json);
  return json;
}

void Settings::updateSetting(char* message, bool save) {
  log_i("Updating setting %s", message);
  // Split on '='
  char* equalSign = strchr(message, '='); 
  if (equalSign != NULL) {
     *equalSign = 0; // terminate key string
     const char* fullKey = message;
     const char* value = equalSign + 1;

     // Check if the key has a type prefix: first char is type, second char is '_'
     bool hasTypePrefix = (fullKey[0] != '\0' && fullKey[1] == '_');
     const char* key = hasTypePrefix ? fullKey + 2 : fullKey; // Skip "x_" prefix if present
     char typeChar = hasTypePrefix ? fullKey[0] : '\0';

     log_i("Setting key: '%s', value: '%s'", key, value);
     switch(typeChar) {
        case 'i': // integer
           {
              int intValue = atoi(value);
              setInt(key, intValue);
              log_i("Stored int setting");
           }
           break;
        case 's': // string
           setString(key, value);
           log_i("Stored string setting");
           break;
        case 'b': // boolean
           {
              bool boolValue = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
              doc[key] = boolValue;
              log_i("Stored boolean setting");
           }
           break;
        default:
           log_i("No type prefix, saving as string with full key '%s'", fullKey);
           setString(fullKey, value);
     }
     if (save) saveSettings();
     
  } else {
     log_i("Invalid setting format, expected key=value");
  }
}

void Settings::saveSettings() {
   String json;
   serializeJson(doc, json);
   preferences.begin("settings", false);
   preferences.putString("settingsJson", json);
   log_i("Settings saved to JSON");
   preferences.end();
}

int Settings::getInt(const char* key, int defaultValue) {
   if (doc[key].is<int>()) {
      return doc[key];
   }
   return defaultValue;
}
void Settings::setInt(const char* key, int value) {
   doc[key] = value;
}
void Settings::setString(const char* key, const char* value) {
   doc[key] = value;
}
const char* Settings::getString(const char* key, const char* defaultValue) {
   if (doc[key].is<const char*>()) {
      return doc[key];
   }
   return defaultValue;
}

void Settings::setBool(const char* key, boolean value) {
   doc[key] = value;
}
boolean Settings::getBool(const char* key, boolean defaultValue) {
   if (doc[key].is<bool>()) {
      return doc[key];
   }
   return defaultValue;
}

// close the preferences when done
Settings::~Settings() {
  saveSettings();
  preferences.end();
}
