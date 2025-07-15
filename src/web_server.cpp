 #include "web_server.h"
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "html_pages.h"

// Static instance pointer
WebServerManager* WebServerManager::_instance = nullptr;

// ================================
// CONSTRUCTOR & INITIALIZATION
// ================================

WebServerManager::WebServerManager() :
    _server(nullptr),
    _webSocket(nullptr),
    _wifiManager(nullptr),
    _sensorManager(nullptr),
    _isRunning(false),
    _startTime(0),
    _requestCount(0),
    _errorCount(0),
    _lastBroadcast(0),
    _onDeviceNameChangeCallback(nullptr),
    _onLEDControlCallback(nullptr),
    _onFactoryResetCallback(nullptr),
    _onRestartCallback(nullptr)
{
    _instance = this;
}

void WebServerManager::begin() {
    DEBUG_I("Initializing Web Server Manager...");
    
    // Create server instance
    _server = new AsyncWebServer(WEB_SERVER_PORT);
    _webSocket = new AsyncWebSocket(WEBSOCKET_PATH);
    
    // Setup routes and handlers
    _setupRoutes();
    _setupWebSocketHandlers();
    _setupCORSHeaders();
    
    // Start server
    start();
    
    DEBUG_I("Web Server Manager initialized successfully");
}

void WebServerManager::end() {
    DEBUG_I("Shutting down Web Server Manager...");
    
    stop();
    
    if (_webSocket) {
        delete _webSocket;
        _webSocket = nullptr;
    }
    
    if (_server) {
        delete _server;
        _server = nullptr;
    }
    
    DEBUG_I("Web Server Manager shutdown complete");
}

// ================================
// SERVER CONTROL
// ================================

void WebServerManager::start() {
    if (_isRunning || !_server) {
        return;
    }
    
    DEBUG_I("Starting web server on port %d", WEB_SERVER_PORT);
    
    _server->begin();
    _isRunning = true;
    _startTime = millis();
    
    DEBUG_I("Web server started successfully");
}

void WebServerManager::stop() {
    if (!_isRunning || !_server) {
        return;
    }
    
    DEBUG_I("Stopping web server");
    
    _server->end();
    _isRunning = false;
    
    DEBUG_I("Web server stopped");
}

bool WebServerManager::isRunning() {
    return _isRunning;
}

// ================================
// MAIN LOOP HANDLER
// ================================

void WebServerManager::handleClient() {
    // WebSocket cleanup
    if (_webSocket) {
        _webSocket->cleanupClients();
    }
    
    // Periodic sensor data broadcast
    unsigned long currentTime = millis();
    if (currentTime - _lastBroadcast >= SENSOR_UPDATE_INTERVAL) {
        broadcastSensorData();
        _lastBroadcast = currentTime;
    }
}

// ================================
// WEBSOCKET MANAGEMENT
// ================================

void WebServerManager::broadcastMessage(const String& message) {
    if (_webSocket && _webSocket->count() > 0) {
        _webSocket->textAll(message);
        DEBUG_V("Broadcast message to %d clients", _webSocket->count());
    }
}

void WebServerManager::broadcastSensorData() {
    if (_sensorManager) {
        String sensorData = _sensorManager->getSensorDataJSON();
        broadcastMessage(sensorData);
    }
}

void WebServerManager::broadcastDeviceStats() {
    if (_sensorManager) {
        String deviceStats = _sensorManager->getDeviceStatsJSON();
        broadcastMessage(deviceStats);
    }
}

int WebServerManager::getWebSocketClientCount() {
    return _webSocket ? _webSocket->count() : 0;
}

// ================================
// MANAGER REFERENCES
// ================================

void WebServerManager::setWiFiManager(WiFiManager* wifiManager) {
    _wifiManager = wifiManager;
}

void WebServerManager::setSensorManager(SensorManager* sensorManager) {
    _sensorManager = sensorManager;
}

// ================================
// CALLBACK REGISTRATION
// ================================

void WebServerManager::onDeviceNameChange(std::function<void(const String&)> callback) {
    _onDeviceNameChangeCallback = callback;
}

