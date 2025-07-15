#include "sensor_manager.h"
#include <algorithm>
#include <numeric>

// ================================
// CONSTRUCTOR & INITIALIZATION
// ================================

SensorManager::SensorManager() :
    _maxHistorySize(SENSOR_HISTORY_SIZE),
    _statsValid(false),
    _temperatureEnabled(SENSOR_TEMPERATURE),
    _humidityEnabled(SENSOR_HUMIDITY),
    _pressureEnabled(SENSOR_PRESSURE),
    _lightEnabled(SENSOR_LIGHT),
    _motionEnabled(SENSOR_MOTION),
    _batteryEnabled(SENSOR_BATTERY),
    _lastUpdate(0),
    _updateInterval(SENSOR_UPDATE_INTERVAL),
    _lastStatsUpdate(0),
    _tempBase(TEMP_BASE),
    _tempTrend(0.0),
    _humidityBase(HUMIDITY_BASE),
    _humidityTrend(0.0),
    _pressureBase(PRESSURE_BASE),
    _pressureTrend(0.0),
    _lightBase(50.0),
    _lightTrend(0.0),
    _motionActive(false),
    _motionStartTime(0),
    _lastMotionEvent(0),
    _motionEventCount(0),
    _batteryLevel(85.0),
    _batteryCharging(false),
    _lastBatteryUpdate(0),
    _tempOffset(0.0),
    _humidityOffset(0.0),
    _pressureOffset(0.0),
    _uptimeCallback(nullptr),
    _bootCountCallback(nullptr),
    _totalConnectionsCallback(nullptr),
    _wifiSSIDCallback(nullptr),
    _wifiRSSICallback(nullptr),
    _ledStateCallback(nullptr),
    _webSocketClientsCallback(nullptr)
{
    // Initialize current reading
    memset(&_currentReading, 0, sizeof(SensorReading));
    memset(&_stats, 0, sizeof(SensorStats));
    
    // Reserve history vector
    _history.reserve(_maxHistorySize);
}

void SensorManager::begin() {
    DEBUG_I("Initializing Sensor Manager...");
    
    // Initialize random seed for sensor simulation
    randomSeed(analogRead(0) + millis());
    
    // Set initial sensor values
    _currentReading.timestamp = millis();
    _currentReading.temperature = _tempBase;
    _currentReading.humidity = _humidityBase;
    _currentReading.pressure = _pressureBase;
    _currentReading.lightLevel = _lightBase;
    _currentReading.motionDetected = false;
    _currentReading.batteryLevel = _batteryLevel;
    
    // Initialize statistics
    _stats.minTemperature = _tempBase;
    _stats.maxTemperature = _tempBase;
    _stats.avgTemperature = _tempBase;
    _stats.minHumidity = _humidityBase;
    _stats.maxHumidity = _humidityBase;
    _stats.avgHumidity = _humidityBase;
    _stats.minPressure = _pressureBase;
    _stats.maxPressure = _pressureBase;
    _stats.avgPressure = _pressureBase;
    _stats.minLightLevel = _lightBase;
    _stats.maxLightLevel = _lightBase;
    _stats.avgLightLevel = _lightBase;
    _stats.motionEvents = 0;
    _stats.lastMotionTime = 0;
    _stats.batteryHealth = 100.0;
    _stats.dataPoints = 0;
    
    DEBUG_I("Sensor Manager initialized successfully");
    DEBUG_I("Enabled sensors: T:%d H:%d P:%d L:%d M:%d B:%d", 
           _temperatureEnabled, _humidityEnabled, _pressureEnabled,
           _lightEnabled, _motionEnabled, _batteryEnabled);
}

void SensorManager::end() {
    DEBUG_I("Shutting down Sensor Manager...");
    
    _history.clear();
    _statsValid = false;
    
    DEBUG_I("Sensor Manager shutdown complete");
}

// ================================
// MAIN UPDATE LOOP
// ================================

void SensorManager::update() {
    unsigned long currentTime = millis();
    
    // Check if it's time to update sensors
    if (currentTime - _lastUpdate >= _updateInterval) {
        _updateSensors();
        _lastUpdate = currentTime;
        
        // Add to history
        _addToHistory(_currentReading);
        
        // Update statistics periodically
        if (currentTime - _lastStatsUpdate >= STATS_UPDATE_INTERVAL) {
            _updateStatistics();
            _lastStatsUpdate = currentTime;
        }
    }
    
    // Handle motion detection timeout
    if (_motionActive && (currentTime - _motionStartTime) >= MOTION_DURATION_MS) {
        _motionActive = false;
        _currentReading.motionDetected = false;
        DEBUG_V("Motion detection timeout");
    }
}

