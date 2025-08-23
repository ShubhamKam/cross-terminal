#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include "memory/memory_manager.h"

/**
 * @file i_shell.h
 * @brief Shell execution interface for terminal emulator
 * 
 * Provides unified API for shell execution across different platforms
 * with support for process management, I/O redirection, and job control.
 * 
 * @performance Optimized for low-latency command execution
 * @thread_safety All methods are thread-safe unless explicitly noted
 * @memory_model Uses memory pools for frequent allocations
 */

namespace cross_terminal {
namespace core {

/**
 * @brief Process execution state
 */
enum class ProcessState : uint8_t {
    NotStarted = 0,   ///< Process created but not started
    Running = 1,      ///< Process is currently running
    Completed = 2,    ///< Process finished successfully
    Failed = 3,       ///< Process terminated with error
    Terminated = 4,   ///< Process killed by signal
    Suspended = 5     ///< Process suspended (job control)
};

/**
 * @brief Process information structure
 */
struct ProcessInfo {
    int pid;                    ///< Process ID
    int parent_pid;            ///< Parent process ID
    ProcessState state;        ///< Current execution state
    int exit_code;             ///< Exit code (valid when state is Completed/Failed)
    uint64_t start_time;       ///< Start time in milliseconds since epoch
    uint64_t end_time;         ///< End time in milliseconds since epoch
    std::string command;       ///< Original command string
    std::vector<std::string> arguments; ///< Command arguments
    std::string working_dir;   ///< Working directory
    
    /// @brief Default constructor
    ProcessInfo() 
        : pid(0), parent_pid(0), state(ProcessState::NotStarted)
        , exit_code(0), start_time(0), end_time(0) {}
    
    /// @brief Check if process is active
    bool isActive() const noexcept {
        return state == ProcessState::Running || state == ProcessState::Suspended;
    }
    
    /// @brief Get execution duration in milliseconds
    uint64_t getDuration() const noexcept {
        if (start_time == 0) return 0;
        uint64_t end = (end_time > 0) ? end_time : getCurrentTime();
        return end - start_time;
    }
    
private:
    static uint64_t getCurrentTime() noexcept;
};

/**
 * @brief Shell environment variable container
 */
class Environment {
private:
    std::unordered_map<std::string, std::string> variables_;
    mutable std::shared_mutex mutex_;
    
public:
    /// @brief Set environment variable
    void set(const std::string& name, const std::string& value);
    
    /// @brief Get environment variable
    std::string get(const std::string& name) const;
    
    /// @brief Check if variable exists
    bool has(const std::string& name) const noexcept;
    
    /// @brief Remove environment variable
    bool remove(const std::string& name);
    
    /// @brief Get all environment variables
    std::vector<std::pair<std::string, std::string>> getAll() const;
    
    /// @brief Clear all variables
    void clear();
    
    /// @brief Export to system environment
    void exportToSystem() const;
    
    /// @brief Import from system environment
    void importFromSystem();
};

/**
 * @brief Command execution options
 */
struct ExecutionOptions {
    std::string working_directory;     ///< Working directory for execution
    Environment environment;           ///< Environment variables
    bool capture_output = true;       ///< Capture stdout/stderr
    bool merge_stderr = false;        ///< Merge stderr with stdout
    uint32_t timeout_ms = 0;          ///< Execution timeout (0 = no timeout)
    bool run_in_background = false;   ///< Run as background job
    int priority = 0;                 ///< Process priority (-20 to 19)
    
    /// @brief Default constructor with sensible defaults
    ExecutionOptions() = default;
    
    /// @brief Builder pattern for options
    ExecutionOptions& setWorkingDirectory(const std::string& dir) {
        working_directory = dir;
        return *this;
    }
    
    ExecutionOptions& setTimeout(uint32_t ms) {
        timeout_ms = ms;
        return *this;
    }
    
    ExecutionOptions& setBackground(bool bg) {
        run_in_background = bg;
        return *this;
    }
    
