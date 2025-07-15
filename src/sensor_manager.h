 #ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "config.h"

// ================================
// SENSOR DATA STRUCTURES
// ================================

struct SensorReading {
    float temperature;
    float humidity;
    float pressure;
    float lightLevel;
    bool motionDetected;
    float batteryLevel;
    unsigned long timestamp;
};

struct SensorStats {
    float minTemperature;
    float maxTemperature;
    float avgTemperature;
    float minHumidity;
    float maxHumidity;
    float avgHumidity;
    float minPressure;
    float maxPressure;
    float avgPressure;
    float minLightLevel;
    float maxLightLevel;
    float avgLightLevel;
    int motionEvents;
    unsigned long lastMotionTime;
    float batteryHealth;
    unsigned long dataPoints;
};

struct DeviceStats {
    unsigned long uptime;
    uint32_t bootCount;
    uint32_t totalConnections;
    size_t freeHeap;
    size_t totalHeap;
    float cpuUsage;
    String wifiSSID;
    int wifiRSSI;
    IPAddress localIP;
    String macAddress;
    float temperature;
    bool ledState;
    int webSocketClients;
};

// ================================
// SENSOR MANAGER CLASS
// ================================

class SensorManager {
public:
    // Constructor
    SensorManager();
    
    // Initialization
    void begin();
    void end();
    
    // Main update loop
    void update();
    
    // Sensor Control
    void enableSensor(const String& sensorName, bool enabled);
    bool isSensorEnabled(const String& sensorName);
    void setUpdateInterval(unsigned long interval);
    unsigned long getUpdateInterval();
    
    // Data Access
    SensorReading getCurrentReading();
    std::vector<SensorReading> getHistory();
    SensorStats getStatistics();
    DeviceStats getDeviceStatistics();
    
    // JSON Output
    String getSensorDataJSON();
    String getSensorHistoryJSON();
    String getSensorStatsJSON();
    String getDeviceStatsJSON();
    String getAllDataJSON();
    
    // Data Management
    void clearHistory();
    void resetStatistics();
    int getHistorySize();
    void setHistorySize(int size);
    
    // Calibration (for real sensors)
    void calibrateTemperature(float offset);
    void calibrateHumidity(float offset);
    void calibratePressure(float offset);
    
    // Battery Management
    void setBatteryLevel(float level);
    float getBatteryLevel();
    bool isBatteryLow();
    
    // Motion Detection
    bool isMotionDetected();
    unsigned long getLastMotionTime();
    int getMotionEventCount();
    
    // Device Statistics Callbacks
    void setUptimeCallback(std::function<unsigned long()> callback);
    void setBootCountCallback(std::function<uint32_t()> callback);
    void setTotalConnectionsCallback(std::function<uint32_t()> callback);
    void setWiFiInfoCallback(std::function<String()> ssidCallback, std::function<int()> rssiCallback);
    void setLEDStateCallback(std::function<bool()> callback);
    void setWebSocketClientsCallback(std::function<int()> callback);

private:
    // Current sensor reading
    SensorReading _currentReading;
    
    // Historical data
    std::vector<SensorReading> _history;
    int _maxHistorySize;
    
    // Statistics
    SensorStats _stats;
    bool _statsValid;
    
    // Sensor states
    bool _temperatureEnabled;
    bool _humidityEnabled;
    bool _pressureEnabled;
    bool _lightEnabled;
    bool _motionEnabled;
    bool _batteryEnabled;
    
    // Timing
    unsigned long _lastUpdate;
    unsigned long _updateInterval;
    unsigned long _lastStatsUpdate;
    
    // Simulation parameters
    float _tempBase;
    float _tempTrend;
    float _humidityBase;
    float _humidityTrend;
    float _pressureBase;
    float _pressureTrend;
    float _lightBase;
    float _lightTrend;
    
    // Motion simulation
    bool _motionActive;
    unsigned long _motionStartTime;
    unsigned long _lastMotionEvent;
    int _motionEventCount;
    
    // Battery simulation
    float _batteryLevel;
    bool _batteryCharging;
    unsigned long _lastBatteryUpdate;
    
    // Calibration offsets
    float _tempOffset;
    float _humidityOffset;
    float _pressureOffset;
    
    // Device statistics callbacks
    std::function<unsigned long()> _uptimeCallback;
    std::function<uint32_t()> _bootCountCallback;
    std::function<uint32_t()> _totalConnectionsCallback;
    std::function<String()> _wifiSSIDCallback;
    std::function<int()> _wifiRSSICallback;
    std::function<bool()> _ledStateCallback;
    std::function<int()> _webSocketClientsCallback;
    
    // Private methods
    void _updateSensors();
    void _updateTemperature();
    void _updateHumidity();
    void _updatePressure();
    void _updateLightLevel();
    void _updateMotionDetection();
    void _updateBatteryLevel();
    void _addToHistory(const SensorReading& reading);
    void _updateStatistics();
    void _calculateStatistics();
    float _generateSensorValue(float base, float variation, float& trend);
    float _applyNoise(float value, float noiseLevel);
    bool _shouldTriggerMotion();
    void _simulateBatteryDrain();
    String _formatTimestamp(unsigned long timestamp);
    String _boolToString(bool value);
};

// ================================
// SENSOR CONFIGURATION
// ================================

enum class SensorType {
    TEMPERATURE,
    HUMIDITY,
    PRESSURE,
    LIGHT,
    MOTION,
    BATTERY
};

struct SensorConfig {
    SensorType type;
    bool enabled;
    float minValue;
    float maxValue;
    float currentValue;
    String unit;
    String name;
};

#endif // SENSOR_MANAGER_H

