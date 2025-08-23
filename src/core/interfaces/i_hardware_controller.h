#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

/**
 * @file i_hardware_controller.h
 * @brief Hardware control interface for cross-platform hardware access
 * 
 * Provides unified API for hardware control across different platforms
 * including GPIO, sensors, system monitoring, and device control.
 * 
 * @performance Optimized for real-time hardware access
 * @thread_safety All methods are thread-safe unless explicitly noted
 * @memory_model RAII with automatic resource cleanup
 */

namespace cross_terminal {
namespace hardware {

/**
 * @brief GPIO pin operation modes
 */
enum class GPIOMode : uint8_t {
    Input = 0,        ///< Digital input (high impedance)
    Output = 1,       ///< Digital output (push-pull)
    InputPullUp = 2,  ///< Input with internal pull-up resistor
    InputPullDown = 3 ///< Input with internal pull-down resistor
};

/**
 * @brief Hardware sensor types
 */
enum class SensorType : uint8_t {
    Accelerometer = 0,  ///< 3-axis accelerometer (m/s²)
    Gyroscope = 1,      ///< 3-axis gyroscope (rad/s)
    Magnetometer = 2,   ///< 3-axis magnetometer (µT)
    Temperature = 3,    ///< Temperature sensor (°C)
    Humidity = 4,       ///< Humidity sensor (%RH)
    Pressure = 5,       ///< Barometric pressure (hPa)
    Light = 6,          ///< Ambient light sensor (lux)
    Proximity = 7,      ///< Proximity sensor (cm)
    GPS = 8,            ///< GPS location sensor
    Microphone = 9,     ///< Audio input level (dB)
    Camera = 10         ///< Camera sensor
};

/**
 * @brief Sensor data container
 * 
 * Standardized container for sensor readings with timestamp
 * and multi-dimensional data support.
 */
struct SensorData {
    SensorType type;              ///< Sensor type identifier
    std::vector<float> values;    ///< Sensor readings (axis-dependent)
    uint64_t timestamp;           ///< Timestamp in milliseconds since epoch
    float accuracy;               ///< Reading accuracy/confidence [0.0-1.0]
    
    /// @brief Default constructor
    SensorData() : type(SensorType::Temperature), timestamp(0), accuracy(1.0f) {}
    
    /// @brief Constructor with type
    explicit SensorData(SensorType t) : type(t), timestamp(0), accuracy(1.0f) {}
    
    /// @brief Check if data is valid
    bool isValid() const noexcept {
        return !values.empty() && timestamp > 0 && accuracy > 0.0f;
    }
    
    /// @brief Get single value for 1D sensors
    float getValue() const noexcept {
        return values.empty() ? 0.0f : values[0];
    }
    
    /// @brief Get 3D vector for multi-axis sensors
    struct Vec3 { float x, y, z; };
    Vec3 getVec3() const noexcept {
        if (values.size() >= 3) {
            return {values[0], values[1], values[2]};
        }
        return {0.0f, 0.0f, 0.0f};
    }
};

/**
 * @brief System performance metrics
 * 
 * Real-time system performance data for monitoring
 * and optimization purposes.
 */
struct SystemMetrics {
    float cpuUsage;        ///< CPU utilization percentage [0.0-100.0]
    float memoryUsage;     ///< Memory utilization percentage [0.0-100.0]
    float storageUsage;    ///< Storage utilization percentage [0.0-100.0]
    float temperature;     ///< System temperature in Celsius
    float batteryLevel;    ///< Battery charge percentage [0.0-100.0]
    bool isCharging;       ///< Battery charging status
    uint32_t uptime;       ///< System uptime in seconds
    
    /// @brief Default constructor
    SystemMetrics() 
        : cpuUsage(0.0f), memoryUsage(0.0f), storageUsage(0.0f)
        , temperature(25.0f), batteryLevel(100.0f), isCharging(false)
        , uptime(0) {}
    
    /// @brief Check if metrics are within normal ranges
    bool isHealthy() const noexcept {
        return cpuUsage < 90.0f && 
               memoryUsage < 85.0f && 
               temperature < 80.0f && 
               (batteryLevel > 10.0f || isCharging);
    }
};

/**
 * @brief Hardware controller interface
 * 
 * Abstract interface for platform-specific hardware control
 * implementations. Provides unified API for GPIO, sensors,
 * system monitoring, and device control.
 * 
 * @design_pattern Factory Method - Use create() for instantiation
 * @resource_management RAII with automatic cleanup
 * @performance Real-time optimized for hardware access
 */
class IHardwareController {
public:
    /// @brief Virtual destructor for proper cleanup
    virtual ~IHardwareController() = default;
    
