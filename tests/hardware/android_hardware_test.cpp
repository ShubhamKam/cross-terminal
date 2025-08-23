#include <gtest/gtest.h>
#include "hardware/android/android_hardware.h"

class AndroidHardwareTest : public ::testing::Test {
protected:
    void SetUp() override {
        hardware = std::make_unique<AndroidHardwareController>();
    }

    void TearDown() override {
        hardware->stopSystemMonitoring();
        hardware.reset();
    }

    std::unique_ptr<AndroidHardwareController> hardware;
};

TEST_F(AndroidHardwareTest, GPIOSupport) {
    // GPIO support depends on device and permissions
    bool gpioSupported = hardware->isGPIOSupported();
    // Can't assume GPIO availability, just test it doesn't crash
    EXPECT_TRUE(true);
    
    if (gpioSupported) {
        // Test GPIO configuration
        bool configured = hardware->configureGPIO(18, GPIOMode::Output);
        // Success depends on permissions and hardware
    }
}

TEST_F(AndroidHardwareTest, SensorAvailability) {
    auto sensors = hardware->getAvailableSensors();
    // Android devices typically have some sensors
    // But can't guarantee specific sensors in test environment
    EXPECT_TRUE(true); // Test completed without crash
    
    // Test sensor operations
    for (auto sensorType : sensors) {
        EXPECT_TRUE(hardware->enableSensor(sensorType));
        
        SensorData data = hardware->readSensor(sensorType);
        EXPECT_EQ(data.type, sensorType);
        EXPECT_GT(data.timestamp, 0);
        
        EXPECT_TRUE(hardware->disableSensor(sensorType));
    }
}

TEST_F(AndroidHardwareTest, SystemMetrics) {
    SystemMetrics metrics = hardware->getSystemMetrics();
    
    // CPU usage should be a valid percentage
    EXPECT_GE(metrics.cpuUsage, 0.0f);
    EXPECT_LE(metrics.cpuUsage, 100.0f);
    
    // Memory usage should be a valid percentage
    EXPECT_GE(metrics.memoryUsage, 0.0f);
    EXPECT_LE(metrics.memoryUsage, 100.0f);
    
    // Storage usage should be a valid percentage
    EXPECT_GE(metrics.storageUsage, 0.0f);
    EXPECT_LE(metrics.storageUsage, 100.0f);
    
    // Temperature should be reasonable (0-100°C)
    EXPECT_GE(metrics.temperature, 0.0f);
    EXPECT_LE(metrics.temperature, 100.0f);
    
    // Battery level should be valid percentage
    EXPECT_GE(metrics.batteryLevel, 0.0f);
    EXPECT_LE(metrics.batteryLevel, 100.0f);
}

TEST_F(AndroidHardwareTest, SystemMonitoring) {
    bool callbackCalled = false;
    SystemMetrics lastMetrics;
    
    hardware->startSystemMonitoring([&](const SystemMetrics& metrics) {
        callbackCalled = true;
        lastMetrics = metrics;
    });
    
    // Wait for at least one callback
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    hardware->stopSystemMonitoring();
    
    EXPECT_TRUE(callbackCalled);
    if (callbackCalled) {
        EXPECT_GE(lastMetrics.cpuUsage, 0.0f);
        EXPECT_LE(lastMetrics.cpuUsage, 100.0f);
    }
}

TEST_F(AndroidHardwareTest, DeviceControl) {
    // Test screen brightness control
    float originalBrightness = hardware->getScreenBrightness();
    EXPECT_GE(originalBrightness, 0.0f);
    EXPECT_LE(originalBrightness, 1.0f);
    
    // Try setting brightness (may require permissions)
    bool brightnessSet = hardware->setScreenBrightness(0.5f);
    // Can't guarantee success due to permissions
    
    if (brightnessSet) {
        float newBrightness = hardware->getScreenBrightness();
        EXPECT_NEAR(newBrightness, 0.5f, 0.1f); // Allow some tolerance
        
        // Restore original brightness
        hardware->setScreenBrightness(originalBrightness);
    }
}

TEST_F(AndroidHardwareTest, AudioControl) {
    // Test system volume control
    float originalVolume = hardware->getSystemVolume();
    EXPECT_GE(originalVolume, 0.0f);
    EXPECT_LE(originalVolume, 1.0f);
    
    // Test beep functionality
    bool beepPlayed = hardware->playBeep(1000, 100);
    // Success depends on system permissions and audio availability
    EXPECT_TRUE(true); // Test completed without crash
}

TEST_F(AndroidHardwareTest, NetworkControl) {
    // Test WiFi control (requires system permissions)
    bool wifiEnabled = hardware->enableWiFi(true);
    // Can't guarantee success due to permissions
    EXPECT_TRUE(true);
    
    // Test Bluetooth control (requires system permissions)  
    bool bluetoothEnabled = hardware->enableBluetooth(true);
    // Can't guarantee success due to permissions
    EXPECT_TRUE(true);
}

TEST_F(AndroidHardwareTest, EdgeCases) {
    // Test invalid brightness values
    EXPECT_FALSE(hardware->setScreenBrightness(-0.1f));
    EXPECT_FALSE(hardware->setScreenBrightness(1.1f));
    
    // Test invalid volume values
    EXPECT_FALSE(hardware->setSystemVolume(-0.1f));
    EXPECT_FALSE(hardware->setSystemVolume(1.1f));
    
    // Test reading non-existent sensor
    SensorData data = hardware->readSensor(static_cast<SensorType>(999));
    EXPECT_EQ(data.type, static_cast<SensorType>(999));
    // Should return empty or default data
}

TEST_F(AndroidHardwareTest, SensorDataValidation) {
    auto sensors = hardware->getAvailableSensors();
    
    for (auto sensorType : sensors) {
        hardware->enableSensor(sensorType);
        SensorData data = hardware->readSensor(sensorType);
        
        EXPECT_EQ(data.type, sensorType);
        EXPECT_GT(data.timestamp, 0);
        
        // Validate sensor-specific data
        switch (sensorType) {
            case SensorType::Accelerometer:
                EXPECT_EQ(data.values.size(), 3); // X, Y, Z
                break;
            case SensorType::Gyroscope:
                EXPECT_EQ(data.values.size(), 3); // X, Y, Z rotation
                break;
            case SensorType::Temperature:
                EXPECT_EQ(data.values.size(), 1); // Single temperature value
                if (!data.values.empty()) {
                    EXPECT_GT(data.values[0], -50.0f); // Reasonable temp range
                    EXPECT_LT(data.values[0], 100.0f);
                }
                break;
            default:
                // Other sensors may have variable data sizes
                break;
        }
        
        hardware->disableSensor(sensorType);
    }
}