#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

enum class GPIOMode {
    Input,
    Output,
    InputPullUp,
    InputPullDown
};

enum class SensorType {
    Accelerometer,
    Gyroscope,
    Magnetometer,
    Temperature,
    Humidity,
    Pressure,
    Light,
    Proximity
};

struct SensorData {
    SensorType type;
    std::vector<float> values;
    uint64_t timestamp;
};

struct SystemMetrics {
    float cpuUsage;
    float memoryUsage;
    float storageUsage;
    float temperature;
    float batteryLevel;
    bool isCharging;
};

class HardwareController {
public:
    virtual ~HardwareController() = default;
    
    static std::unique_ptr<HardwareController> create();
    
    // GPIO operations (mainly for embedded/Android devices)
    virtual bool isGPIOSupported() = 0;
    virtual bool configureGPIO(int pin, GPIOMode mode) = 0;
    virtual bool writeGPIO(int pin, bool high) = 0;
    virtual bool readGPIO(int pin) = 0;
    
    // Sensor access
    virtual std::vector<SensorType> getAvailableSensors() = 0;
    virtual bool enableSensor(SensorType type) = 0;
    virtual bool disableSensor(SensorType type) = 0;
    virtual SensorData readSensor(SensorType type) = 0;
    
    // System monitoring
    virtual SystemMetrics getSystemMetrics() = 0;
    virtual void startSystemMonitoring(std::function<void(const SystemMetrics&)> callback) = 0;
    virtual void stopSystemMonitoring() = 0;
    
    // Device control
    virtual bool setScreenBrightness(float level) = 0;
    virtual float getScreenBrightness() = 0;
    virtual bool enableWiFi(bool enable) = 0;
    virtual bool enableBluetooth(bool enable) = 0;
    
    // Audio control
    virtual bool setSystemVolume(float level) = 0;
    virtual float getSystemVolume() = 0;
    virtual bool playBeep(int frequency, int duration) = 0;
    
protected:
    HardwareController() = default;
};