    /**
     * @brief Factory method for platform-specific implementation
     * @return std::unique_ptr to hardware controller
     * @throws std::runtime_error if hardware access not available
     * @thread_safe Yes
     * @performance O(1)
     */
    static std::unique_ptr<IHardwareController> create();
    
    // GPIO Operations
    
    /**
     * @brief Check GPIO support availability
     * @return true if GPIO operations are supported
     * @thread_safe Yes
     * @performance O(1)
     * @noexcept
     */
    virtual bool isGPIOSupported() noexcept = 0;
    
    /**
     * @brief Configure GPIO pin mode
     * @param pin GPIO pin number (platform-specific)
     * @param mode Pin operation mode
     * @return true if configuration successful
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool configureGPIO(int pin, GPIOMode mode) = 0;
    
    /**
     * @brief Set GPIO output state
     * @param pin GPIO pin number
     * @param high true for high (VCC), false for low (GND)
     * @return true if operation successful
     * @thread_safe Yes
     * @performance O(1) - hardware optimized
     * @exception_safety Strong guarantee
     * @pre Pin must be configured as output
     */
    virtual bool writeGPIO(int pin, bool high) = 0;
    
    /**
     * @brief Read GPIO input state
     * @param pin GPIO pin number
     * @return true for high state, false for low
     * @thread_safe Yes
     * @performance O(1) - hardware optimized
     * @exception_safety Strong guarantee
     * @pre Pin must be configured as input
     */
    virtual bool readGPIO(int pin) = 0;
    
    // Sensor Access
    
    /**
     * @brief Get list of available sensors
     * @return Vector of supported sensor types
     * @thread_safe Yes
     * @performance O(n) where n is number of sensors
     * @exception_safety Strong guarantee
     */
    virtual std::vector<SensorType> getAvailableSensors() = 0;
    
    /**
     * @brief Enable sensor for data collection
     * @param type Sensor type to enable
     * @return true if sensor enabled successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool enableSensor(SensorType type) = 0;
    
    /**
     * @brief Disable sensor to save power
     * @param type Sensor type to disable
     * @return true if sensor disabled successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool disableSensor(SensorType type) = 0;
    
    /**
     * @brief Read current sensor data
     * @param type Sensor type to read
     * @return SensorData with current reading
     * @thread_safe Yes
     * @performance O(1) - cached/buffered reads
     * @exception_safety Strong guarantee
     */
    virtual SensorData readSensor(SensorType type) = 0;
    
    /**
     * @brief Set sensor sampling rate
     * @param type Sensor type
     * @param rate_hz Sampling rate in Hz
     * @return true if rate set successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setSensorRate(SensorType type, float rate_hz) = 0;
    
    // System Monitoring
    
    /**
     * @brief Get current system metrics
     * @return SystemMetrics structure with current values
     * @thread_safe Yes
     * @performance O(1) - cached with periodic updates
     * @exception_safety Strong guarantee
     */
    virtual SystemMetrics getSystemMetrics() = 0;
    
    /**
     * @brief Start continuous system monitoring
     * @param callback Function called with metrics updates
     * @param interval_ms Update interval in milliseconds
     * @return true if monitoring started successfully
     * @thread_safe Yes
     * @performance Dedicated monitoring thread
     * @exception_safety Strong guarantee
     */
    virtual bool startSystemMonitoring(
        std::function<void(const SystemMetrics&)> callback,
        uint32_t interval_ms = 1000) = 0;
    
