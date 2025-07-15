 #ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declarations
class WiFiManager;
class SensorManager;

// ================================
// WEB SERVER MANAGER CLASS
// ================================

class WebServerManager {
public:
    // Constructor
    WebServerManager();
    
    // Initialization
    void begin();
    void end();
    
    // Main loop handler
    void handleClient();
    
    // Server Control
    void start();
    void stop();
    bool isRunning();
    
    // WebSocket Management
    void broadcastMessage(const String& message);
    void broadcastSensorData();
    void broadcastDeviceStats();
    int getWebSocketClientCount();
    
    // Manager References (set these after creating managers)
    void setWiFiManager(WiFiManager* wifiManager);
    void setSensorManager(SensorManager* sensorManager);
    
    // Device Control Callbacks
    void onDeviceNameChange(std::function<void(const String&)> callback);
    void onLEDControl(std::function<void(bool)> callback);
    void onFactoryReset(std::function<void()> callback);
    void onRestart(std::function<void()> callback);
  