// ================================
// SENSOR CONTROL
// ================================

void SensorManager::enableSensor(const String& sensorName, bool enabled) {
    String sensor = sensorName;
    sensor.toLowerCase();
    
    if (sensor == "temperature") {
        _temperatureEnabled = enabled;
    } else if (sensor == "humidity") {
        _humidityEnabled = enabled;
    } else if (sensor == "pressure") {
        _pressureEnabled = enabled;
    } else if (sensor == "light") {
        _lightEnabled = enabled;
    } else if (sensor == "motion") {
        _motionEnabled = enabled;
    } else if (sensor == "battery") {
        _batteryEnabled = enabled;
    }
    
    DEBUG_I("Sensor %s %s", sensorName.c_str(), enabled ? "enabled" : "disabled");
}

bool SensorManager::isSensorEnabled(const String& sensorName) {
    String sensor = sensorName;
    sensor.toLowerCase();
    
    if (sensor == "temperature") return _temperatureEnabled;
    if (sensor == "humidity") return _humidityEnabled;
    if (sensor == "pressure") return _pressureEnabled;
    if (sensor == "light") return _lightEnabled;
    if (sensor == "motion") return _motionEnabled;
    if (sensor == "battery") return _batteryEnabled;
    
    return false;
}

void SensorManager::setUpdateInterval(unsigned long interval) {
    _updateInterval = max(interval, 100UL); // Minimum 100ms
    DEBUG_I("Sensor update interval set to %lu ms", _updateInterval);
}

unsigned long SensorManager::getUpdateInterval() {
    return _updateInterval;
}

// ================================
// DATA ACCESS
// ================================

SensorReading SensorManager::getCurrentReading() {
    return _currentReading;
}

std::vector<SensorReading> SensorManager::getHistory() {
    return _history;
}

SensorStats SensorManager::getStatistics() {
    if (!_statsValid) {
        _calculateStatistics();
    }
    return _stats;
}

DeviceStats SensorManager::getDeviceStatistics() {
    DeviceStats stats;
    
    // Get system information
    stats.uptime = _uptimeCallback ? _uptimeCallback() : millis();
    stats.bootCount = _bootCountCallback ? _bootCountCallback() : 0;
    stats.totalConnections = _totalConnectionsCallback ? _totalConnectionsCallback() : 0;
    stats.freeHeap = ESP.getFreeHeap();
    stats.totalHeap = ESP.getHeapSize();
    stats.cpuUsage = 0.0; // Simplified - would need more complex calculation
    stats.wifiSSID = _wifiSSIDCallback ? _wifiSSIDCallback() : "";
    stats.wifiRSSI = _wifiRSSICallback ? _wifiRSSICallback() : 0;
    stats.localIP = WiFi.localIP();
    stats.macAddress = WiFi.macAddress();
    stats.temperature = ESP.getTemperature();
    stats.ledState = _ledStateCallback ? _ledStateCallback() : false;
    stats.webSocketClients = _webSocketClientsCallback ? _webSocketClientsCallback() : 0;
    
    return stats;
}

// ================================
// JSON OUTPUT
// ================================