    /**
     * @brief Stop system monitoring
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual void stopSystemMonitoring() noexcept = 0;
    
    // Device Control
    
    /**
     * @brief Set screen brightness
     * @param level Brightness level [0.0-1.0]
     * @return true if brightness set successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setScreenBrightness(float level) = 0;
    
    /**
     * @brief Get current screen brightness
     * @return Brightness level [0.0-1.0]
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual float getScreenBrightness() = 0;
    
    /**
     * @brief Control WiFi state
     * @param enable true to enable, false to disable
     * @return true if operation successful
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool enableWiFi(bool enable) = 0;
    
    /**
     * @brief Control Bluetooth state
     * @param enable true to enable, false to disable
     * @return true if operation successful
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool enableBluetooth(bool enable) = 0;
    
    // Audio Control
    
    /**
     * @brief Set system volume
     * @param level Volume level [0.0-1.0]
     * @return true if volume set successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setSystemVolume(float level) = 0;
    
    /**
     * @brief Get current system volume
     * @return Volume level [0.0-1.0]
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual float getSystemVolume() = 0;
    
    /**
     * @brief Play system beep/tone
     * @param frequency_hz Tone frequency in Hz
     * @param duration_ms Duration in milliseconds
     * @return true if beep played successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool playBeep(uint32_t frequency_hz, uint32_t duration_ms) = 0;
    
    // Power Management
    
    /**
     * @brief Set device power profile
     * @param profile Power profile name ("performance", "balanced", "power_save")
     * @return true if profile set successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setPowerProfile(const std::string& profile) = 0;
    
    /**
     * @brief Get current power profile
     * @return Power profile name
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getPowerProfile() = 0;
    
    /**
     * @brief Request device wake lock
     * @param timeout_ms Wake lock duration in milliseconds (0 = indefinite)
     * @return true if wake lock acquired
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool acquireWakeLock(uint32_t timeout_ms = 0) = 0;
    
    /**
     * @brief Release device wake lock
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual void releaseWakeLock() noexcept = 0;
    
protected:
    /// @brief Protected constructor - use factory method
    IHardwareController() = default;
    
    /// @brief Non-copyable
    IHardwareController(const IHardwareController&) = delete;
    IHardwareController& operator=(const IHardwareController&) = delete;
    
    /// @brief Non-movable
    IHardwareController(IHardwareController&&) = delete;
    IHardwareController& operator=(IHardwareController&&) = delete;
};

/**
 * @brief Hardware capability detection
 * 
 * Utility class for checking hardware capabilities at runtime
 * without instantiating full hardware controller.
 */
class HardwareCapabilities {
public:
    /**
     * @brief Check if GPIO is supported
     * @return true if GPIO operations available
     * @thread_safe Yes
     * @performance O(1)
     */
    static bool hasGPIO() noexcept;
    
    /**
     * @brief Check if specific sensor is available
     * @param type Sensor type to check
     * @return true if sensor is available
     * @thread_safe Yes
     * @performance O(1)
     */
    static bool hasSensor(SensorType type) noexcept;
    
    /**
     * @brief Check if device control features are available
     * @return true if brightness, WiFi, etc. can be controlled
     * @thread_safe Yes
     * @performance O(1)
     */
    static bool hasDeviceControl() noexcept;
    
    /**
     * @brief Get platform-specific hardware info
     * @return JSON string with hardware capabilities
     * @thread_safe Yes
     * @performance O(1)
     */
    static std::string getHardwareInfo();
};

/**
 * @brief RAII wrapper for sensor management
 * 
 * Automatically enables sensor on construction and
 * disables on destruction for exception-safe operation.
 */
class SensorGuard {
private:
    IHardwareController& controller_;
    SensorType sensor_type_;
    bool enabled_;
    
public:
    /**
     * @brief Constructor - enables sensor
     * @param controller Hardware controller reference
     * @param type Sensor type to manage
     */
    SensorGuard(IHardwareController& controller, SensorType type)
        : controller_(controller), sensor_type_(type) {
        enabled_ = controller_.enableSensor(sensor_type_);
    }
    
    /**
     * @brief Destructor - disables sensor
     */
    ~SensorGuard() {
        if (enabled_) {
            controller_.disableSensor(sensor_type_);
        }
    }
    
    /// @brief Non-copyable, non-movable
    SensorGuard(const SensorGuard&) = delete;
    SensorGuard& operator=(const SensorGuard&) = delete;
    SensorGuard(SensorGuard&&) = delete;
    SensorGuard& operator=(SensorGuard&&) = delete;
    
    /**
     * @brief Check if sensor was enabled successfully
     * @return true if sensor is active
     */
    bool isEnabled() const noexcept { return enabled_; }
    
    /**
     * @brief Read sensor data
     * @return Current sensor reading
     */
    SensorData read() { return controller_.readSensor(sensor_type_); }
};

} // namespace hardware
} // namespace cross_terminal