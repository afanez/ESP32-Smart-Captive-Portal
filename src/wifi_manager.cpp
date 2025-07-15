 #include "wifi_manager.h"

// Static instance pointer for event handling
WiFiManager* WiFiManager::_instance = nullptr;

// ================================
// CONSTRUCTOR & INITIALIZATION
// ================================

WiFiManager::WiFiManager() :
    _deviceName(DEFAULT_DEVICE_NAME),
    _apSSID(""),
    _connectedSSID(""),
    _connectedPassword(""),
    _isConnected(false),
    _isAPActive(false),
    _shouldReconnect(false),
    _lastConnectionAttempt(0),
    _lastReconnectAttempt(0),
    _connectionStartTime(0),
    _reconnectAttempts(0),
    _dnsServer(nullptr),
    _onConnectedCallback(nullptr),
    _onDisconnectedCallback(nullptr),
    _onAccessPointStartedCallback(nullptr)
{
    _instance = this;
}

void WiFiManager::begin(const String& deviceName) {
    DEBUG_I("Initializing WiFi Manager...");
    
    setDeviceName(deviceName);
    
    // Initialize preferences
    _preferences.begin(PREFS_WIFI_NAMESPACE, false);
    
    // Load saved WiFi credentials
    _loadWiFiCredentials();
    
    // Set WiFi mode
    WiFi.mode(WIFI_AP_STA);
    
    // Setup WiFi event handler
    WiFi.onEvent(_wifiEventHandler);
    
    // Try to connect to saved WiFi first
    if (_connectedSSID.length() > 0) {
        DEBUG_I("Attempting to connect to saved WiFi: %s", _connectedSSID.c_str());
        if (!connectToWiFi(_connectedSSID, _connectedPassword)) {
            DEBUG_W("Failed to connect to saved WiFi, starting Access Point");
            startAccessPoint();
        }
    } else {
        DEBUG_I("No saved WiFi credentials, starting Access Point");
        startAccessPoint();
    }
    
    DEBUG_I("WiFi Manager initialized successfully");
}

void WiFiManager::end() {
    DEBUG_I("Shutting down WiFi Manager...");
    
    stopAccessPoint();
    disconnectWiFi();
    
    if (_dnsServer) {
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    _preferences.end();
    
    DEBUG_I("WiFi Manager shutdown complete");
}

// ================================
// MAIN LOOP HANDLER
// ================================

void WiFiManager::handleClient() {
    // Handle DNS server for captive portal
    if (_dnsServer && _isAPActive) {
        _dnsServer->processNextRequest();
    }
    
    // Handle WiFi events and reconnection
    _handleWiFiEvents();
    
    // Attempt reconnection if needed
    if (_shouldReconnect && !_isConnected) {
        _attemptReconnection();
    }
    
    // Update connection status
    _updateConnectionStatus();
}

// ================================
// WIFI CONNECTION MANAGEMENT
// ================================

bool WiFiManager::connectToWiFi(const String& ssid, const String& password) {
    if (!_isValidSSID(ssid)) {
        DEBUG_E("Invalid SSID provided");
        return false;
    }
    
    DEBUG_I("Connecting to WiFi: %s", ssid.c_str());
    
    // Disconnect from current WiFi if connected
    if (_isConnected) {
        WiFi.disconnect();
        delay(1000);
    }
    
    // Store connection details
    _connectedSSID = ssid;
    _connectedPassword = password;
    _connectionStartTime = millis();
    _reconnectAttempts = 0;
    
    // Begin connection
    if (password.length() > 0) {
        WiFi.begin(ssid.c_str(), password.c_str());
    } else {
        WiFi.begin(ssid.c_str());
    }
    
    // Wait for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           (millis() - startTime) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(500);
        DEBUG_D("Connecting...");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        _isConnected = true;
        _shouldReconnect = true;
        
        // Save credentials
        _saveWiFiCredentials();
        
        // Stop Access Point if it was running
        if (_isAPActive) {
            stopAccessPoint();
        }
        
        DEBUG_I("WiFi connected successfully!");
        DEBUG_I("IP address: %s", WiFi.localIP().toString().c_str());
        DEBUG_I("RSSI: %d dBm", WiFi.RSSI());
        
        // Call connected callback
        if (_onConnectedCallback) {
            _onConnectedCallback();
        }
        
        return true;
    } else {
        DEBUG_E("WiFi connection failed. Status: %d", WiFi.status());
        
        // Start Access Point if connection failed
        if (!_isAPActive) {
            startAccessPoint();
        }
        
        return false;
    }
}