String SensorManager::getSensorDataJSON() {
    DynamicJsonDocument doc(1024);
    
    doc["timestamp"] = _currentReading.timestamp;
    
    if (_temperatureEnabled) {
        doc["temperature"] = round(_currentReading.temperature * 10) / 10.0;
    }
    
    if (_humidityEnabled) {
        doc["humidity"] = round(_currentReading.humidity * 10) / 10.0;
    }
    
    if (_pressureEnabled) {
        doc["pressure"] = round(_currentReading.pressure * 100) / 100.0;
    }
    
    if (_lightEnabled) {
        doc["light_level"] = round(_currentReading.lightLevel * 10) / 10.0;
    }
    
    if (_motionEnabled) {
        doc["motion_detected"] = _currentReading.motionDetected;
    }
    
    if (_batteryEnabled) {
        doc["battery_level"] = round(_currentReading.batteryLevel * 10) / 10.0;
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String SensorManager::getSensorHistoryJSON() {
    DynamicJsonDocument doc(4096);
    JsonArray historyArray = doc.createNestedArray("history");
    
    // Get last 20 readings for history
    int startIndex = max(0, (int)_history.size() - 20);
    
    for (int i = startIndex; i < _history.size(); i++) {
        JsonObject reading = historyArray.createNestedObject();
        reading["timestamp"] = _history[i].timestamp;
        
        if (_temperatureEnabled) {
            reading["temperature"] = round(_history[i].temperature * 10) / 10.0;
        }
        
        if (_humidityEnabled) {
            reading["humidity"] = round(_history[i].humidity * 10) / 10.0;
        }
        
        if (_pressureEnabled) {
            reading["pressure"] = round(_history[i].pressure * 100) / 100.0;
        }
        
        if (_lightEnabled) {
            reading["light_level"] = round(_history[i].lightLevel * 10) / 10.0;
        }
        
        if (_batteryEnabled) {
            reading["battery_level"] = round(_history[i].batteryLevel * 10) / 10.0;
        }
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

String SensorManager::getSensorStatsJSON() {
    if (!_statsValid) {
        _calculateStatistics();
    }
    
    DynamicJsonDocument doc(1024);
    
    if (_temperatureEnabled) {
        JsonObject temp = doc.createNestedObject("temperature");
        temp["min"] = round(_stats.minTemperature * 10) / 10.0;
        temp["max"] = round(_stats.maxTemperature * 10) / 10.0;
        temp["avg"] = round(_stats.avgTemperature * 10) / 10.0;
    }
    
    if (_humidityEnabled) {
        JsonObject humidity = doc.createNestedObject("humidity");
        humidity["min"] = round(_stats.minHumidity * 10) / 10.0;
        humidity["max"] = round(_stats.maxHumidity * 10) / 10.0;
        humidity["avg"] = round(_stats.avgHumidity * 10) / 10.0;
    }
    
    if (_pressureEnabled) {
        JsonObject pressure = doc.createNestedObject("pressure");
        pressure["min"] = round(_stats.minPressure * 100) / 100.0;
        pressure["max"] = round(_stats.maxPressure * 100) / 100.0;
        pressure["avg"] = round(_stats.avgPressure * 100) / 100.0;
    }
    
    if (_lightEnabled) {
        JsonObject light = doc.createNestedObject("light");
        light["min"] = round(_stats.minLightLevel * 10) / 10.0;
        light["max"] = round(_stats.maxLightLevel * 10) / 10.0;
        light["avg"] = round(_stats.avgLightLevel * 10) / 10.0;
    }
    
    if (_motionEnabled) {
        JsonObject motion = doc.createNestedObject("motion");
        motion["events"] = _stats.motionEvents;
        motion["last_detection"] = _stats.lastMotionTime;
    }
    
    if (_batteryEnabled) {
        JsonObject battery = doc.createNestedObject("battery");
        battery["level"] = round(_currentReading.batteryLevel * 10) / 10.0;
        battery["health"] = round(_stats.batteryHealth * 10) / 10.0;
    }
    
    doc["data_points"] = _stats.dataPoints;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String SensorManager::getDeviceStatsJSON() {
    DeviceStats stats = getDeviceStatistics();
    
    DynamicJsonDocument doc(1024);
    
    doc["uptime"] = stats.uptime;
    doc["boot_count"] = stats.bootCount;
    doc["total_connections"] = stats.totalConnections;
    doc["free_heap"] = stats.freeHeap;
    doc["total_heap"] = stats.totalHeap;
    doc["heap_usage"] = round(((float)(stats.totalHeap - stats.freeHeap) / stats.totalHeap) * 1000) / 10.0;
    doc["wifi_ssid"] = stats.wifiSSID;
    doc["wifi_rssi"] = stats.wifiRSSI;
    doc["local_ip"] = stats.localIP.toString();
    doc["mac_address"] = stats.macAddress;
    doc["chip_temperature"] = round(stats.temperature * 10) / 10.0;
    doc["led_state"] = stats.ledState;
    doc["websocket_clients"] = stats.webSocketClients;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String SensorManager::getAllDataJSON() {
    DynamicJsonDocument doc(2048);
    
    // Current sensor data
    JsonObject sensors = doc.createNestedObject("sensors");
    DynamicJsonDocument sensorDoc(1024);
    deserializeJson(sensorDoc, getSensorDataJSON());
    sensors.set(sensorDoc.as<JsonObject>());
    
    // Device statistics
    JsonObject device = doc.createNestedObject("device");
    DynamicJsonDocument deviceDoc(1024);
    deserializeJson(deviceDoc, getDeviceStatsJSON());
    device.set(deviceDoc.as<JsonObject>());
    
    // Sensor statistics
    JsonObject stats = doc.createNestedObject("statistics");
    DynamicJsonDocument statsDoc(1024);
    deserializeJson(statsDoc, getSensorStatsJSON());
    stats.set(statsDoc.as<JsonObject>());
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ================================
// DATA MANAGEMENT
// ================================

void SensorManager::clearHistory() {
    _history.clear();
    _statsValid = false;
    DEBUG_I("Sensor history cleared");
}

void SensorManager::resetStatistics() {
    memset(&_stats, 0, sizeof(SensorStats));
    _statsValid = false;
    DEBUG_I("Sensor statistics reset");
}

int SensorManager::getHistorySize() {
    return _history.size();
}

void SensorManager::setHistorySize(int size) {
    _maxHistorySize = max(size, 10); // Minimum 10 readings
    
    // Trim history if needed
    while (_history.size() > _maxHistorySize) {
        _history.erase(_history.begin());
    }
    
    DEBUG_I("History size set to %d", _maxHistorySize);
}

// ================================
// CALIBRATION
// ================================

void SensorManager::calibrateTemperature(float offset) {
    _tempOffset = offset;
    DEBUG_I("Temperature calibration offset: %.2f°C", offset);
}

void SensorManager::calibrateHumidity(float offset) {
    _humidityOffset = offset;
    DEBUG_I("Humidity calibration offset: %.2f%%", offset);
}

void SensorManager::calibratePressure(float offset) {
    _pressureOffset = offset;
    DEBUG_I("Pressure calibration offset: %.2f hPa", offset);
}

// ================================
// BATTERY MANAGEMENT
// ================================

void SensorManager::setBatteryLevel(float level) {
    _batteryLevel = constrain(level, 0.0, 100.0);
    _currentReading.batteryLevel = _batteryLevel;
}

float SensorManager::getBatteryLevel() {
    return _batteryLevel;
}

bool SensorManager::isBatteryLow() {
    return _batteryLevel < BATTERY_RECHARGE_THRESHOLD;
}

// ================================
// MOTION DETECTION
// ================================

bool SensorManager::isMotionDetected() {
    return _motionActive;
}

unsigned long SensorManager::getLastMotionTime() {
    return _lastMotionEvent;
}

int SensorManager::getMotionEventCount() {
    return _motionEventCount;
}

// ================================
// CALLBACK REGISTRATION
// ================================

void SensorManager::setUptimeCallback(std::function<unsigned long()> callback) {
    _uptimeCallback = callback;
}

void SensorManager::setBootCountCallback(std::function<uint32_t()> callback) {
    _bootCountCallback = callback;
}

void SensorManager::setTotalConnectionsCallback(std::function<uint32_t()> callback) {
    _totalConnectionsCallback = callback;
}

void SensorManager::setWiFiInfoCallback(std::function<String()> ssidCallback, std::function<int()> rssiCallback) {
    _wifiSSIDCallback = ssidCallback;
    _wifiRSSICallback = rssiCallback;
}

void SensorManager::setLEDStateCallback(std::function<bool()> callback) {
    _ledStateCallback = callback;
}

void SensorManager::setWebSocketClientsCallback(std::function<int()> callback) {
    _webSocketClientsCallback = callback;
}

// ================================
// PRIVATE METHODS
// ================================

void SensorManager::_updateSensors() {
    _currentReading.timestamp = millis();
    
    if (_temperatureEnabled) {
        _updateTemperature();
    }
    
    if (_humidityEnabled) {
        _updateHumidity();
    }
    
    if (_pressureEnabled) {
        _updatePressure();
    }
    
    if (_lightEnabled) {
        _updateLightLevel();
    }
    
    if (_motionEnabled) {
        _updateMotionDetection();
    }
    
    if (_batteryEnabled) {
        _updateBatteryLevel();
    }
    
    DEBUG_V("Sensors updated - T:%.1f H:%.1f P:%.1f L:%.1f M:%d B:%.1f", 
           _currentReading.temperature, _currentReading.humidity, 
           _currentReading.pressure, _currentReading.lightLevel,
           _currentReading.motionDetected, _currentReading.batteryLevel);
}

void SensorManager::_updateTemperature() {
    _currentReading.temperature = _generateSensorValue(_tempBase, TEMP_VARIATION, _tempTrend);
    _currentReading.temperature += _tempOffset;
    _currentReading.temperature = _applyNoise(_currentReading.temperature, 0.1);
}

void SensorManager::_updateHumidity() {
    _currentReading.humidity = _generateSensorValue(_humidityBase, HUMIDITY_VARIATION, _humidityTrend);
    _currentReading.humidity += _humidityOffset;
    _currentReading.humidity = constrain(_currentReading.humidity, 0.0, 100.0);
    _currentReading.humidity = _applyNoise(_currentReading.humidity, 0.5);
}

void SensorManager::_updatePressure() {
    _currentReading.pressure = _generateSensorValue(_pressureBase, PRESSURE_VARIATION, _pressureTrend);
    _currentReading.pressure += _pressureOffset;
    _currentReading.pressure = _applyNoise(_currentReading.pressure, 0.5);
}

void SensorManager::_updateLightLevel() {
    // Simulate day/night cycle
    unsigned long timeOfDay = (millis() / 1000) % 86400; // Seconds in a day
    float dayFactor = sin((timeOfDay * 2 * PI) / 86400) * 0.5 + 0.5;
    
    _lightBase = 20.0 + (dayFactor * 80.0); // 20-100% light level
    _currentReading.lightLevel = _generateSensorValue(_lightBase, 10.0, _lightTrend);
    _currentReading.lightLevel = constrain(_currentReading.lightLevel, 0.0, 100.0);
    _currentReading.lightLevel = _applyNoise(_currentReading.lightLevel, 1.0);
}

void SensorManager::_updateMotionDetection() {
    if (!_motionActive && _shouldTriggerMotion()) {
        _motionActive = true;
        _motionStartTime = millis();
        _lastMotionEvent = _motionStartTime;
        _motionEventCount++;
        DEBUG_D("Motion detected! Event #%d", _motionEventCount);
    }
    
    _currentReading.motionDetected = _motionActive;
}

void SensorManager::_updateBatteryLevel() {
    _simulateBatteryDrain();
    _currentReading.batteryLevel = _batteryLevel;
}

void SensorManager::_addToHistory(const SensorReading& reading) {
    _history.push_back(reading);
    
    // Maintain history size limit
    while (_history.size() > _maxHistorySize) {
        _history.erase(_history.begin());
    }
    
    _statsValid = false; // Invalidate statistics
}

void SensorManager::_updateStatistics() {
    _calculateStatistics();
}

void SensorManager::_calculateStatistics() {
    if (_history.empty()) {
        _statsValid = false;
        return;
    }
    
    // Initialize min/max values
    _stats.minTemperature = _history[0].temperature;
    _stats.maxTemperature = _history[0].temperature;
    _stats.minHumidity = _history[0].humidity;
    _stats.maxHumidity = _history[0].humidity;
    _stats.minPressure = _history[0].pressure;
    _stats.maxPressure = _history[0].pressure;
    _stats.minLightLevel = _history[0].lightLevel;
    _stats.maxLightLevel = _history[0].lightLevel;
    
    // Calculate sums for averages
    float tempSum = 0, humiditySum = 0, pressureSum = 0, lightSum = 0;
    
    for (const auto& reading : _history) {
        // Temperature
        _stats.minTemperature = min(_stats.minTemperature, reading.temperature);
        _stats.maxTemperature = max(_stats.maxTemperature, reading.temperature);
        tempSum += reading.temperature;
        
        // Humidity
        _stats.minHumidity = min(_stats.minHumidity, reading.humidity);
        _stats.maxHumidity = max(_stats.maxHumidity, reading.humidity);
        humiditySum += reading.humidity;
        
        // Pressure
        _stats.minPressure = min(_stats.minPressure, reading.pressure);
        _stats.maxPressure = max(_stats.maxPressure, reading.pressure);
        pressureSum += reading.pressure;
        
        // Light
        _stats.minLightLevel = min(_stats.minLightLevel, reading.lightLevel);
        _stats.maxLightLevel = max(_stats.maxLightLevel, reading.lightLevel);
        lightSum += reading.lightLevel;
    }
    
    // Calculate averages
    int count = _history.size();
    _stats.avgTemperature = tempSum / count;
    _stats.avgHumidity = humiditySum / count;
    _stats.avgPressure = pressureSum / count;
    _stats.avgLightLevel = lightSum / count;
    
    // Motion statistics
    _stats.motionEvents = _motionEventCount;
    _stats.lastMotionTime = _lastMotionEvent;
    
    // Battery health (simplified calculation)
    _stats.batteryHealth = max(50.0,
