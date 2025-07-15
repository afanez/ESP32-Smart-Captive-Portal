 #ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ================================
// DEVICE CONFIGURATION
// ================================

// Default Device Settings
#define DEFAULT_DEVICE_NAME         "ESP32-Smart-Device"
#define DEVICE_VERSION             "2.0.0"
#define DEVICE_MANUFACTURER        "ESP32-IoT"
#define FIRMWARE_BUILD_DATE        __DATE__ " " __TIME__

// Device Name Constraints
#define DEVICE_NAME_MIN_LENGTH     3
#define DEVICE_NAME_MAX_LENGTH     32
#define DEVICE_NAME_ALLOWED_CHARS  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"

// ================================
// HARDWARE CONFIGURATION
// ================================

// Pin Definitions
#define LED_PIN                    2      // Built-in LED
#define BUTTON_PIN                 0      // Boot button (GPIO0)
#define SENSOR_POWER_PIN          -1      // Optional sensor power control

// Button Configuration
#define BUTTON_DEBOUNCE_MS        50
#define BUTTON_LONG_PRESS_MS      5000    // 5 seconds for WiFi reset
#define BUTTON_VERY_LONG_PRESS_MS 10000   // 10 seconds for factory reset

// LED Configuration
#define LED_ACTIVE_HIGH           true
#define LED_HEARTBEAT_INTERVAL    2000    // Heartbeat blink interval (ms)
#define LED_HEARTBEAT_DURATION    100     // Heartbeat blink duration (ms)

// ================================
// WIFI CONFIGURATION
// ================================

// Access Point Settings
#define AP_SSID_PREFIX            "ESP32-"
#define AP_PASSWORD               "12345678"
#define AP_CHANNEL                1
#define AP_MAX_CONNECTIONS        4
#define AP_HIDDEN                 false

// Network Settings
#define AP_IP_ADDRESS             IPAddress(192, 168, 4, 1)
#define AP_GATEWAY                IPAddress(192, 168, 4, 1)
#define AP_SUBNET                 IPAddress(255, 255, 255, 0)

// WiFi Connection Settings
#define WIFI_CONNECT_TIMEOUT_MS   20000   // 20 seconds
#define WIFI_RECONNECT_INTERVAL   30000   // 30 seconds
#define WIFI_MAX_RECONNECT_ATTEMPTS 5

// Captive Portal Settings
#define CAPTIVE_PORTAL_TIMEOUT    300000  // 5 minutes before auto-restart
#define DNS_PORT                  53

// ================================
// WEB SERVER CONFIGURATION
// ================================

// Server Settings
#define WEB_SERVER_PORT           80
#define WEBSOCKET_PATH            "/ws"
#define MAX_WEBSOCKET_CLIENTS     8

// API Endpoints
#define API_PREFIX                "/api"
#define API_SCAN                  "/scan"
#define API_CONNECT               "/connect"
#define API_STATUS                "/status"
#define API_SENSOR_DATA           "/sensor-data"
#define API_DEVICE_STATS          "/stats"
#define API_DEVICE_NAME           "/device-name"
#define API_FACTORY_RESET         "/factory-reset"
#define API_RESTART               "/restart"
#define API_LED_CONTROL           "/led"

// CORS Settings
#define CORS_MAX_AGE              86400   // 24 hours

// ================================
// SENSOR CONFIGURATION
// ================================

// Sensor Update Intervals
#define SENSOR_UPDATE_INTERVAL    2000    // 2 seconds
#define SENSOR_HISTORY_SIZE       50      // Number of readings to store
#define STATS_UPDATE_INTERVAL     10000   // 10 seconds

// Sensor Simulation Settings
#define TEMP_BASE                 22.0    // Base temperature (°C)
#define TEMP_VARIATION            5.0     // Temperature variation range
#define HUMIDITY_BASE             45.0    // Base humidity (%)
#define HUMIDITY_VARIATION        20.0    // Humidity variation range
#define PRESSURE_BASE             1013.25 // Base pressure (hPa)
#define PRESSURE_VARIATION        30.0    // Pressure variation range

// Battery Simulation
#define BATTERY_DRAIN_RATE        0.01    // Battery drain per update (%)
#define BATTERY_RECHARGE_THRESHOLD 10.0   // Auto-recharge below this level
#define BATTERY_RECHARGE_RATE     5.0     // Recharge rate (%)

