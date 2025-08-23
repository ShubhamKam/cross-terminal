#include "terminal.h"
#include "shell.h"
#include "command_parser.h"
#include "process_manager.h"
#include "history.h"
#include <iostream>
#include <sstream>

Terminal::Terminal() 
    : m_prompt("$ "), m_hardwareControlEnabled(false) {
}

Terminal::~Terminal() {
    shutdown();
}

bool Terminal::initialize() {
    try {
        m_shell = std::make_unique<Shell>();
        m_parser = std::make_unique<CommandParser>();
        m_processManager = std::make_unique<ProcessManager>();
        m_history = std::make_unique<History>();

        if (!m_shell->initialize()) {
            return false;
        }

        m_workingDirectory = m_shell->getCurrentDirectory();
        updatePrompt();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Terminal initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void Terminal::shutdown() {
    if (m_processManager) {
        m_processManager->terminateAll();
    }
    
    m_shell.reset();
    m_parser.reset();
    m_processManager.reset();
    m_history.reset();
}

void Terminal::update() {
    if (m_processManager) {
        m_processManager->update();
    }
}

void Terminal::executeCommand(const std::string& command) {
    if (command.empty()) {
        return;
    }

    addToHistory(command);

    try {
        auto parsedCommand = m_parser->parse(command);
        
        if (parsedCommand.isBuiltin) {
            handleBuiltinCommand(parsedCommand);
        } else {
            auto process = m_shell->execute(parsedCommand.executable, parsedCommand.arguments);
            if (process) {
                m_processManager->addProcess(std::move(process));
            }
        }
    } catch (const std::exception& e) {
        processOutput("Error: " + std::string(e.what()) + "\n");
    }
}

void Terminal::sendInput(const std::string& input) {
    m_processManager->sendInputToForeground(input);
}

void Terminal::clear() {
    m_output.clear();
    m_lines.clear();
    
    if (m_outputCallback) {
        m_outputCallback("");
    }
}

void Terminal::resize(int width, int height) {
    if (m_shell) {
        m_shell->setTerminalSize(width, height);
    }
}

std::string Terminal::getOutput() const {
    return m_output;
}

std::vector<std::string> Terminal::getLines() const {
    return m_lines;
}

size_t Terminal::getLineCount() const {
    return m_lines.size();
}

std::vector<std::string> Terminal::getHistory() const {
    return m_history->getCommands();
}

void Terminal::addToHistory(const std::string& command) {
    m_history->addCommand(command);
}

void Terminal::setPrompt(const std::string& prompt) {
    m_prompt = prompt;
}

std::string Terminal::getPrompt() const {
    return m_prompt;
}

void Terminal::setWorkingDirectory(const std::string& path) {
    if (m_shell && m_shell->changeDirectory(path)) {
        m_workingDirectory = path;
        updatePrompt();
    }
}

std::string Terminal::getWorkingDirectory() const {
    return m_workingDirectory;
}

void Terminal::enableHardwareControl(bool enable) {
    m_hardwareControlEnabled = enable;
}

bool Terminal::isHardwareControlEnabled() const {
    return m_hardwareControlEnabled;
}

void Terminal::setOutputCallback(OutputCallback callback) {
    m_outputCallback = callback;
}

void Terminal::processOutput(const std::string& output) {
    m_output += output;
    
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        m_lines.push_back(line);
    }
    
    if (m_outputCallback) {
        m_outputCallback(output);
    }
}

void Terminal::updatePrompt() {
    std::string shortPath = m_workingDirectory;
    
    // Shorten path for display
    size_t homePos = shortPath.find(getenv("HOME") ? getenv("HOME") : "");
    if (homePos != std::string::npos) {
        shortPath.replace(homePos, strlen(getenv("HOME")), "~");
    }
    
    m_prompt = shortPath + " $ ";
}

void Terminal::handleBuiltinCommand(const ParsedCommand& command) {
    if (command.executable == "cd") {
        std::string path = command.arguments.empty() ? 
            (getenv("HOME") ? getenv("HOME") : "/") : 
            command.arguments[0];
        setWorkingDirectory(path);
    }
    else if (command.executable == "clear") {
        clear();
    }
    else if (command.executable == "pwd") {
        processOutput(m_workingDirectory + "\n");
    }
    else if (command.executable == "history") {
        auto hist = getHistory();
        for (size_t i = 0; i < hist.size(); ++i) {
            processOutput(std::to_string(i + 1) + " " + hist[i] + "\n");
        }
    }
}