void WebServerManager::onLEDControl(std::function<void(bool)> callback) {
    _onLEDControlCallback = callback;
}

void WebServerManager::onFactoryReset(std::function<void()> callback) {
    _onFactoryResetCallback = callback;
}

void WebServerManager::onRestart(std::function<void()> callback) {
    _onRestartCallback = callback;
}

// ================================
// ROUTE SETUP
// ================================

void WebServerManager::_setupRoutes() {
    if (!_server) return;
    
    DEBUG_I("Setting up web server routes...");
    
    // Root page handler
    _server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleRoot(request);
    });
    
    // API Routes
    _server->on((API_PREFIX + API_SCAN).c_str(), HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleAPIScan(request);
    });
    
    _server->on((API_PREFIX + API_CONNECT).c_str(), HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handleAPIConnect(request);
    });
    
    _server->on((API_PREFIX + API_STATUS).c_str(), HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleAPIStatus(request);
    });
    
    _server->on((API_PREFIX + API_SENSOR_DATA).c_str(), HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleAPISensorData(request);
    });
    
    _server->on((API_PREFIX + API_DEVICE_STATS).c_str(), HTTP_GET, [this](AsyncWebServerRequest* request) {
        _handleAPIDeviceStats(request);
    });
    
    _server->on((API_PREFIX + API_DEVICE_NAME).c_str(), HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handleAPIDeviceName(request);
    });
    
    _server->on((API_PREFIX + API_LED_CONTROL).c_str(), HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handleAPILEDControl(request);
    });
    
    _server->on((API_PREFIX + API_FACTORY_RESET).c_str(), HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handleAPIFactoryReset(request);
    });
    
    _server->on((API_PREFIX + API_RESTART).c_str(), HTTP_POST, [this](AsyncWebServerRequest* request) {
        _handleAPIRestart(request);
    });
    
    // 404 handler
    _server->onNotFound([this](AsyncWebServerRequest* request) {
        _handleNotFound(request);
    });
    
    DEBUG_I("Web server routes configured");
}

void WebServerManager::_setupWebSocketHandlers() {
    if (!_webSocket || !_server) return;
    
    DEBUG_I("Setting up WebSocket handlers...");
    
    _webSocket->onEvent(_staticWebSocketEvent);
    _server->addHandler(_webSocket);
    
    DEBUG_I("WebSocket handlers configured");
}

void WebServerManager::_setupCORSHeaders() {
    if (!_server) return;
    
    // Add CORS headers to all responses
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    DefaultHeaders::Instance().addHeader("Access-Control-Max-Age", String(CORS_MAX_AGE));
    
    DEBUG_I("CORS headers configured");
}

// ================================
// PAGE HANDLERS
// ================================

void WebServerManager::_handleRoot(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_D("Handling root request from: %s", request->client()->remoteIP().toString().c_str());
    
    String html;
    
    if (_wifiManager && _wifiManager->isConnected()) {
        // Show dashboard if connected to WiFi
        html = getDashboardHTML();
    } else {
        // Show WiFi setup if not connected
        html = getWiFiSetupHTML();
    }
    
    AsyncWebServerResponse* response = request->beginResponse(200, "text/html", html);
    _addCORSHeaders(response);
    request->send(response);
}

void WebServerManager::_handleNotFound(AsyncWebServerRequest* request) {
    _errorCount++;
    
    DEBUG_W("404 Not Found: %s", request->url().c_str());
    
    // For captive portal, redirect to root
    if (_wifiManager && _wifiManager->isAccessPointActive()) {
        AsyncWebServerResponse* response = request->beginResponse(302);
        response->addHeader("Location", "http://" + _wifiManager->getAccessPointIP().toString());
        _addCORSHeaders(response);
        request->send(response);
    } else {
        _sendErrorResponse(request, "Page not found", 404);
    }
}

// ================================
// API HANDLERS
// ================================

void WebServerManager::_handleAPIScan(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_D("API: WiFi scan request");
    
    if (!_wifiManager) {
        _sendErrorResponse(request, "WiFi manager not available");
        return;
    }
    
    // Start network scan
    int networkCount = _wifiManager->scanNetworks();
    
    if (networkCount >= 0) {
        String networksJSON = _wifiManager->getScannedNetworksJSON();
        _sendJSONResponse(request, networksJSON);
    } else {
        _sendErrorResponse(request, "Network scan failed");
    }
}

