 #ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"

// ================================
// WIFI MANAGER CLASS
// ================================

class WiFiManager {
public:
    // Constructor
    WiFiManager();
    
    // Initialization
    void begin(const String& deviceName);
    void end();
    
    // Main loop handler
    void handleClient();
    
    // WiFi Connection Management
    bool connectToWiFi(const String& ssid, const String& password);
    void disconnectWiFi();
    bool isConnected();
    void resetWiFiSettings();
    
    // Access Point Management
    void startAccessPoint();
    void stopAccessPoint();
    bool isAccessPointActive();
    
    // Network Information
    String getConnectedSSID();
    IPAddress getLocalIP();
    IPAddress getAccessPointIP();
    String getMACAddress();
    int getRSSI();
    
    // Network Scanning
    int scanNetworks();
    String getScannedNetworksJSON();
    
    // Status Information
    String getStatusJSON();
    String getNetworkInfoJSON();
    
    // Configuration
    void setDeviceName(const String& name);
    String getDeviceName();
    String getAccessPointSSID();
    
    // Callbacks
    void onConnected(std::function<void()> callback);
    void onDisconnected(std::function<void()> callback);
    void onAccessPointStarted(std::function<void()> callback);

private:
    // Private member variables
    String _deviceName;
    String _apSSID;
    String _connectedSSID;
    String _connectedPassword;
    
    // Network state
    bool _isConnected;
    bool _isAPActive;
    bool _shouldReconnect;
    
    // Timing variables
    unsigned long _lastConnectionAttempt;
    unsigned long _lastReconnectAttempt;
    unsigned long _connectionStartTime;
    int _reconnectAttempts;
    
    // DNS Server for captive portal
    DNSServer* _dnsServer;
    
    // Preferences for persistent storage
    Preferences _preferences;
    
    // Callback functions
    std::function<void()> _onConnectedCallback;
    std::function<void()> _onDisconnectedCallback;
    std::function<void()> _onAccessPointStartedCallback;
    
    // Private methods
    void _loadWiFiCredentials();
    void _saveWiFiCredentials();
    void _clearWiFiCredentials();
    void _handleWiFiEvents();
    void _attemptReconnection();
    bool _isValidSSID(const String& ssid);
    bool _isValidPassword(const String& password);
    String _sanitizeSSID(const String& ssid);
    void _updateConnectionStatus();
    void _setupCaptivePortal();
    void _stopCaptivePortal();
    String _encryptionTypeToString(wifi_auth_mode_t encryptionType);
    
    // Static event handlers
    static void _wifiEventHandler(WiFiEvent_t event);
    static WiFiManager* _instance;
};

// ================================
// WIFI EVENT TYPES
// ================================

enum class WiFiManagerEvent {
    CONNECTED,
    DISCONNECTED,
    CONNECTION_FAILED,
    ACCESS_POINT_STARTED,
    ACCESS_POINT_STOPPED,
    SCAN_COMPLETED
};

// ================================
// WIFI STATUS STRUCTURE
// ================================

struct WiFiStatus {
    bool connected;
    bool accessPointActive;
    String ssid;
    IPAddress localIP;
    IPAddress accessPointIP;
    int rssi;
    String macAddress;
    unsigned long uptime;
    int reconnectAttempts;
};

#endif // WIFI_MANAGER_H

