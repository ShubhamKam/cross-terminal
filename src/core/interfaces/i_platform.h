#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

/**
 * @file i_platform.h
 * @brief Platform abstraction interface for cross-platform terminal functionality
 * 
 * This interface provides a unified API for platform-specific operations across
 * Android, iOS, macOS, Windows, and Linux platforms.
 * 
 * @performance All operations are designed to be O(1) unless otherwise specified
 * @thread_safety All methods are thread-safe unless explicitly marked otherwise
 * @memory_model Uses RAII and smart pointers for automatic resource management
 */

namespace cross_terminal {
namespace core {

/**
 * @brief System information structure
 * 
 * Contains platform-specific system information that can be queried
 * in a cross-platform manner.
 */
struct SystemInfo {
    std::string osName;          ///< Operating system name (e.g., "Android", "macOS")
    std::string osVersion;       ///< OS version string
    std::string architecture;    ///< CPU architecture (e.g., "arm64", "x86_64")
    int cpuCores;               ///< Number of CPU cores
    uint64_t totalMemory;       ///< Total system memory in bytes
    uint64_t availableMemory;   ///< Available memory in bytes
    
    /// @brief Default constructor
    SystemInfo() = default;
    
    /// @brief Copy constructor
    SystemInfo(const SystemInfo&) = default;
    
    /// @brief Move constructor
    SystemInfo(SystemInfo&&) noexcept = default;
    
    /// @brief Copy assignment
    SystemInfo& operator=(const SystemInfo&) = default;
    
    /// @brief Move assignment
    SystemInfo& operator=(SystemInfo&&) noexcept = default;
};

/**
 * @brief Platform type enumeration
 * 
 * Identifies the current platform type for conditional behavior.
 */
enum class PlatformType : uint8_t {
    Android = 0,
    iOS = 1,
    macOS = 2,
    Windows = 3,
    Linux = 4,
    Unknown = 255
};

/**
 * @brief Abstract platform interface
 * 
 * This interface defines the contract for platform-specific operations.
 * Each platform implementation must provide concrete implementations
 * of all pure virtual methods.
 * 
 * @design_pattern Factory Method - Use Platform::create() to instantiate
 * @memory_safety All returned strings use std::string for automatic memory management
 * @exception_safety All methods are noexcept or provide strong exception guarantee
 */
class IPlatform {
public:
    /// @brief Virtual destructor for proper cleanup of derived classes
    virtual ~IPlatform() = default;
    
    /**
     * @brief Factory method to create platform-specific implementation
     * @return std::unique_ptr to platform implementation
     * @throws std::runtime_error if platform is not supported
     * @thread_safe Yes
     * @performance O(1)
     */
    static std::unique_ptr<IPlatform> create();
    
    /**
     * @brief Get current platform type
     * @return PlatformType enumeration value
     * @thread_safe Yes
     * @performance O(1)
     * @noexcept
     */
    static PlatformType getCurrentPlatform() noexcept;
    
    // System Information
    
    /**
     * @brief Retrieve comprehensive system information
     * @return SystemInfo structure with platform details
     * @thread_safe Yes
     * @performance O(1) - cached after first call
     * @exception_safety Strong guarantee
     */
    virtual SystemInfo getSystemInfo() = 0;
    
    /**
     * @brief Get device model string
     * @return Device model identifier (e.g., "iPhone13,4", "MacBookPro18,1")
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getDeviceModel() = 0;
    
    // File System Operations
    
    /**
     * @brief Check if file or directory exists
     * @param path Absolute or relative path to check
     * @return true if path exists, false otherwise
     * @thread_safe Yes
     * @performance O(1) - single system call
     * @exception_safety No-throw guarantee
     */
    virtual bool fileExists(const std::string& path) noexcept = 0;
    
    /**
     * @brief Create directory with intermediate directories
     * @param path Directory path to create
     * @return true if created successfully or already exists
     * @thread_safe Yes
     * @performance O(n) where n is path depth
     * @exception_safety Strong guarantee
     */
    virtual bool createDirectory(const std::string& path) = 0;
    
    /**
     * @brief List directory contents
     * @param path Directory path to list
     * @return Vector of file/directory names (not full paths)
     * @thread_safe Yes
     * @performance O(n) where n is number of entries
     * @exception_safety Strong guarantee
     */
    virtual std::vector<std::string> listDirectory(const std::string& path) = 0;
    
