#include "android_hardware.h"
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#define LOG_TAG "AndroidHW"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// GPIO paths for common Android devices
static const char* GPIO_BASE_PATH = "/sys/class/gpio";
static const char* GPIO_EXPORT_PATH = "/sys/class/gpio/export";
static const char* GPIO_UNEXPORT_PATH = "/sys/class/gpio/unexport";

AndroidHardwareController::AndroidHardwareController() 
    : m_systemMonitoringActive(false) {
    LOGD("AndroidHardwareController initialized");
}

AndroidHardwareController::~AndroidHardwareController() {
    stopSystemMonitoring();
    LOGD("AndroidHardwareController destroyed");
}

bool AndroidHardwareController::isGPIOSupported() {
    // Check if GPIO sysfs interface is available
    struct stat st;
    return (stat(GPIO_BASE_PATH, &st) == 0 && S_ISDIR(st.st_mode));
}

bool AndroidHardwareController::configureGPIO(int pin, GPIOMode mode) {
    if (!isGPIOSupported()) {
        LOGE("GPIO not supported on this device");
        return false;
    }
    
    // Export GPIO pin
    std::ofstream exportFile(GPIO_EXPORT_PATH);
    if (!exportFile.is_open()) {
        LOGE("Failed to export GPIO pin %d", pin);
        return false;
    }
    exportFile << pin;
    exportFile.close();
    
    // Wait for GPIO to be available
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Set GPIO direction
    std::string directionPath = std::string(GPIO_BASE_PATH) + "/gpio" + std::to_string(pin) + "/direction";
    std::ofstream directionFile(directionPath);
    if (!directionFile.is_open()) {
        LOGE("Failed to set direction for GPIO pin %d", pin);
        return false;
    }
    
    switch (mode) {
        case GPIOMode::Input:
            directionFile << "in";
            break;
        case GPIOMode::Output:
            directionFile << "out";
            break;
        case GPIOMode::InputPullUp:
        case GPIOMode::InputPullDown:
            // Android GPIO sysfs doesn't directly support pull modes
            // This would require device-specific implementation
            directionFile << "in";
            LOGD("Pull mode not directly supported, configured as input");
            break;
    }
    directionFile.close();
    
    m_configuredPins[pin] = mode;
    return true;
}

bool AndroidHardwareController::writeGPIO(int pin, bool high) {
    if (m_configuredPins.find(pin) == m_configuredPins.end()) {
        LOGE("GPIO pin %d not configured", pin);
        return false;
    }
    
    if (m_configuredPins[pin] != GPIOMode::Output) {
        LOGE("GPIO pin %d not configured for output", pin);
        return false;
    }
    
    std::string valuePath = std::string(GPIO_BASE_PATH) + "/gpio" + std::to_string(pin) + "/value";
    std::ofstream valueFile(valuePath);
    if (!valueFile.is_open()) {
        LOGE("Failed to write to GPIO pin %d", pin);
        return false;
    }
    
    valueFile << (high ? "1" : "0");
    valueFile.close();
    return true;
}

bool AndroidHardwareController::readGPIO(int pin) {
    if (m_configuredPins.find(pin) == m_configuredPins.end()) {
        LOGE("GPIO pin %d not configured", pin);
        return false;
    }
    
    std::string valuePath = std::string(GPIO_BASE_PATH) + "/gpio" + std::to_string(pin) + "/value";
    std::ifstream valueFile(valuePath);
    if (!valueFile.is_open()) {
        LOGE("Failed to read from GPIO pin %d", pin);
        return false;
    }
    
    char value;
    valueFile >> value;
    valueFile.close();
    
    return (value == '1');
}

std::vector<SensorType> AndroidHardwareController::getAvailableSensors() {
    std::vector<SensorType> sensors;
    
    // Check for common Android sensor files
    const std::map<std::string, SensorType> sensorPaths = {
        {"/sys/class/sensors/accelerometer", SensorType::Accelerometer},
        {"/sys/class/sensors/gyroscope", SensorType::Gyroscope},
        {"/sys/class/sensors/magnetometer", SensorType::Magnetometer},
        {"/sys/class/hwmon/hwmon0/temp1_input", SensorType::Temperature},
        {"/sys/class/power_supply/battery/temp", SensorType::Temperature},
        {"/proc/sys/kernel/brightness", SensorType::Light}
    };
    
    struct stat st;
    for (const auto& [path, sensorType] : sensorPaths) {
        if (stat(path.c_str(), &st) == 0) {
            sensors.push_back(sensorType);
        }
    }
    
    return sensors;
}

