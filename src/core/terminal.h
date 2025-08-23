#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

class Shell;
class CommandParser;
class ProcessManager;
class History;

class Terminal {
public:
    Terminal();
    ~Terminal();

    bool initialize();
    void shutdown();
    void update();

    // Core terminal operations
    void executeCommand(const std::string& command);
    void sendInput(const std::string& input);
    void clear();
    void resize(int width, int height);

    // Output handling
    std::string getOutput() const;
    std::vector<std::string> getLines() const;
    size_t getLineCount() const;

    // History management
    std::vector<std::string> getHistory() const;
    void addToHistory(const std::string& command);

    // Settings
    void setPrompt(const std::string& prompt);
    std::string getPrompt() const;
    void setWorkingDirectory(const std::string& path);
    std::string getWorkingDirectory() const;

    // Hardware control interface
    void enableHardwareControl(bool enable);
    bool isHardwareControlEnabled() const;

    // Event callbacks
    using OutputCallback = std::function<void(const std::string&)>;
    void setOutputCallback(OutputCallback callback);

private:
    std::unique_ptr<Shell> m_shell;
    std::unique_ptr<CommandParser> m_parser;
    std::unique_ptr<ProcessManager> m_processManager;
    std::unique_ptr<History> m_history;
    
    std::string m_output;
    std::vector<std::string> m_lines;
    std::string m_prompt;
    std::string m_workingDirectory;
    bool m_hardwareControlEnabled;
    
    OutputCallback m_outputCallback;
    
    void processOutput(const std::string& output);
    void updatePrompt();
};