 /*
 * ESP32 Smart Captive Portal
 * Professional IoT Device with WiFi Management and Real-time Dashboard
 * 
 * Author: ESP32-IoT Project
 * Version: 2.0.0
 * License: MIT
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// Project Headers
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "sensor_manager.h"

// ================================
// GLOBAL VARIABLES
// ================================

// System State
bool systemInitialized = false;
unsigned long bootTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastHeapCheck = 0;

// Device Configuration
String deviceName = DEFAULT_DEVICE_NAME;
Preferences preferences;

// Managers
WiFiManager wifiManager;
WebServerManager webServer;
SensorManager sensorManager;

// Hardware State
bool ledState = false;
bool buttonPressed = false;
unsigned long buttonPressTime = 0;

// Statistics
uint32_t bootCount = 0;
uint32_t totalConnections = 0;

// ================================
// FUNCTION DECLARATIONS
// ================================

void initializeSystem();
void loadConfiguration();
void saveConfiguration();
void handleButton();
void handleHeartbeat();
void checkSystemHealth();
void performFactoryReset();
void restartDevice();
String getSystemInfo();

// ================================
// SETUP FUNCTION
// ================================

void setup() {
    // Initialize Serial Communication
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_I("=================================");
    DEBUG_I("ESP32 Smart Captive Portal v%s", DEVICE_VERSION);
    DEBUG_I("Build: %s", FIRMWARE_BUILD_DATE);
    DEBUG_I("=================================");
    
    // Record boot time
    bootTime = millis();
    
    // Initialize system components
    initializeSystem();
    
    DEBUG_I("System initialization complete");
    DEBUG_I("Free heap: %d bytes", ESP.getFreeHeap());
    DEBUG_I("Device ready!");
}

// ================================
// MAIN LOOP
// ================================

void loop() {
    // Handle WiFi management
    wifiManager.handleClient();
    
    // Handle web server
    webServer.handleClient();
    
    // Update sensor data
    sensorManager.update();
    
    // Handle hardware inputs
    handleButton();
    
    // System maintenance
    handleHeartbeat();
    checkSystemHealth();
    
    // Small delay to prevent watchdog issues
    delay(LOOP_DELAY_MS);
}

// ================================
// SYSTEM INITIALIZATION
// ================================

void initializeSystem() {
    DEBUG_I("Initializing system components...");
    
    // Initialize hardware
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? LOW : HIGH); // LED off initially
    
    #if FEATURE_BUTTON_CONTROL
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    #endif
    
    // Initialize preferences
    preferences.begin(PREFS_NAMESPACE, false);
    
    // Load configuration
    loadConfiguration();
    
    // Initialize managers
    DEBUG_I("Initializing WiFi Manager...");
    wifiManager.begin(deviceName);
    
    DEBUG_I("Initializing Web Server...");
    webServer.begin();
    
    DEBUG_I("Initializing Sensor Manager...");
    sensorManager.begin();
    
    // Setup mDNS
    #if FEATURE_MDNS
    String mdnsName = deviceName;
    mdnsName.toLowerCase();
    mdnsName.replace(" ", "-");
    
    if (MDNS.begin(mdnsName.c_str())) {
        MDNS.addService(MDNS_SERVICE_NAME, MDNS_PROTOCOL, MDNS_SERVICE_PORT);
        MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, MDNS_TXT_VERSION, DEVICE_VERSION);
        MDNS.addServiceTxt(MDNS_SERVICE_NAME, MDNS_PROTOCOL, MDNS_TXT_DEVICE, deviceName);
        DEBUG_I("mDNS started: %s.local", mdnsName.c_str());
    } else {
        DEBUG_E("mDNS initialization failed");
    }
    #endif
    
    // Update boot statistics
    bootCount++;
    preferences.putUInt(PREF_BOOT_COUNT, bootCount);
    
    systemInitialized = true;
    DEBUG_I("System initialization completed successfully");
}

// ================================
// CONFIGURATION MANAGEMENT
// ================================

void loadConfiguration() {
    DEBUG_I("Loading configuration from preferences...");
    
    // Load device name
    deviceName = preferences.getString(PREF_DEVICE_NAME, DEFAULT_DEVICE_NAME);
    
    // Validate device name
    if (deviceName.length() < DEVICE_NAME_MIN_LENGTH || 
        deviceName.length() > DEVICE_NAME_MAX_LENGTH) {
        DEBUG_W("Invalid device name length, using default");
        deviceName = DEFAULT_DEVICE_NAME;
    }
    
    // Load statistics
    bootCount = preferences.getUInt(PREF_BOOT_COUNT, 0);
    totalConnections = preferences.getUInt(PREF_TOTAL_CONNECTIONS, 0);
    
    DEBUG_I("Device name: %s", deviceName.c_str());
    DEBUG_I("Boot count: %d", bootCount);
    DEBUG_I("Total connections: %d", totalConnections);
}

void saveConfiguration() {
    DEBUG_I("Saving configuration to preferences...");
    
    preferences.putString(PREF_DEVICE_NAME, deviceName);
    preferences.putUInt(PREF_BOOT_COUNT, bootCount);
    preferences.putUInt(PREF_TOTAL_CONNECTIONS, totalConnections);
    
    DEBUG_I("Configuration saved successfully");
}

// ================================
// HARDWARE HANDLING
// ================================

void handleButton() {
    #if FEATURE_BUTTON_CONTROL
    static bool lastButtonState = HIGH;
    static unsigned long buttonDebounceTime = 0;
    
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    // Debounce button
    if (currentButtonState != lastButtonState) {
        buttonDebounceTime = millis();
    }
    
    if ((millis() - buttonDebounceTime) > BUTTON_DEBOUNCE_MS) {
        if (currentButtonState == LOW && lastButtonState == HIGH) {
            // Button pressed
            buttonPressed = true;
            buttonPressTime = millis();
            DEBUG_D("Button pressed");
        }
        
        if (currentButtonState == HIGH && lastButtonState == LOW) {
            // Button released
            if (buttonPressed) {
                unsigned long pressDuration = millis() - buttonPressTime;
                
                if (pressDuration >= BUTTON_VERY_LONG_PRESS_MS) {
                    // Very long press - Factory reset
                    DEBUG_I("Very long button press detected - Factory reset");
                    performFactoryReset();
                } else if (pressDuration >= BUTTON_LONG_PRESS_MS) {
                    // Long press - WiFi reset
                    DEBUG_I("Long button press detected - WiFi reset");
                    wifiManager.resetWiFiSettings();
                    restartDevice();
                } else {
                    // Short press - Toggle LED
                    DEBUG_D("Short button press - Toggle LED");
                    ledState = !ledState;
                    digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? ledState : !ledState);
                }
                
                buttonPressed = false;
            }
        }
    }
    
    lastButtonState = currentButtonState;
    #endif
}

void handleHeartbeat() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastHeartbeat >= LED_HEARTBEAT_INTERVAL) {
        // Quick heartbeat blink
        digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? HIGH : LOW);
        delay(LED_HEARTBEAT_DURATION);
        digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? ledState : !ledState);
        
        lastHeartbeat = currentTime;
        
        // Update mDNS
        #if FEATURE_MDNS
        MDNS.update();
        #endif
    }
}

// ================================
// SYSTEM HEALTH MONITORING
// ================================

void checkSystemHealth() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastHeapCheck >= HEAP_CHECK_INTERVAL) {
        size_t freeHeap = ESP.getFreeHeap();
        
        if (freeHeap < MIN_FREE_HEAP) {
            DEBUG_W("Low memory warning: %d bytes free", freeHeap);
            
            // Attempt garbage collection
            delay(100);
            
            // Check again
            freeHeap = ESP.getFreeHeap();
            if (freeHeap < MIN_FREE_HEAP / 2) {
                DEBUG_E("Critical memory shortage - restarting");
                restartDevice();
            }
        }
        
        lastHeapCheck = currentTime;
        DEBUG_V("System health check - Free heap: %d bytes", freeHeap);
    }
}

// ================================
// SYSTEM CONTROL FUNCTIONS
// ================================

void performFactoryReset() {
    DEBUG_I("Performing factory reset...");
    
    // Clear all preferences
    preferences.clear();
    
    // Reset WiFi settings
    wifiManager.resetWiFiSettings();
    
    // Update factory reset counter
    uint32_t resetCount = preferences.getUInt(PREF_FACTORY_RESET_COUNT, 0) + 1;
    preferences.putUInt(PREF_FACTORY_RESET_COUNT, resetCount);
    
    DEBUG_I("Factory reset completed. Reset count: %d", resetCount);
    
    // Restart device
    delay(2000);
    ESP.restart();
}

void restartDevice() {
    DEBUG_I("Restarting device...");
    
    // Save current configuration
    saveConfiguration();
    
    // Clean shutdown
    webServer.end();
    wifiManager.end();
    
    delay(1000);
    ESP.restart();
}

// ================================
// SYSTEM INFORMATION
// ================================

String getSystemInfo() {
    String info = "{\n";
    info += "  \"device_name\": \"" + deviceName + "\",\n";
    info += "  \"version\": \"" + String(DEVICE_VERSION) + "\",\n";
    info += "  \"build_date\": \"" + String(FIRMWARE_BUILD_DATE) + "\",\n";
    info += "  \"uptime\": " + String(millis() - bootTime) + ",\n";
    info += "  \"boot_count\": " + String(bootCount) + ",\n";
    info += "  \"free_heap\": " + String(ESP.getFreeHeap()) + ",\n";
    info += "  \"chip_model\": \"" + String(ESP.getChipModel()) + "\",\n";
    info += "  \"chip_revision\": " + String(ESP.getChipRevision()) + ",\n";
    info += "  \"cpu_freq\": " + String(ESP.getCpuFreqMHz()) + ",\n";
    info += "  \"flash_size\": " + String(ESP.getFlashChipSize()) + ",\n";
    info += "  \"mac_address\": \"" + WiFi.macAddress() + "\"\n";
    info += "}";
    
    return info;
}

// ================================
// CALLBACK FUNCTIONS FOR MANAGERS
// ================================

// Called when device name is changed
void onDeviceNameChanged(const String& newName) {
    if (newName.length() >= DEVICE_NAME_MIN_LENGTH && 
        newName.length() <= DEVICE_NAME_MAX_LENGTH) {
        
        deviceName = newName;
        saveConfiguration();
        
        DEBUG_I("Device name changed to: %s", deviceName.c_str());
        
        // Update mDNS
        #if FEATURE_MDNS
        MDNS.end();
        String mdnsName = deviceName;
        mdnsName.toLowerCase();
        mdnsName.replace(" ", "-");
        
        if (MDNS.begin(mdnsName.c_str())) {
            MDNS.addService(MDNS_SERVICE_NAME, MDNS_PROTOCOL, MDNS_SERVICE_PORT);
            DEBUG_I("mDNS updated: %s.local", mdnsName.c_str());
        }
        #endif
    }
}

// Called when WiFi connection status changes
void onWiFiStatusChanged(bool connected) {
    if (connected) {
        totalConnections++;
        preferences.putUInt(PREF_TOTAL_CONNECTIONS, totalConnections);
        DEBUG_I("WiFi connected. Total connections: %d", totalConnections);
    } else {
        DEBUG_I("WiFi disconnected");
    }
}

// Called when LED state should be changed
void onLEDControlRequest(bool state) {
    ledState = state;
    digitalWrite(LED_PIN, LED_ACTIVE_HIGH ? ledState : !ledState);
    DEBUG_D("LED state changed to: %s", ledState ? "ON" : "OFF");
}

// ================================
// GETTER FUNCTIONS FOR MANAGERS
// ================================

String getDeviceName() {
    return deviceName;
}

uint32_t getBootCount() {
    return bootCount;
}

uint32_t getTotalConnections() {
    return totalConnections;
}

unsigned long getUptime() {
    return millis() - bootTime;
}

bool getLEDState() {
    return ledState;
}

