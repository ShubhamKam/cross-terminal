#include <iostream>
#include <memory>
#include "core/terminal.h"
#include "ui/terminal_ui.h"
#include "platform/platform.h"

int main(int argc, char* argv[]) {
    try {
        // Initialize platform layer
        auto platform = Platform::create();
        if (!platform) {
            std::cerr << "Failed to initialize platform layer" << std::endl;
            return -1;
        }

        // Initialize terminal core
        auto terminal = std::make_unique<Terminal>();
        if (!terminal->initialize()) {
            std::cerr << "Failed to initialize terminal" << std::endl;
            return -1;
        }

        // Initialize UI
        auto ui = std::make_unique<TerminalUI>(terminal.get());
        if (!ui->initialize()) {
            std::cerr << "Failed to initialize UI" << std::endl;
            return -1;
        }

        // Main loop
        while (!ui->shouldClose()) {
            ui->processInput();
            terminal->update();
            ui->render();
        }

        // Cleanup
        ui->shutdown();
        terminal->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}