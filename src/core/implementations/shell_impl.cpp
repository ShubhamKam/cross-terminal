#include "shell_impl.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

namespace cross_terminal {
namespace core {

// Static member definitions
ShellImpl::ProcessPool ShellImpl::process_pool_;
CommandParser::TokenPool CommandParser::token_pool_;

// ProcessHandle implementation
ProcessHandle::ProcessHandle() {
#ifdef _WIN32
    process_handle = nullptr;
    thread_handle = nullptr;
    process_id = 0;
    thread_id = 0;
#else
    pid = -1;
    stdin_fd = -1;
    stdout_fd = -1;
    stderr_fd = -1;
#endif
}

ProcessHandle::~ProcessHandle() {
    close();
}

bool ProcessHandle::isValid() const noexcept {
#ifdef _WIN32
    return process_handle != nullptr && process_handle != INVALID_HANDLE_VALUE;
#else
    return pid > 0;
#endif
}

void ProcessHandle::close() noexcept {
#ifdef _WIN32
    if (thread_handle && thread_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(thread_handle);
        thread_handle = nullptr;
    }
    if (process_handle && process_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(process_handle);
        process_handle = nullptr;
    }
#else
    if (stdin_fd >= 0) {
        ::close(stdin_fd);
        stdin_fd = -1;
    }
    if (stdout_fd >= 0) {
        ::close(stdout_fd);
        stdout_fd = -1;
    }
    if (stderr_fd >= 0) {
        ::close(stderr_fd);
        stderr_fd = -1;
    }
#endif
}

// ProcessIO implementation
ProcessIO::ProcessIO() 
    : stdout_buffer_(std::make_unique<char[]>(BUFFER_SIZE))
    , stderr_buffer_(std::make_unique<char[]>(BUFFER_SIZE))
    , stdout_size_(0)
    , stderr_size_(0) {
}

ProcessIO::ProcessIO(ProcessIO&& other) noexcept
    : stdout_buffer_(std::move(other.stdout_buffer_))
    , stderr_buffer_(std::move(other.stderr_buffer_))
    , stdout_size_(other.stdout_size_)
    , stderr_size_(other.stderr_size_) {
    other.stdout_size_ = 0;
    other.stderr_size_ = 0;
}

ProcessIO& ProcessIO::operator=(ProcessIO&& other) noexcept {
    if (this != &other) {
        stdout_buffer_ = std::move(other.stdout_buffer_);
        stderr_buffer_ = std::move(other.stderr_buffer_);
        stdout_size_ = other.stdout_size_;
        stderr_size_ = other.stderr_size_;
        other.stdout_size_ = 0;
        other.stderr_size_ = 0;
    }
    return *this;
}

void ProcessIO::appendStdout(const char* data, size_t size) {
    std::unique_lock lock(io_mutex_);
    
    if (stdout_size_ + size > BUFFER_SIZE) {
        // Reallocate larger buffer or implement circular buffer
        size_t new_size = std::max(BUFFER_SIZE * 2, stdout_size_ + size);
        auto new_buffer = std::make_unique<char[]>(new_size);
        std::memcpy(new_buffer.get(), stdout_buffer_.get(), stdout_size_);
        stdout_buffer_ = std::move(new_buffer);
    }
    
    std::memcpy(stdout_buffer_.get() + stdout_size_, data, size);
    stdout_size_ += size;
}

void ProcessIO::appendStderr(const char* data, size_t size) {
    std::unique_lock lock(io_mutex_);
    
    if (stderr_size_ + size > BUFFER_SIZE) {
        size_t new_size = std::max(BUFFER_SIZE * 2, stderr_size_ + size);
        auto new_buffer = std::make_unique<char[]>(new_size);
        std::memcpy(new_buffer.get(), stderr_buffer_.get(), stderr_size_);
        stderr_buffer_ = std::move(new_buffer);
    }
    
    std::memcpy(stderr_buffer_.get() + stderr_size_, data, size);
    stderr_size_ += size;
}

std::string ProcessIO::getStdout() const {
    std::shared_lock lock(io_mutex_);
    return std::string(stdout_buffer_.get(), stdout_size_);
}

std::string ProcessIO::getStderr() const {
    std::shared_lock lock(io_mutex_);
    return std::string(stderr_buffer_.get(), stderr_size_);
}

std::string ProcessIO::getAllOutput() const {
    std::shared_lock lock(io_mutex_);
    std::string result;
    result.reserve(stdout_size_ + stderr_size_);
    result.append(stdout_buffer_.get(), stdout_size_);
    result.append(stderr_buffer_.get(), stderr_size_);
    return result;
}

void ProcessIO::clear() noexcept {
    std::unique_lock lock(io_mutex_);
    stdout_size_ = 0;
    stderr_size_ = 0;
}

bool ProcessIO::hasData() const noexcept {
    std::shared_lock lock(io_mutex_);
    return stdout_size_ > 0 || stderr_size_ > 0;
}

size_t ProcessIO::getStdoutSize() const noexcept {
    std::shared_lock lock(io_mutex_);
    return stdout_size_;
}

size_t ProcessIO::getStderrSize() const noexcept {
    std::shared_lock lock(io_mutex_);
    return stderr_size_;
}

// ManagedProcess implementation
ManagedProcess::ManagedProcess(int pid, const std::string& command, 
                              const std::vector<std::string>& args)
    : running_(false), io_thread_active_(false) {
    info_.pid = pid;
    info_.command = command;
    info_.arguments = args;
    info_.state = ProcessState::NotStarted;
    info_.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

ManagedProcess::~ManagedProcess() {
    if (running_.load()) {
        terminate(true); // Force termination
    }
    
    if (io_thread_active_.load() && io_thread_.joinable()) {
        io_thread_active_.store(false);
        io_thread_.join();
    }
}

ManagedProcess::ManagedProcess(ManagedProcess&& other) noexcept
    : handle_(std::move(other.handle_))
    , info_(std::move(other.info_))
    , io_(std::move(other.io_))
    , running_(other.running_.load())
    , io_thread_active_(other.io_thread_active_.load())
    , io_thread_(std::move(other.io_thread_))
    , output_callback_(std::move(other.output_callback_))
    , completion_callback_(std::move(other.completion_callback_)) {
    
    other.running_.store(false);
    other.io_thread_active_.store(false);
}

ManagedProcess& ManagedProcess::operator=(ManagedProcess&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        if (running_.load()) {
            terminate(true);
        }
        if (io_thread_active_.load() && io_thread_.joinable()) {
            io_thread_active_.store(false);
            io_thread_.join();
        }
        
        // Move from other
        handle_ = std::move(other.handle_);
        info_ = std::move(other.info_);
        io_ = std::move(other.io_);
        running_.store(other.running_.load());
        io_thread_active_.store(other.io_thread_active_.load());
        io_thread_ = std::move(other.io_thread_);
        output_callback_ = std::move(other.output_callback_);
        completion_callback_ = std::move(other.completion_callback_);
        
        other.running_.store(false);
        other.io_thread_active_.store(false);
    }
    return *this;
}

bool ManagedProcess::start(const ExecutionOptions& options) {
    if (running_.load()) {
        return false; // Already running
    }
    
    info_.state = ProcessState::Running;
    running_.store(true);
    
    // Start I/O monitoring thread
    io_thread_active_.store(true);
    io_thread_ = std::thread(&ManagedProcess::ioThreadFunction, this);
    
    return true;
}

bool ManagedProcess::terminate(bool force) noexcept {
    if (!running_.load()) {
        return true; // Already terminated
    }
    
    bool success = false;
    
#ifdef _WIN32
    if (handle_.isValid()) {
        UINT exit_code = force ? 1 : 0;
        success = TerminateProcess(handle_.process_handle, exit_code) != 0;
    }
#else
    if (handle_.pid > 0) {
        int signal = force ? SIGKILL : SIGTERM;
        success = kill(handle_.pid, signal) == 0;
    }
#endif
    
    if (success) {
        running_.store(false);
        info_.state = ProcessState::Terminated;
        info_.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // Stop I/O thread
        if (io_thread_active_.load()) {
            io_thread_active_.store(false);
            if (io_thread_.joinable()) {
                io_thread_.join();
            }
        }
        
        notifyCompletion();
    }
    
    return success;
}

bool ManagedProcess::suspend() {
    if (!running_.load()) {
        return false;
    }
    
#ifdef _WIN32
    // Windows doesn't have direct process suspension
    return false;
#else
    if (handle_.pid > 0 && kill(handle_.pid, SIGSTOP) == 0) {
        info_.state = ProcessState::Suspended;
        return true;
    }
#endif
    
    return false;
}

bool ManagedProcess::resume() {
    if (info_.state != ProcessState::Suspended) {
        return false;
    }
    
#ifdef _WIN32
    return false;
#else
    if (handle_.pid > 0 && kill(handle_.pid, SIGCONT) == 0) {
        info_.state = ProcessState::Running;
        return true;
    }
#endif
    
    return false;
}

bool ManagedProcess::sendInput(const std::string& input) {
    if (!running_.load()) {
        return false;
    }
    
#ifdef _WIN32
    // Implementation for Windows
    return false; // Simplified for now
#else
    if (handle_.stdin_fd >= 0) {
        ssize_t written = write(handle_.stdin_fd, input.c_str(), input.length());
        return written >= 0;
    }
#endif
    
    return false;
}

std::string ManagedProcess::readOutput(size_t max_bytes) {
    if (max_bytes == 0) {
        return io_.getAllOutput();
    } else {
        std::string output = io_.getAllOutput();
        if (output.size() <= max_bytes) {
            return output;
        }
        return output.substr(0, max_bytes);
    }
}

bool ManagedProcess::hasOutput() const noexcept {
    return io_.hasData();
}

ProcessInfo ManagedProcess::getInfo() const {
    return info_;
}

bool ManagedProcess::isRunning() const noexcept {
    return running_.load();
}

bool ManagedProcess::isComplete() const noexcept {
    return info_.state == ProcessState::Completed || 
           info_.state == ProcessState::Failed ||
           info_.state == ProcessState::Terminated;
}

void ManagedProcess::setOutputCallback(IShell::OutputCallback callback) {
    output_callback_ = callback;
}

void ManagedProcess::setCompletionCallback(IShell::CompletionCallback callback) {
    completion_callback_ = callback;
}

void ManagedProcess::ioThreadFunction() {
    char buffer[4096];
    
    while (io_thread_active_.load()) {
#ifndef _WIN32
        fd_set read_fds;
        FD_ZERO(&read_fds);
        
        int max_fd = -1;
        if (handle_.stdout_fd >= 0) {
            FD_SET(handle_.stdout_fd, &read_fds);
            max_fd = std::max(max_fd, handle_.stdout_fd);
        }
        if (handle_.stderr_fd >= 0) {
            FD_SET(handle_.stderr_fd, &read_fds);
            max_fd = std::max(max_fd, handle_.stderr_fd);
        }
        
        if (max_fd < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        struct timeval timeout = {0, 100000}; // 100ms timeout
        int result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (result > 0) {
            if (handle_.stdout_fd >= 0 && FD_ISSET(handle_.stdout_fd, &read_fds)) {
                ssize_t bytes_read = read(handle_.stdout_fd, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    io_.appendStdout(buffer, bytes_read);
                    notifyOutput(std::string(buffer, bytes_read), false);
                }
            }
            
            if (handle_.stderr_fd >= 0 && FD_ISSET(handle_.stderr_fd, &read_fds)) {
                ssize_t bytes_read = read(handle_.stderr_fd, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    io_.appendStderr(buffer, bytes_read);
                    notifyOutput(std::string(buffer, bytes_read), true);
                }
            }
        }
        
        // Check if process is still running
        if (handle_.pid > 0) {
            int status;
            pid_t result = waitpid(handle_.pid, &status, WNOHANG);
            if (result > 0) {
                // Process completed
                running_.store(false);
                info_.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                if (WIFEXITED(status)) {
                    info_.exit_code = WEXITSTATUS(status);
                    info_.state = (info_.exit_code == 0) ? ProcessState::Completed : ProcessState::Failed;
                } else if (WIFSIGNALED(status)) {
                    info_.exit_code = WTERMSIG(status);
                    info_.state = ProcessState::Terminated;
                }
                
                notifyCompletion();
                break;
            }
        }
#endif
    }
}

void ManagedProcess::notifyOutput(const std::string& output, bool is_error) {
    if (output_callback_) {
        output_callback_(output, is_error);
    }
}

void ManagedProcess::notifyCompletion() {
    if (completion_callback_) {
        completion_callback_(info_);
    }
}

// ShellImpl implementation
ShellImpl::ShellImpl() 
    : next_pid_(1000), cleanup_active_(false) {
    
    // Initialize default shell path
#ifdef _WIN32
    shell_path_ = "cmd.exe";
#else
    const char* shell_env = getenv("SHELL");
    shell_path_ = shell_env ? shell_env : "/bin/sh";
#endif

    // Get current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        current_directory_ = cwd;
    } else {
        current_directory_ = "/";
    }
    
    // Import system environment
    environment_.importFromSystem();
}

ShellImpl::~ShellImpl() {
    shutdown();
}

bool ShellImpl::initialize() {
    // Start cleanup thread
    cleanup_active_.store(true);
    cleanup_thread_ = std::thread(&ShellImpl::cleanupThreadFunction, this);
    
    return true;
}

void ShellImpl::shutdown() noexcept {
    // Stop cleanup thread
    if (cleanup_active_.load()) {
        cleanup_active_.store(false);
        cleanup_condition_.notify_all();
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }
    
    // Terminate all active processes
    std::unique_lock lock(processes_mutex_);
    for (auto& [pid, process] : active_processes_) {
        if (process && process->isRunning()) {
            process->terminate(true);
        }
    }
    active_processes_.clear();
}

ProcessInfo ShellImpl::executeSync(const std::string& command,
                                  const ExecutionOptions& options) {
    auto parsed = parseCommand(command);
    if (!parsed.isValid()) {
        ProcessInfo info;
        info.state = ProcessState::Failed;
        info.exit_code = -1;
        return info;
    }
    
    if (isBuiltinCommand(parsed.executable)) {
        return executeBuiltin(parsed.executable, parsed.arguments, options);
    }
    
    // Create and start process
    auto process = createProcess(parsed.executable, parsed.arguments);
    if (!process) {
        ProcessInfo info;
        info.state = ProcessState::Failed;
        info.exit_code = -1;
        return info;
    }
    
    int pid = next_pid_.fetch_add(1);
    process->start(options);
    
    // Wait for completion (synchronous execution)
    while (process->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return process->getInfo();
}

int ShellImpl::executeAsync(const std::string& command,
                           const ExecutionOptions& options,
                           OutputCallback output_callback,
                           CompletionCallback completion_callback) {
    auto parsed = parseCommand(command);
    if (!parsed.isValid()) {
        return -1;
    }
    
    auto process = createProcess(parsed.executable, parsed.arguments);
    if (!process) {
        return -1;
    }
    
    int pid = next_pid_.fetch_add(1);
    process->setOutputCallback(output_callback);
    process->setCompletionCallback(completion_callback);
    
    if (!process->start(options)) {
        return -1;
    }
    
    // Store in active processes
    {
        std::unique_lock lock(processes_mutex_);
        active_processes_[pid] = std::move(process);
    }
    
    return pid;
}

int ShellImpl::executeInteractive(const std::string& command,
                                 const ExecutionOptions& options) {
    auto parsed = parseCommand(command);
    if (!parsed.isValid()) {
        return -1;
    }
    
    auto process = createProcess(parsed.executable, parsed.arguments);
    if (!process) {
        return -1;
    }
    
    int pid = next_pid_.fetch_add(1);
    
    if (!process->start(options)) {
        return -1;
    }
    
    // Store in active processes
    {
        std::unique_lock lock(processes_mutex_);
        active_processes_[pid] = std::move(process);
    }
    
    return pid;
}

ProcessInfo ShellImpl::getProcessInfo(int pid) {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->getInfo();
    }
    
    ProcessInfo info;
    info.pid = pid;
    info.state = ProcessState::NotStarted;
    return info;
}

std::vector<ProcessInfo> ShellImpl::getAllProcesses() {
    std::shared_lock lock(processes_mutex_);
    std::vector<ProcessInfo> processes;
    processes.reserve(active_processes_.size());
    
    for (const auto& [pid, process] : active_processes_) {
        processes.push_back(process->getInfo());
    }
    
    return processes;
}

bool ShellImpl::terminateProcess(int pid, bool force) noexcept {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->terminate(force);
    }
    return false;
}

bool ShellImpl::suspendProcess(int pid) {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->suspend();
    }
    return false;
}