    ExecutionOptions& setPriority(int prio) {
        priority = std::clamp(prio, -20, 19);
        return *this;
    }
};

/**
 * @brief Abstract shell interface
 * 
 * Defines the contract for shell implementations across different
 * platforms. Provides process execution, job control, and I/O management.
 * 
 * @design_pattern Factory Method - Use Shell::create() for instantiation
 * @resource_management RAII with automatic process cleanup
 * @performance Memory pooling for frequent allocations
 */
class IShell {
public:
    /// @brief Process output callback function
    using OutputCallback = std::function<void(const std::string& output, bool is_error)>;
    
    /// @brief Process completion callback function
    using CompletionCallback = std::function<void(const ProcessInfo& info)>;
    
    /// @brief Virtual destructor for proper cleanup
    virtual ~IShell() = default;
    
    /**
     * @brief Factory method for platform-specific shell
     * @return std::unique_ptr to shell implementation
     * @throws std::runtime_error if shell creation fails
     * @thread_safe Yes
     * @performance O(1)
     */
    static std::unique_ptr<IShell> create();
    
    /**
     * @brief Initialize shell with default settings
     * @return true if initialization successful
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Cleanup and terminate all processes
     * @thread_safe Yes
     * @performance O(n) where n is number of active processes
     * @exception_safety No-throw guarantee
     */
    virtual void shutdown() noexcept = 0;
    
    // Process Execution
    
    /**
     * @brief Execute command synchronously
     * @param command Command string to execute
     * @param options Execution options
     * @return ProcessInfo with execution results
     * @thread_safe Yes
     * @performance O(1) for command parsing, O(n) for execution
     * @exception_safety Strong guarantee
     */
    virtual ProcessInfo executeSync(const std::string& command, 
                                   const ExecutionOptions& options = {}) = 0;
    
    /**
     * @brief Execute command asynchronously
     * @param command Command string to execute
     * @param options Execution options
     * @param output_callback Callback for output data
     * @param completion_callback Callback for process completion
     * @return Process ID, or -1 if execution failed
     * @thread_safe Yes
     * @performance O(1) for command parsing
     * @exception_safety Strong guarantee
     */
    virtual int executeAsync(const std::string& command,
                            const ExecutionOptions& options,
                            OutputCallback output_callback,
                            CompletionCallback completion_callback) = 0;
    
    /**
     * @brief Execute command with real-time I/O
     * @param command Command string to execute
     * @param options Execution options
     * @return Process ID for I/O operations
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual int executeInteractive(const std::string& command,
                                  const ExecutionOptions& options = {}) = 0;
    
    // Process Management
    
    /**
     * @brief Get information about running process
     * @param pid Process ID
     * @return ProcessInfo structure
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual ProcessInfo getProcessInfo(int pid) = 0;
    
    /**
     * @brief Get list of all managed processes
     * @return Vector of ProcessInfo structures
     * @thread_safe Yes
     * @performance O(n) where n is number of processes
     * @exception_safety Strong guarantee
     */
    virtual std::vector<ProcessInfo> getAllProcesses() = 0;
    
    /**
     * @brief Terminate process by ID
     * @param pid Process ID to terminate
     * @param force true for SIGKILL, false for SIGTERM
     * @return true if termination signal sent successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual bool terminateProcess(int pid, bool force = false) noexcept = 0;
    
    /**
     * @brief Suspend process (job control)
     * @param pid Process ID to suspend
     * @return true if suspended successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool suspendProcess(int pid) = 0;
    
    /**
     * @brief Resume suspended process
     * @param pid Process ID to resume
     * @return true if resumed successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool resumeProcess(int pid) = 0;
    
    // I/O Operations
    
    /**
     * @brief Send input to interactive process
     * @param pid Process ID
     * @param input Input string to send
     * @return true if input sent successfully
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool sendInput(int pid, const std::string& input) = 0;
    
    /**
     * @brief Read available output from process
     * @param pid Process ID
     * @param max_bytes Maximum bytes to read (0 = all available)
     * @return Output string
     * @thread_safe Yes
     * @performance O(1) for buffered reads
     * @exception_safety Strong guarantee
     */
    virtual std::string readOutput(int pid, size_t max_bytes = 0) = 0;
    
