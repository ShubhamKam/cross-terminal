#pragma once

#include "core/interfaces/i_shell.h"
#include "memory/memory_manager.h"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>

/**
 * @file shell_impl.h
 * @brief Cross-platform shell implementation
 * 
 * Concrete implementation of IShell interface providing
 * process execution, job control, and I/O management.
 * 
 * @performance Optimized with memory pools and I/O threading
 * @thread_safety All operations are thread-safe
 * @memory_model Uses custom memory pools for process management
 */

namespace cross_terminal {
namespace core {

/**
 * @brief Platform-specific process handle wrapper
 */
struct ProcessHandle {
#ifdef _WIN32
    void* process_handle;
    void* thread_handle;
    unsigned long process_id;
    unsigned long thread_id;
#else
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
    int stderr_fd;
#endif
    
    ProcessHandle();
    ~ProcessHandle();
    
    bool isValid() const noexcept;
    void close() noexcept;
};

/**
 * @brief Process I/O buffer management
 */
class ProcessIO {
private:
    static constexpr size_t BUFFER_SIZE = 8192;
    
    std::unique_ptr<char[]> stdout_buffer_;
    std::unique_ptr<char[]> stderr_buffer_;
    size_t stdout_size_;
    size_t stderr_size_;
    mutable std::shared_mutex io_mutex_;
    
public:
    ProcessIO();
    ~ProcessIO() = default;
    
    // Non-copyable, movable
    ProcessIO(const ProcessIO&) = delete;
    ProcessIO& operator=(const ProcessIO&) = delete;
    ProcessIO(ProcessIO&&) noexcept;
    ProcessIO& operator=(ProcessIO&&) noexcept;
    
    void appendStdout(const char* data, size_t size);
    void appendStderr(const char* data, size_t size);
    
    std::string getStdout() const;
    std::string getStderr() const;
    std::string getAllOutput() const;
    
    void clear() noexcept;
    bool hasData() const noexcept;
    size_t getStdoutSize() const noexcept;
    size_t getStderrSize() const noexcept;
};

/**
 * @brief Managed process wrapper
 */
class ManagedProcess {
private:
    ProcessHandle handle_;
    ProcessInfo info_;
    ProcessIO io_;
    std::atomic<bool> running_;
    std::atomic<bool> io_thread_active_;
    std::thread io_thread_;
    
    IShell::OutputCallback output_callback_;
    IShell::CompletionCallback completion_callback_;
    
    // I/O monitoring
    void ioThreadFunction();
    void notifyOutput(const std::string& output, bool is_error);
    void notifyCompletion();
    
public:
    ManagedProcess(int pid, const std::string& command, 
                  const std::vector<std::string>& args);
    ~ManagedProcess();
    
    // Non-copyable, movable
    ManagedProcess(const ManagedProcess&) = delete;
    ManagedProcess& operator=(const ManagedProcess&) = delete;
    ManagedProcess(ManagedProcess&&) noexcept;
    ManagedProcess& operator=(ManagedProcess&&) noexcept;
    
    // Process control
    bool start(const ExecutionOptions& options);
    bool terminate(bool force = false) noexcept;
    bool suspend();
    bool resume();
    
    // I/O operations
    bool sendInput(const std::string& input);
    std::string readOutput(size_t max_bytes = 0);
    bool hasOutput() const noexcept;
    
    // Status queries
    ProcessInfo getInfo() const;
    bool isRunning() const noexcept;
    bool isComplete() const noexcept;
    
    // Callbacks
    void setOutputCallback(IShell::OutputCallback callback);
    void setCompletionCallback(IShell::CompletionCallback callback);
};

/**
 * @brief Concrete shell implementation
 * 
 * Provides cross-platform shell execution with process management,
 * job control, and optimized I/O handling.
 */
class ShellImpl : public IShell {
private:
    // Memory management
    using ProcessPool = memory::MemoryPool<ManagedProcess, 64>;
    static ProcessPool process_pool_;
    
    // Process management
    std::unordered_map<int, std::unique_ptr<ManagedProcess>> active_processes_;
    mutable std::shared_mutex processes_mutex_;
    std::atomic<int> next_pid_;
    
    // Shell configuration
    std::string shell_path_;
    std::string current_directory_;
    Environment environment_;
    
    // Terminal settings
    struct {
        int cols = 80;
        int rows = 24;
        bool echo_enabled = true;
        bool raw_mode = false;
    } terminal_settings_;
    
    mutable std::mutex terminal_mutex_;
    
    // Background cleanup thread
    std::atomic<bool> cleanup_active_;
    std::thread cleanup_thread_;
    std::condition_variable cleanup_condition_;
    