bool ShellImpl::resumeProcess(int pid) {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->resume();
    }
    return false;
}

bool ShellImpl::sendInput(int pid, const std::string& input) {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->sendInput(input);
    }
    return false;
}

std::string ShellImpl::readOutput(int pid, size_t max_bytes) {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->readOutput(max_bytes);
    }
    return "";
}

bool ShellImpl::hasOutput(int pid) noexcept {
    std::shared_lock lock(processes_mutex_);
    auto it = active_processes_.find(pid);
    if (it != active_processes_.end()) {
        return it->second->hasOutput();
    }
    return false;
}

std::string ShellImpl::getShellPath() {
    return shell_path_;
}

bool ShellImpl::setShellPath(const std::string& path) {
    // Validate shell exists
    if (access(path.c_str(), X_OK) == 0) {
        shell_path_ = path;
        return true;
    }
    return false;
}

std::string ShellImpl::getCurrentDirectory() {
    return current_directory_;
}

bool ShellImpl::setCurrentDirectory(const std::string& path) {
    if (chdir(path.c_str()) == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) {
            current_directory_ = cwd;
            return true;
        }
    }
    return false;
}

Environment& ShellImpl::getEnvironment() noexcept {
    return environment_;
}

void ShellImpl::setTerminalSize(int cols, int rows) noexcept {
    std::lock_guard lock(terminal_mutex_);
    terminal_settings_.cols = cols;
    terminal_settings_.rows = rows;
    
    // Update environment variables
    environment_.set("COLUMNS", std::to_string(cols));
    environment_.set("LINES", std::to_string(rows));
}