bool AndroidHardwareController::enableSensor(SensorType type) {
    // In a real Android implementation, this would interface with
    // the Android sensor framework via JNI
    m_enabledSensors.insert(type);
    LOGD("Sensor %d enabled", static_cast<int>(type));
    return true;
}

bool AndroidHardwareController::disableSensor(SensorType type) {
    m_enabledSensors.erase(type);
    LOGD("Sensor %d disabled", static_cast<int>(type));
    return true;
}

SensorData AndroidHardwareController::readSensor(SensorType type) {
    SensorData data;
    data.type = type;
    data.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    switch (type) {
        case SensorType::Accelerometer:
            // Read from accelerometer sysfs or mock data
            data.values = {0.0f, 0.0f, 9.8f}; // Mock: gravity pointing down
            break;
        case SensorType::Gyroscope:
            data.values = {0.0f, 0.0f, 0.0f}; // Mock: no rotation
            break;
        case SensorType::Temperature:
            data.values = {readTemperature()};
            break;
        default:
            LOGE("Unsupported sensor type: %d", static_cast<int>(type));
            break;
    }
    
    return data;
}

SystemMetrics AndroidHardwareController::getSystemMetrics() {
    SystemMetrics metrics;
    
    // CPU usage
    metrics.cpuUsage = getCPUUsage();
    
    // Memory usage
    metrics.memoryUsage = getMemoryUsage();
    
    // Storage usage
    metrics.storageUsage = getStorageUsage();
    
    // Temperature
    metrics.temperature = readTemperature();
    
    // Battery info
    auto batteryInfo = getBatteryInfo();
    metrics.batteryLevel = batteryInfo.first;
    metrics.isCharging = batteryInfo.second;
    
    return metrics;
}