    /**
     * @brief Get current working directory
     * @return Absolute path of current working directory
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getCurrentDirectory() = 0;
    
    /**
     * @brief Change current working directory
     * @param path New working directory path
     * @return true if successful, false otherwise
     * @thread_safe No - affects process-global state
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setCurrentDirectory(const std::string& path) = 0;
    
    // Process Management
    
    /**
     * @brief Execute shell command and capture output
     * @param command Command string to execute
     * @param output Reference to string that will receive command output
     * @return Exit code of executed command
     * @thread_safe Yes
     * @performance O(n) where n is command execution time
     * @exception_safety Strong guarantee
     */
    virtual int executeCommand(const std::string& command, std::string& output) = 0;
    
    /**
     * @brief Terminate process by PID
     * @param pid Process ID to terminate
     * @return true if termination signal sent successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual bool killProcess(int pid) noexcept = 0;
    
    /**
     * @brief Get list of running process IDs
     * @return Vector of process IDs
     * @thread_safe Yes
     * @performance O(n) where n is number of processes
     * @exception_safety Strong guarantee
     */
    virtual std::vector<int> getRunningProcesses() = 0;
    
    // Hardware Access
    
    /**
     * @brief Check if hardware access is available
     * @return true if hardware control features can be used
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual bool hasHardwareAccess() noexcept = 0;
    
    /**
     * @brief Request hardware access permissions
     * @return true if permissions granted or already available
     * @thread_safe Yes
     * @performance O(1) - may trigger user prompt
     * @exception_safety Strong guarantee
     */
    virtual bool requestHardwarePermissions() = 0;
    
    // Network Operations
    
    /**
     * @brief Check network connectivity
     * @return true if network is available
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual bool hasNetworkAccess() noexcept = 0;
    
    /**
     * @brief Get primary IP address
     * @return IP address string, empty if unavailable
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getIPAddress() = 0;
    
    /**
     * @brief Get list of network interface names
     * @return Vector of interface names (e.g., "eth0", "wlan0")
     * @thread_safe Yes
     * @performance O(n) where n is number of interfaces
     * @exception_safety Strong guarantee
     */
    virtual std::vector<std::string> getNetworkInterfaces() = 0;
    
protected:
    /// @brief Protected constructor - use factory method
    IPlatform() = default;
    
    /// @brief Non-copyable
    IPlatform(const IPlatform&) = delete;
    IPlatform& operator=(const IPlatform&) = delete;
    
    /// @brief Non-movable  
    IPlatform(IPlatform&&) = delete;
    IPlatform& operator=(IPlatform&&) = delete;
};

/**
 * @brief Platform capability flags
 * 
 * Bitfield enumeration for checking platform-specific capabilities
 * at compile time or runtime.
 */
enum class PlatformCapability : uint32_t {
    None = 0,
    HardwareControl = 1 << 0,    ///< GPIO, sensors, device control
    NetworkControl = 1 << 1,     ///< WiFi, Bluetooth management
    SystemMonitoring = 1 << 2,   ///< CPU, memory, performance metrics
    FileSystemAccess = 1 << 3,   ///< Full filesystem read/write
    ProcessControl = 1 << 4,     ///< Process creation/termination
    AudioControl = 1 << 5,       ///< Audio playback/recording
    DisplayControl = 1 << 6,     ///< Brightness, display settings
    PowerManagement = 1 << 7,    ///< Battery, power profiles
    
    // Platform-specific combinations
    AndroidCapabilities = HardwareControl | NetworkControl | SystemMonitoring | 
                         AudioControl | DisplayControl | PowerManagement,
    macOSCapabilities = NetworkControl | SystemMonitoring | FileSystemAccess | 
                       ProcessControl | AudioControl | DisplayControl,
    LinuxCapabilities = HardwareControl | NetworkControl | SystemMonitoring | 
                       FileSystemAccess | ProcessControl | AudioControl,
    
    All = 0xFFFFFFFF
};

/**
 * @brief Bitwise OR operator for PlatformCapability
 */
constexpr PlatformCapability operator|(PlatformCapability lhs, PlatformCapability rhs) noexcept {
    return static_cast<PlatformCapability>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
    );
}

/**
 * @brief Bitwise AND operator for PlatformCapability
 */
constexpr PlatformCapability operator&(PlatformCapability lhs, PlatformCapability rhs) noexcept {
    return static_cast<PlatformCapability>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
    );
}

/**
 * @brief Check if platform has specific capability
 * @param capabilities Platform capability flags
 * @param capability Capability to check
 * @return true if capability is available
 */
constexpr bool hasCapability(PlatformCapability capabilities, PlatformCapability capability) noexcept {
    return (capabilities & capability) == capability;
}

} // namespace core
} // namespace cross_terminal