bool ShellImpl::setEcho(bool enable) {
    std::lock_guard lock(terminal_mutex_);
    terminal_settings_.echo_enabled = enable;
    
#ifndef _WIN32
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        if (enable) {
            term.c_lflag |= ECHO;
        } else {
            term.c_lflag &= ~ECHO;
        }
        return tcsetattr(STDIN_FILENO, TCSANOW, &term) == 0;
    }
#endif
    
    return false;
}

bool ShellImpl::setRawMode(bool raw_mode) {
    std::lock_guard lock(terminal_mutex_);
    terminal_settings_.raw_mode = raw_mode;
    
#ifndef _WIN32
    struct termios term;
    if (tcgetattr(STDIN_FILENO, &term) == 0) {
        if (raw_mode) {
            term.c_lflag &= ~(ICANON | ECHO);
            term.c_cc[VMIN] = 1;
            term.c_cc[VTIME] = 0;
        } else {
            term.c_lflag |= (ICANON | ECHO);
        }
        return tcsetattr(STDIN_FILENO, TCSANOW, &term) == 0;
    }
#endif
    
    return false;
}

// Private methods
void ShellImpl::cleanupCompletedProcesses() {
    std::unique_lock lock(processes_mutex_);
    
    auto it = active_processes_.begin();
    while (it != active_processes_.end()) {
        if (it->second->isComplete()) {
            it = active_processes_.erase(it);
        } else {
            ++it;
        }
    }
}