    // Process lifecycle
    void cleanupCompletedProcesses();
    void cleanupThreadFunction();
    std::unique_ptr<ManagedProcess> createProcess(const std::string& command,
                                                const std::vector<std::string>& args);
    
    // Command parsing
    struct ParsedCommand {
        std::string executable;
        std::vector<std::string> arguments;
        std::vector<std::string> input_redirections;
        std::vector<std::string> output_redirections;
        bool append_output = false;
        bool run_in_background = false;
        
        bool isValid() const noexcept {
            return !executable.empty();
        }
    };
    
    ParsedCommand parseCommand(const std::string& command) const;
    bool isBuiltinCommand(const std::string& command) const noexcept;
    ProcessInfo executeBuiltin(const std::string& command, 
                             const std::vector<std::string>& args,
                             const ExecutionOptions& options);
    
    // Platform-specific implementations
#ifdef _WIN32
    bool createWindowsProcess(const ParsedCommand& cmd, 
                            const ExecutionOptions& options,
                            ProcessHandle& handle);
#else
    bool createUnixProcess(const ParsedCommand& cmd,
                         const ExecutionOptions& options, 
                         ProcessHandle& handle);
#endif
    
public:
    ShellImpl();
    virtual ~ShellImpl();
    
    // IShell implementation
    bool initialize() override;
    void shutdown() noexcept override;
    
    ProcessInfo executeSync(const std::string& command,
                          const ExecutionOptions& options = {}) override;
    
    int executeAsync(const std::string& command,
                    const ExecutionOptions& options,
                    OutputCallback output_callback,
                    CompletionCallback completion_callback) override;
    
    int executeInteractive(const std::string& command,
                          const ExecutionOptions& options = {}) override;
    
    ProcessInfo getProcessInfo(int pid) override;
    std::vector<ProcessInfo> getAllProcesses() override;
    bool terminateProcess(int pid, bool force = false) noexcept override;
    bool suspendProcess(int pid) override;
    bool resumeProcess(int pid) override;
    
    bool sendInput(int pid, const std::string& input) override;
    std::string readOutput(int pid, size_t max_bytes = 0) override;
    bool hasOutput(int pid) noexcept override;
    
    std::string getShellPath() override;
    bool setShellPath(const std::string& path) override;
    std::string getCurrentDirectory() override;
    bool setCurrentDirectory(const std::string& path) override;
    Environment& getEnvironment() noexcept override;
    
    void setTerminalSize(int cols, int rows) noexcept override;
    bool setEcho(bool enable) override;
    bool setRawMode(bool raw_mode) override;
    
private:
    // Utility methods
    std::string expandPath(const std::string& path) const;
    bool validateCommand(const std::string& command) const noexcept;
    void updateProcessState(int pid, ProcessState state, int exit_code = 0);
    
    // Built-in commands
    ProcessInfo executeBuiltinCd(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinPwd(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinEcho(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinExit(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinJobs(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinKill(const std::vector<std::string>& args);
    ProcessInfo executeBuiltinExport(const std::vector<std::string>& args);
};

/**
 * @brief Command parser utility
 * 
 * Parses shell command strings into structured command objects
 * with support for arguments, redirections, and job control.
 */
class CommandParser {
private:
    using TokenPool = memory::MemoryPool<std::string, 256>;
    static TokenPool token_pool_;
    
    enum class TokenType {
        Word,
        Pipe,
        Redirect,
        Background,
        Semicolon,
        And,
        Or
    };
    
    struct Token {
        TokenType type;
        std::string value;
        size_t position;
        
        Token(TokenType t, const std::string& v, size_t pos)
            : type(t), value(v), position(pos) {}
    };
    
    std::vector<Token> tokenize(const std::string& command) const;
    bool isQuoted(const std::string& str) const noexcept;
    std::string removeQuotes(const std::string& str) const;
    std::string expandVariables(const std::string& str, const Environment& env) const;
    
public:
    /**
     * @brief Parse command string into structured representation
     * @param command Command string to parse
     * @param env Environment for variable expansion
     * @return Parsed command structure
     * @thread_safe Yes
     * @performance O(n) where n is command length
     */
    ShellImpl::ParsedCommand parse(const std::string& command, 
                                  const Environment& env) const;
    
    /**
     * @brief Validate command syntax
     * @param command Command string to validate
     * @return true if syntax is valid
     * @thread_safe Yes
     * @performance O(n) where n is command length
     */
    bool validate(const std::string& command) const noexcept;
    
    /**
     * @brief Get completion suggestions for partial command
     * @param partial_command Incomplete command string
     * @param env Environment for context
     * @return Vector of completion suggestions
     * @thread_safe Yes
     * @performance O(n) where n is number of available commands
     */
    std::vector<std::string> getCompletions(const std::string& partial_command,
                                          const Environment& env) const;
};

} // namespace core
} // namespace cross_terminal