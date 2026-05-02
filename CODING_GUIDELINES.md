# Project Coding Guidelines for IOTSharedEPaperDisplay

## Logging Standards

**DO NOT use Serial.print() for debugging messages.**

Instead, use the following logging functions:
- `log_i()` - Info level messages
- `log_w()` - Warning level messages
- `log_d()` - Debug level messages
- `log_e()` - Error level messages

### Examples:
```cpp
// ❌ WRONG
Serial.print("Connected to WiFi");

// ✅ CORRECT
log_i("Connected to WiFi");

// ❌ WRONG
Serial.println("Warning: Low signal strength");

// ✅ CORRECT
log_w("Warning: Low signal strength");

// ❌ WRONG
Serial.printf("Debug: sensor value = %d", value);

// ✅ CORRECT
log_d("Debug: sensor value = %d", value);
```

This applies to all .cpp and .h files in this project. Using the log_* functions ensures consistent logging output and better integration with the monitoring system.