void ShellImpl::cleanupThreadFunction() {
    while (cleanup_active_.load()) {
        cleanupCompletedProcesses();
        
        std::unique_lock lock(processes_mutex_);
        cleanup_condition_.wait_for(lock, std::chrono::seconds(5));
    }
}

std::unique_ptr<ManagedProcess> ShellImpl::createProcess(const std::string& command,
                                                       const std::vector<std::string>& args) {
    int pid = next_pid_.load();
    return std::make_unique<ManagedProcess>(pid, command, args);
}

ShellImpl::ParsedCommand ShellImpl::parseCommand(const std::string& command) const {
    CommandParser parser;
    return parser.parse(command, environment_);
}

bool ShellImpl::isBuiltinCommand(const std::string& command) const noexcept {
    static const std::unordered_set<std::string> builtins = {
        "cd", "pwd", "echo", "exit", "export", "jobs", "kill", "help"
    };
    
    return builtins.find(command) != builtins.end();
}

ProcessInfo ShellImpl::executeBuiltin(const std::string& command, 
                                     const std::vector<std::string>& args,
                                     const ExecutionOptions& options) {
    if (command == "cd") {
        return executeBuiltinCd(args);
    } else if (command == "pwd") {
        return executeBuiltinPwd(args);
    } else if (command == "echo") {
        return executeBuiltinEcho(args);
    } else if (command == "exit") {
        return executeBuiltinExit(args);
    } else if (command == "jobs") {
        return executeBuiltinJobs(args);
    } else if (command == "kill") {
        return executeBuiltinKill(args);
    } else if (command == "export") {
        return executeBuiltinExport(args);
    }
    
    ProcessInfo info;
    info.state = ProcessState::Failed;
    info.exit_code = 1;
    return info;
}