// Motion Sensor Simulation
#define MOTION_DETECTION_CHANCE   15      // 15% chance of motion detection
#define MOTION_DURATION_MS        5000    // Motion stays active for 5 seconds

// ================================
// DATA STORAGE CONFIGURATION
// ================================

// Preferences Namespaces
#define PREFS_NAMESPACE           "esp32_config"
#define PREFS_WIFI_NAMESPACE      "wifi_config"
#define PREFS_DEVICE_NAMESPACE    "device_config"

// Preferences Keys
#define PREF_DEVICE_NAME          "device_name"
#define PREF_WIFI_SSID            "wifi_ssid"
#define PREF_WIFI_PASSWORD        "wifi_password"
#define PREF_TOTAL_CONNECTIONS    "total_conn"
#define PREF_BOOT_COUNT           "boot_count"
#define PREF_FACTORY_RESET_COUNT  "factory_count"

// Data Retention
#define PREFS_AUTO_COMMIT         true
#define SENSOR_DATA_RETENTION_MS  3600000 // 1 hour

// ================================
// MDNS CONFIGURATION
// ================================

#define MDNS_SERVICE_NAME         "_http"
#define MDNS_PROTOCOL             "_tcp"
#define MDNS_SERVICE_PORT         80
#define MDNS_TXT_VERSION          "version"
#define MDNS_TXT_DEVICE           "device"

// ================================
// SYSTEM CONFIGURATION
// ================================

// Watchdog Settings
#define WATCHDOG_TIMEOUT_MS       30000   // 30 seconds
#define TASK_STACK_SIZE           4096
#define LOOP_DELAY_MS             10

// Memory Management
#define MIN_FREE_HEAP             10000   // Minimum free heap (bytes)
#define HEAP_CHECK_INTERVAL       30000   // Check heap every 30 seconds

// System Limits
#define MAX_JSON_BUFFER_SIZE      4096
#define MAX_HTTP_RESPONSE_SIZE    8192
#define MAX_WEBSOCKET_MESSAGE     1024

// ================================
// DEBUG CONFIGURATION
// ================================

// Debug Levels
#define DEBUG_NONE                0
#define DEBUG_ERROR               1
#define DEBUG_WARN                2
#define DEBUG_INFO                3
#define DEBUG_DEBUG               4
#define DEBUG_VERBOSE             5

// Current Debug Level (change as needed)
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL               DEBUG_INFO
#endif

// Debug Macros
#if DEBUG_LEVEL >= DEBUG_ERROR
#define DEBUG_E(fmt, ...) Serial.printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_E(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_WARN
#define DEBUG_W(fmt, ...) Serial.printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_W(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_INFO
#define DEBUG_I(fmt, ...) Serial.printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_I(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_DEBUG
#define DEBUG_D(fmt, ...) Serial.printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_D(fmt, ...)
#endif

#if DEBUG_LEVEL >= DEBUG_VERBOSE
#define DEBUG_V(fmt, ...) Serial.printf("[VERBOSE] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_V(fmt, ...)
#endif

// ================================
// FEATURE FLAGS
// ================================

// Enable/Disable Features
#define FEATURE_WEBSOCKET         true
#define FEATURE_MDNS              true
#define FEATURE_OTA               false   // Disabled by default
#define FEATURE_SENSOR_HISTORY    true
#define FEATURE_DEVICE_STATS      true
#define FEATURE_FACTORY_RESET     true
#define FEATURE_LED_CONTROL       true
#define FEATURE_BUTTON_CONTROL    true

// Sensor Features
#define SENSOR_TEMPERATURE        true
#define SENSOR_HUMIDITY           true
#define SENSOR_PRESSURE           true
#define SENSOR_LIGHT              true
#define SENSOR_MOTION             true
#define SENSOR_BATTERY            true

// ================================
// VALIDATION MACROS
// ================================

// Compile-time checks
#if DEVICE_NAME_MAX_LENGTH > 32
#error "Device name maximum length cannot exceed 32 characters"
#endif

#if AP_MAX_CONNECTIONS > 8
#error "ESP32 supports maximum 8 AP connections"
#endif

#if SENSOR_HISTORY_SIZE > 100
#warning "Large sensor history size may cause memory issues"
#endif

#endif // CONFIG_H