void AndroidHardwareController::startSystemMonitoring(std::function<void(const SystemMetrics&)> callback) {
    if (m_systemMonitoringActive) {
        return;
    }
    
    m_systemMonitoringActive = true;
    m_monitoringCallback = callback;
    
    m_monitoringThread = std::thread([this]() {
        while (m_systemMonitoringActive) {
            SystemMetrics metrics = getSystemMetrics();
            if (m_monitoringCallback) {
                m_monitoringCallback(metrics);
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void AndroidHardwareController::stopSystemMonitoring() {
    m_systemMonitoringActive = false;
    if (m_monitoringThread.joinable()) {
        m_monitoringThread.join();
    }
}

bool AndroidHardwareController::setScreenBrightness(float level) {
    if (level < 0.0f || level > 1.0f) {
        return false;
    }
    
    // Convert to Android brightness scale (0-255)
    int brightness = static_cast<int>(level * 255);
    
    std::ofstream brightnessFile("/sys/class/backlight/panel0-backlight/brightness");
    if (!brightnessFile.is_open()) {
        // Try alternative path
        brightnessFile.open("/sys/class/leds/lcd-backlight/brightness");
        if (!brightnessFile.is_open()) {
            LOGE("Failed to set screen brightness");
            return false;
        }
    }
    
    brightnessFile << brightness;
    brightnessFile.close();
    return true;
}

float AndroidHardwareController::getScreenBrightness() {
    std::ifstream brightnessFile("/sys/class/backlight/panel0-backlight/brightness");
    if (!brightnessFile.is_open()) {
        brightnessFile.open("/sys/class/leds/lcd-backlight/brightness");
        if (!brightnessFile.is_open()) {
            return 0.5f; // Default brightness
        }
    }
    
    int brightness;
    brightnessFile >> brightness;
    brightnessFile.close();
    
    return static_cast<float>(brightness) / 255.0f;
}

bool AndroidHardwareController::enableWiFi(bool enable) {
    // This would typically require system-level permissions
    // For now, we'll attempt to toggle via sysfs
    std::string command = enable ? "svc wifi enable" : "svc wifi disable";
    return system(command.c_str()) == 0;
}

bool AndroidHardwareController::enableBluetooth(bool enable) {
    // Similar to WiFi, this requires system permissions
    std::string command = enable ? "svc bluetooth enable" : "svc bluetooth disable";
    return system(command.c_str()) == 0;
}

bool AndroidHardwareController::setSystemVolume(float level) {
    if (level < 0.0f || level > 1.0f) {
        return false;
    }
    
    // Convert to Android volume scale (typically 0-15 for media)
    int volume = static_cast<int>(level * 15);
    std::string command = "media volume --stream 3 --set " + std::to_string(volume);
    return system(command.c_str()) == 0;
}

float AndroidHardwareController::getSystemVolume() {
    // This would require parsing the output of volume commands
    // For now, return a default value
    return 0.5f;
}

bool AndroidHardwareController::playBeep(int frequency, int duration) {
    // Generate a simple beep using the speaker
    // This is a simplified implementation
    std::string command = "echo -e '\\a'";
    return system(command.c_str()) == 0;
}

float AndroidHardwareController::getCPUUsage() {
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        return 0.0f;
    }
    
    std::string line;
    std::getline(statFile, line);
    statFile.close();
    
    std::istringstream iss(line);
    std::string cpu;
    long user, nice, system, idle;
    iss >> cpu >> user >> nice >> system >> idle;
    
    static long lastTotal = 0, lastIdle = 0;
    long total = user + nice + system + idle;
    long totalDiff = total - lastTotal;
    long idleDiff = idle - lastIdle;
    
    float usage = 0.0f;
    if (totalDiff > 0) {
        usage = 100.0f * (totalDiff - idleDiff) / totalDiff;
    }
    
    lastTotal = total;
    lastIdle = idle;
    
    return usage;
}

float AndroidHardwareController::getMemoryUsage() {
    std::ifstream meminfoFile("/proc/meminfo");
    if (!meminfoFile.is_open()) {
        return 0.0f;
    }
    
    std::string line;
    long totalMem = 0, freeMem = 0, buffers = 0, cached = 0;
    
    while (std::getline(meminfoFile, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;
        iss >> key >> value;
        
        if (key == "MemTotal:") totalMem = value;
        else if (key == "MemFree:") freeMem = value;
        else if (key == "Buffers:") buffers = value;
        else if (key == "Cached:") cached = value;
    }
    meminfoFile.close();
    
    if (totalMem > 0) {
        long usedMem = totalMem - freeMem - buffers - cached;
        return 100.0f * usedMem / totalMem;
    }
    
    return 0.0f;
}

float AndroidHardwareController::getStorageUsage() {
    // Get storage usage for root partition
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        uint64_t total = stat.f_blocks * stat.f_frsize;
        uint64_t free = stat.f_bavail * stat.f_frsize;
        uint64_t used = total - free;
        return 100.0f * used / total;
    }
    return 0.0f;
}

float AndroidHardwareController::readTemperature() {
    // Try reading from various temperature sensors
    const std::vector<std::string> tempPaths = {
        "/sys/class/hwmon/hwmon0/temp1_input",
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/power_supply/battery/temp"
    };
    
    for (const auto& path : tempPaths) {
        std::ifstream tempFile(path);
        if (tempFile.is_open()) {
            int temp;
            tempFile >> temp;
            tempFile.close();
            // Convert from millidegrees to degrees Celsius
            return temp / 1000.0f;
        }
    }
    
    return 25.0f; // Default room temperature
}

std::pair<float, bool> AndroidHardwareController::getBatteryInfo() {
    float level = 50.0f; // Default
    bool charging = false;
    
    // Read battery level
    std::ifstream levelFile("/sys/class/power_supply/battery/capacity");
    if (levelFile.is_open()) {
        int capacity;
        levelFile >> capacity;
        levelFile.close();
        level = static_cast<float>(capacity);
    }
    
    // Read charging status
    std::ifstream statusFile("/sys/class/power_supply/battery/status");
    if (statusFile.is_open()) {
        std::string status;
        statusFile >> status;
        statusFile.close();
        charging = (status == "Charging" || status == "Full");
    }
    
    return {level, charging};
}