void WiFiManager::disconnectWiFi() {
    if (_isConnected) {
        DEBUG_I("Disconnecting from WiFi");
        
        _shouldReconnect = false;
        WiFi.disconnect();
        _isConnected = false;
        
        if (_onDisconnectedCallback) {
            _onDisconnectedCallback();
        }
    }
}

bool WiFiManager::isConnected() {
    return _isConnected && (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::resetWiFiSettings() {
    DEBUG_I("Resetting WiFi settings");
    
    // Disconnect current connection
    disconnectWiFi();
    
    // Clear stored credentials
    _clearWiFiCredentials();
    
    // Start Access Point
    startAccessPoint();
    
    DEBUG_I("WiFi settings reset complete");
}

// ================================
// ACCESS POINT MANAGEMENT
// ================================

void WiFiManager::startAccessPoint() {
    if (_isAPActive) {
        DEBUG_W("Access Point already active");
        return;
    }
    
    DEBUG_I("Starting Access Point: %s", _apSSID.c_str());
    
    // Configure Access Point
    WiFi.softAPConfig(AP_IP_ADDRESS, AP_GATEWAY, AP_SUBNET);
    
    bool apStarted = WiFi.softAP(_apSSID.c_str(), AP_PASSWORD, AP_CHANNEL, 
                                 AP_HIDDEN, AP_MAX_CONNECTIONS);
    
    if (apStarted) {
        _isAPActive = true;
        
        // Setup captive portal
        _setupCaptivePortal();
        
        DEBUG_I("Access Point started successfully");
        DEBUG_I("SSID: %s", _apSSID.c_str());
        DEBUG_I("Password: %s", AP_PASSWORD);
        DEBUG_I("IP: %s", WiFi.softAPIP().toString().c_str());
        
        // Call Access Point started callback
        if (_onAccessPointStartedCallback) {
            _onAccessPointStartedCallback();
        }
    } else {
        DEBUG_E("Failed to start Access Point");
    }
}

void WiFiManager::stopAccessPoint() {
    if (!_isAPActive) {
        return;
    }
    
    DEBUG_I("Stopping Access Point");
    
    _stopCaptivePortal();
    WiFi.softAPdisconnect(true);
    _isAPActive = false;
    
    DEBUG_I("Access Point stopped");
}

bool WiFiManager::isAccessPointActive() {
    return _isAPActive;
}

// ================================
// NETWORK INFORMATION
// ================================

String WiFiManager::getConnectedSSID() {
    return _isConnected ? WiFi.SSID() : "";
}

IPAddress WiFiManager::getLocalIP() {
    return _isConnected ? WiFi.localIP() : IPAddress(0, 0, 0, 0);
}

IPAddress WiFiManager::getAccessPointIP() {
    return _isAPActive ? WiFi.softAPIP() : IPAddress(0, 0, 0, 0);
}

String WiFiManager::getMACAddress() {
    return WiFi.macAddress();
}

int WiFiManager::getRSSI() {
    return _isConnected ? WiFi.RSSI() : 0;
}

// ================================
// NETWORK SCANNING
// ================================

int WiFiManager::scanNetworks() {
    DEBUG_I("Scanning for WiFi networks...");
    
    int networkCount = WiFi.scanNetworks();
    
    if (networkCount == -1) {
        DEBUG_E("WiFi scan failed");
        return 0;
    }
    
    DEBUG_I("Found %d networks", networkCount);
    return networkCount;
}

String WiFiManager::getScannedNetworksJSON() {
    String json = "{\"networks\":[";
    
    int networkCount = WiFi.scanComplete();
    
    if (networkCount > 0) {
        for (int i = 0; i < networkCount; i++) {
            if (i > 0) json += ",";
            
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
            json += "\"encryption\":\"" + _encryptionTypeToString(WiFi.encryptionType(i)) + "\"";
            json += "}";
        }
        
        // Clear scan results
        WiFi.scanDelete();
    }
    
    json += "]}";
    return json;
}

// ================================
// STATUS INFORMATION
// ================================

String WiFiManager::getStatusJSON() {
    String json = "{";
    json += "\"connected\":" + String(_isConnected ? "true" : "false") + ",";
    json += "\"access_point_active\":" + String(_isAPActive ? "true" : "false") + ",";
    json += "\"ssid\":\"" + getConnectedSSID() + "\",";
    json += "\"local_ip\":\"" + getLocalIP().toString() + "\",";
    json += "\"access_point_ip\":\"" + getAccessPointIP().toString() + "\",";
    json += "\"rssi\":" + String(getRSSI()) + ",";
    json += "\"mac_address\":\"" + getMACAddress() + "\",";
    json += "\"reconnect_attempts\":" + String(_reconnectAttempts);
    json += "}";
    
    return json;
}

String WiFiManager::getNetworkInfoJSON() {
    String json = "{";
    
    if (_isConnected) {
        json += "\"status\":\"connected\",";
        json += "\"ssid\":\"" + WiFi.SSID() + "\",";
        json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"gateway\":\"" + WiFi.gatewayIP().toString() + "\",";
        json += "\"subnet\":\"" + WiFi.subnetMask().toString() + "\",";
        json += "\"dns\":\"" + WiFi.dnsIP().toString() + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"channel\":" + String(WiFi.channel());
    } else if (_isAPActive) {
        json += "\"status\":\"access_point\",";
        json += "\"ssid\":\"" + _apSSID + "\",";
        json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
        json += "\"clients\":" + String(WiFi.softAPgetStationNum());
    } else {
        json += "\"status\":\"disconnected\"";
    }
    
    json += ",\"mac\":\"" + WiFi.macAddress() + "\"";
    json += "}";
    
    return json;
}

// ================================
// CONFIGURATION
// ================================

void WiFiManager::setDeviceName(const String& name) {
    _deviceName = name;
    _apSSID = AP_SSID_PREFIX + name;
    
    // Sanitize AP SSID
    _apSSID = _sanitizeSSID(_apSSID);
    
    DEBUG_D("Device name set to: %s", _deviceName.c_str());
    DEBUG_D("AP SSID set to: %s", _apSSID.c_str());
}

String WiFiManager::getDeviceName() {
    return _deviceName;
}

String WiFiManager::getAccessPointSSID() {
    return _apSSID;
}

// ================================
// CALLBACK REGISTRATION
// ================================

void WiFiManager::onConnected(std::function<void()> callback) {
    _onConnectedCallback = callback;
}

void WiFiManager::onDisconnected(std::function<void()> callback) {
    _onDisconnectedCallback = callback;
}

void WiFiManager::onAccessPointStarted(std::function<void()> callback) {
    _onAccessPointStartedCallback = callback;
}

// ================================
// PRIVATE METHODS
// ================================

void WiFiManager::_loadWiFiCredentials() {
    _connectedSSID = _preferences.getString(PREF_WIFI_SSID, "");
    _connectedPassword = _preferences.getString(PREF_WIFI_PASSWORD, "");
    
    if (_connectedSSID.length() > 0) {
        DEBUG_I("Loaded WiFi credentials for: %s", _connectedSSID.c_str());
    } else {
        DEBUG_I("No saved WiFi credentials found");
    }
}

void WiFiManager::_saveWiFiCredentials() {
    _preferences.putString(PREF_WIFI_SSID, _connectedSSID);
    _preferences.putString(PREF_WIFI_PASSWORD, _connectedPassword);
    
    DEBUG_I("WiFi credentials saved");
}

void WiFiManager::_clearWiFiCredentials() {
    _preferences.remove(PREF_WIFI_SSID);
    _preferences.remove(PREF_WIFI_PASSWORD);
    
    _connectedSSID = "";
    _connectedPassword = "";
    
    DEBUG_I("WiFi credentials cleared");
}

void WiFiManager::_handleWiFiEvents() {
    // Check if we lost connection
    if (_isConnected && WiFi.status() != WL_CONNECTED) {
        DEBUG_W("WiFi connection lost");
        _isConnected = false;
        
        if (_onDisconnectedCallback) {
            _onDisconnectedCallback();
        }
        
        // Start reconnection attempts
        if (_shouldReconnect) {
            _lastReconnectAttempt = millis();
        }
    }
}

void WiFiManager::_attemptReconnection() {
    unsigned long currentTime = millis();
    
    if (currentTime - _lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {
        if (_reconnectAttempts < WIFI_MAX_RECONNECT_ATTEMPTS) {
            DEBUG_I("Attempting WiFi reconnection (%d/%d)", 
                   _reconnectAttempts + 1, WIFI_MAX_RECONNECT_ATTEMPTS);
            
            WiFi.disconnect();
            delay(1000);
            
            if (_connectedPassword.length() > 0) {
                WiFi.begin(_connectedSSID.c_str(), _connectedPassword.c_str());
            } else {
                WiFi.begin(_connectedSSID.c_str());
            }
            
            _reconnectAttempts++;
            _lastReconnectAttempt = currentTime;
        } else {
            DEBUG_W("Max reconnection attempts reached, starting Access Point");
            _shouldReconnect = false;
            _reconnectAttempts = 0;
            
            if (!_isAPActive) {
                startAccessPoint();
            }
        }
    }
}

bool WiFiManager::_isValidSSID(const String& ssid) {
    return ssid.length() > 0 && ssid.length() <= 32;
}

bool WiFiManager::_isValidPassword(const String& password) {
    return password.length() == 0 || (password.length() >= 8 && password.length() <= 63);
}

String WiFiManager::_sanitizeSSID(const String& ssid) {
    String sanitized = ssid;
    
    // Remove invalid characters
    sanitized.replace(" ", "-");
    sanitized.replace("_", "-");
    
    // Limit length
    if (sanitized.length() > 32) {
        sanitized = sanitized.substring(0, 32);
    }
    
    return sanitized;
}

void WiFiManager::_updateConnectionStatus() {
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);
    
    if (currentlyConnected && !_isConnected) {
        _isConnected = true;
        _reconnectAttempts = 0;
        
        DEBUG_I("WiFi connection established");
        
        if (_onConnectedCallback) {
            _onConnectedCallback();
        }
    }
}

void WiFiManager::_setupCaptivePortal() {
    if (!_dnsServer) {
        _dnsServer = new DNSServer();
    }
    
    // Start DNS server for captive portal
    _dnsServer->start(DNS_PORT, "*", AP_IP_ADDRESS);
    
    DEBUG_I("Captive portal DNS server started");
}

void WiFiManager::_stopCaptivePortal() {
    if (_dnsServer) {
        _dnsServer->stop();
        DEBUG_I("Captive portal DNS server stopped");
    }
}

String WiFiManager::_encryptionTypeToString(wifi_auth_mode_t encryptionType) {
    switch (encryptionType) {
        case WIFI_AUTH_OPEN: return "none";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        default: return "unknown";
    }
}

// ================================
// STATIC EVENT HANDLER
// ================================

void WiFiManager::_wifiEventHandler(WiFiEvent_t event) {
    if (!_instance) return;
    
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            DEBUG_D("WiFi event: Station connected");
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            DEBUG_D("WiFi event: Station disconnected");
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            DEBUG_D("WiFi event: Got IP address");
            break;
            
        case SYSTEM_EVENT_AP_START:
            DEBUG_D("WiFi event: Access Point started");
            break;
            
        case SYSTEM_EVENT_AP_STOP:
            DEBUG_D("WiFi event: Access Point stopped");
            break;
            
        default:
            break;
    }
}