ProcessInfo ShellImpl::executeBuiltinCd(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "cd";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string target_dir;
    if (args.empty()) {
        target_dir = environment_.get("HOME");
        if (target_dir.empty()) {
            target_dir = "/";
        }
    } else {
        target_dir = args[0];
    }
    
    if (setCurrentDirectory(target_dir)) {
        info.state = ProcessState::Completed;
        info.exit_code = 0;
    } else {
        info.state = ProcessState::Failed;
        info.exit_code = 1;
    }
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinPwd(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "pwd";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // PWD output would be handled by the caller
    info.state = ProcessState::Completed;
    info.exit_code = 0;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinEcho(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "echo";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Echo output would be handled by the caller
    info.state = ProcessState::Completed;
    info.exit_code = 0;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinExit(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "exit";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    int exit_code = 0;
    if (!args.empty()) {
        try {
            exit_code = std::stoi(args[0]);
        } catch (const std::exception&) {
            exit_code = 1;
        }
    }
    
    info.state = ProcessState::Completed;
    info.exit_code = exit_code;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinJobs(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "jobs";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Jobs output would list active processes
    info.state = ProcessState::Completed;
    info.exit_code = 0;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinKill(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "kill";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (!args.empty()) {
        try {
            int pid = std::stoi(args[0]);
            bool success = terminateProcess(pid, false);
            info.exit_code = success ? 0 : 1;
        } catch (const std::exception&) {
            info.exit_code = 1;
        }
    } else {
        info.exit_code = 1;
    }
    
    info.state = (info.exit_code == 0) ? ProcessState::Completed : ProcessState::Failed;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

ProcessInfo ShellImpl::executeBuiltinExport(const std::vector<std::string>& args) {
    ProcessInfo info;
    info.command = "export";
    info.arguments = args;
    info.start_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    for (const auto& arg : args) {
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
            std::string name = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            environment_.set(name, value);
        }
    }
    
    info.state = ProcessState::Completed;
    info.exit_code = 0;
    
    info.end_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return info;
}

} // namespace core
} // namespace cross_terminal