void WebServerManager::_handleAPIConnect(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_D("API: WiFi connect request");
    
    if (!_wifiManager) {
        _sendErrorResponse(request, "WiFi manager not available");
        return;
    }
    
    // Get POST parameters
    String ssid = "";
    String password = "";
    
    if (request->hasParam("ssid", true)) {
        ssid = request->getParam("ssid", true)->value();
    }
    
    if (request->hasParam("password", true)) {
        password = request->getParam("password", true)->value();
    }
    
    if (ssid.length() == 0) {
        _sendErrorResponse(request, "SSID is required");
        return;
    }
    
    // Attempt connection
    bool connected = _wifiManager->connectToWiFi(ssid, password);
    
    if (connected) {
        String response = "{\"success\":true,\"message\":\"Connected to " + ssid + "\"}";
        _sendJSONResponse(request, response);
    } else {
        _sendErrorResponse(request, "Failed to connect to " + ssid);
    }
}

void WebServerManager::_handleAPIStatus(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_V("API: Status request");
    
    String statusJSON = "{\"server\":" + getServerStatus();
    
    if (_wifiManager) {
        statusJSON += ",\"wifi\":" + _wifiManager->getStatusJSON();
    }
    
    if (_sensorManager) {
        statusJSON += ",\"sensors\":" + _sensorManager->getSensorDataJSON();
    }
    
    statusJSON += "}";
    
    _sendJSONResponse(request, statusJSON);
}

void WebServerManager::_handleAPISensorData(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_V("API: Sensor data request");
    
    if (_sensorManager) {
        String sensorData = _sensorManager->getSensorDataJSON();
        _sendJSONResponse(request, sensorData);
    } else {
        _sendErrorResponse(request, "Sensor manager not available");
    }
}

void WebServerManager::_handleAPIDeviceStats(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_V("API: Device stats request");
    
    if (_sensorManager) {
        String deviceStats = _sensorManager->getDeviceStatsJSON();
        _sendJSONResponse(request, deviceStats);
    } else {
        _sendErrorResponse(request, "Sensor manager not available");
    }
}

void WebServerManager::_handleAPIDeviceName(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_D("API: Device name change request");
    
    String newName = "";
    
    if (request->hasParam("name", true)) {
        newName = request->getParam("name", true)->value();
    }
    
    if (!_validateDeviceName(newName)) {
        _sendErrorResponse(request, "Invalid device name. Must be 3-32 characters, alphanumeric with hyphens/underscores only");
        return;
    }
    
    // Call the callback to change device name
    if (_onDeviceNameChangeCallback) {
        _onDeviceNameChangeCallback(newName);
        
        String response = "{\"success\":true,\"message\":\"Device name changed to: " + newName + "\"}";
        _sendJSONResponse(request, response);
    } else {
        _sendErrorResponse(request, "Device name change not supported");
    }
}

void WebServerManager::_handleAPILEDControl(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_D("API: LED control request");
    
    if (!request->hasParam("state", true)) {
        _sendErrorResponse(request, "LED state parameter required");
        return;
    }
    
    String stateParam = request->getParam("state", true)->value();
    bool ledState = (stateParam == "true" || stateParam == "1" || stateParam == "on");
    
    if (_onLEDControlCallback) {
        _onLEDControlCallback(ledState);
        
        String response = "{\"success\":true,\"message\":\"LED turned " + String(ledState ? "on" : "off") + "\"}";
        _sendJSONResponse(request, response);
    } else {
        _sendErrorResponse(request, "LED control not supported");
    }
}

void WebServerManager::_handleAPIFactoryReset(AsyncWebServerRequest* request) {
    _requestCount++;
    
    DEBUG_I("API: Factory reset request");
    
    String response = "{\"success\":true,\"message\":\"Factory reset initiated. Device will restart in 3 seconds.\"}";
    _sendJSONResponse(request, response);
    
    // Delay and then call factory reset
    if (_onFac