    /**
     * @brief Check if process has available output
     * @param pid Process ID
     * @return true if output is available for reading
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual bool hasOutput(int pid) noexcept = 0;
    
    // Shell Configuration
    
    /**
     * @brief Get current shell executable path
     * @return Path to shell executable
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getShellPath() = 0;
    
    /**
     * @brief Set shell executable path
     * @param path Path to shell executable
     * @return true if shell path is valid and set
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setShellPath(const std::string& path) = 0;
    
    /**
     * @brief Get current working directory
     * @return Current working directory path
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual std::string getCurrentDirectory() = 0;
    
    /**
     * @brief Change working directory
     * @param path New working directory
     * @return true if directory changed successfully
     * @thread_safe No - affects global shell state
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setCurrentDirectory(const std::string& path) = 0;
    
    /**
     * @brief Get shell environment
     * @return Reference to environment variables
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual Environment& getEnvironment() noexcept = 0;
    
    // Terminal Integration
    
    /**
     * @brief Set terminal size for shell processes
     * @param cols Number of columns
     * @param rows Number of rows
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety No-throw guarantee
     */
    virtual void setTerminalSize(int cols, int rows) noexcept = 0;
    
    /**
     * @brief Enable/disable terminal echo
     * @param enable true to enable echo
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setEcho(bool enable) = 0;
    
    /**
     * @brief Set terminal mode (raw/canonical)
     * @param raw_mode true for raw mode, false for canonical
     * @thread_safe Yes
     * @performance O(1)
     * @exception_safety Strong guarantee
     */
    virtual bool setRawMode(bool raw_mode) = 0;
    
protected:
    /// @brief Protected constructor - use factory method
    IShell() = default;
    
    /// @brief Non-copyable
    IShell(const IShell&) = delete;
    IShell& operator=(const IShell&) = delete;
    
    /// @brief Non-movable
    IShell(IShell&&) = delete;
    IShell& operator=(IShell&&) = delete;
};

/**
 * @brief RAII wrapper for process management
 * 
 * Automatically terminates process on destruction for
 * exception-safe process lifecycle management.
 */
class ProcessGuard {
private:
    IShell& shell_;
    int pid_;
    bool auto_terminate_;
    
public:
    /**
     * @brief Constructor - manages process lifecycle
     * @param shell Shell reference
     * @param pid Process ID to manage
     * @param auto_terminate Automatically terminate on destruction
     */
    ProcessGuard(IShell& shell, int pid, bool auto_terminate = true)
        : shell_(shell), pid_(pid), auto_terminate_(auto_terminate) {}
    
    /**
     * @brief Destructor - terminates process if needed
     */
    ~ProcessGuard() {
        if (auto_terminate_ && pid_ > 0) {
            auto info = shell_.getProcessInfo(pid_);
            if (info.isActive()) {
                shell_.terminateProcess(pid_);
            }
        }
    }
    
    /// @brief Non-copyable, non-movable
    ProcessGuard(const ProcessGuard&) = delete;
    ProcessGuard& operator=(const ProcessGuard&) = delete;
    ProcessGuard(ProcessGuard&&) = delete;
    ProcessGuard& operator=(ProcessGuard&&) = delete;
    
    /// @brief Get managed process ID
    int getPid() const noexcept { return pid_; }
    
    /// @brief Release management (don't auto-terminate)
    void release() noexcept { auto_terminate_ = false; }
    
    /// @brief Get process information
    ProcessInfo getInfo() { return shell_.getProcessInfo(pid_); }
};

} // namespace core
} // namespace cross_terminal