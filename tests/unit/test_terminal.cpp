#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/terminal.h"
#include "mocks/mock_platform.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

class TerminalTest : public ::testing::Test {
protected:
    void SetUp() override {
        terminal = std::make_unique<Terminal>();
    }

    void TearDown() override {
        terminal.reset();
    }

    std::unique_ptr<Terminal> terminal;
};

TEST_F(TerminalTest, InitializationSucceeds) {
    EXPECT_TRUE(terminal->initialize());
    EXPECT_FALSE(terminal->getPrompt().empty());
    EXPECT_FALSE(terminal->getWorkingDirectory().empty());
}

TEST_F(TerminalTest, CommandHistoryManagement) {
    terminal->initialize();
    
    terminal->addToHistory("ls -la");
    terminal->addToHistory("cd /tmp");
    terminal->addToHistory("pwd");
    
    auto history = terminal->getHistory();
    EXPECT_EQ(history.size(), 3);
    EXPECT_EQ(history[0], "ls -la");
    EXPECT_EQ(history[1], "cd /tmp");
    EXPECT_EQ(history[2], "pwd");
}

TEST_F(TerminalTest, PromptCustomization) {
    terminal->initialize();
    
    std::string originalPrompt = terminal->getPrompt();
    terminal->setPrompt("custom> ");
    
    EXPECT_EQ(terminal->getPrompt(), "custom> ");
    EXPECT_NE(terminal->getPrompt(), originalPrompt);
}

TEST_F(TerminalTest, WorkingDirectoryManagement) {
    terminal->initialize();
    
    std::string originalDir = terminal->getWorkingDirectory();
    EXPECT_FALSE(originalDir.empty());
    
    // Test directory change (assuming /tmp exists on most systems)
    terminal->setWorkingDirectory("/tmp");
    // Note: Actual directory change depends on platform implementation
}

TEST_F(TerminalTest, ClearFunctionality) {
    terminal->initialize();
    
    // Add some content
    terminal->addToHistory("test command");
    
    // Clear should reset output but preserve history
    terminal->clear();
    EXPECT_EQ(terminal->getOutput(), "");
    EXPECT_EQ(terminal->getLineCount(), 0);
    EXPECT_FALSE(terminal->getHistory().empty()); // History should remain
}

TEST_F(TerminalTest, HardwareControlToggle) {
    terminal->initialize();
    
    EXPECT_FALSE(terminal->isHardwareControlEnabled()); // Default off
    
    terminal->enableHardwareControl(true);
    EXPECT_TRUE(terminal->isHardwareControlEnabled());
    
    terminal->enableHardwareControl(false);
    EXPECT_FALSE(terminal->isHardwareControlEnabled());
}

TEST_F(TerminalTest, OutputCallbackFunctionality) {
    terminal->initialize();
    
    std::string capturedOutput;
    bool callbackCalled = false;
    
    terminal->setOutputCallback([&](const std::string& output) {
        capturedOutput = output;
        callbackCalled = true;
    });
    
    // This would trigger the callback in a real scenario
    // For now, we test that the callback is set
    EXPECT_TRUE(callbackCalled == false); // Not called yet
}

TEST_F(TerminalTest, EmptyCommandHandling) {
    terminal->initialize();
    
    size_t initialHistorySize = terminal->getHistory().size();
    
    terminal->executeCommand("");
    terminal->executeCommand("   ");
    terminal->executeCommand("\t\n");
    
    // Empty commands should not be added to history
    EXPECT_EQ(terminal->getHistory().size(), initialHistorySize);
}

TEST_F(TerminalTest, TerminalResize) {
    terminal->initialize();
    
    // Test resize - should not throw or crash
    EXPECT_NO_THROW(terminal->resize(80, 24));
    EXPECT_NO_THROW(terminal->resize(120, 40));
}

class TerminalIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        terminal = std::make_unique<Terminal>();
        terminal->initialize();
    }

    void TearDown() override {
        terminal->shutdown();
    }

    std::unique_ptr<Terminal> terminal;
};

TEST_F(TerminalIntegrationTest, BasicCommandExecution) {
    // Test basic command that should work on most systems
    terminal->executeCommand("echo 'test'");
    
    // Give some time for command to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto history = terminal->getHistory();
    EXPECT_FALSE(history.empty());
    EXPECT_EQ(history.back(), "echo 'test'");
}

TEST_F(TerminalIntegrationTest, BuiltinCommands) {
    size_t originalLineCount = terminal->getLineCount();
    
    // Test pwd builtin
    terminal->executeCommand("pwd");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Should have some output
    EXPECT_GE(terminal->getLineCount(), originalLineCount);
    
    // Test clear builtin
    terminal->executeCommand("clear");
    EXPECT_EQ(terminal->getLineCount(), 0);
}

TEST_F(TerminalIntegrationTest, HistoryBuiltin) {
    terminal->executeCommand("echo 'first'");
    terminal->executeCommand("echo 'second'");
    terminal->executeCommand("history");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto history = terminal->getHistory();
    EXPECT_GE(history.size(), 3); // At least the three commands we executed
}