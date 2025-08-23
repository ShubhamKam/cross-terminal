#!/bin/bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${GREEN}[SETUP]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Detect operating system
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

# Install development dependencies
install_dependencies() {
    local os=$(detect_os)
    
    print_status "Installing development dependencies for $os..."
    
    case $os in
        "linux")
            # Update package manager
            sudo apt-get update
            
            # Essential build tools
            sudo apt-get install -y \
                build-essential \
                cmake \
                git \
                python3 \
                python3-pip \
                pkg-config
            
            # Testing and quality tools
            sudo apt-get install -y \
                libgtest-dev \
                libgmock-dev \
                lcov \
                cppcheck \
                clang-format \
                clang-tidy \
                valgrind
            
            # Android development (optional)
            if command -v java >/dev/null 2>&1; then
                print_info "Java found, Android development tools can be installed"
            else
                print_warning "Java not found, install JDK for Android development"
            fi
            ;;
            
        "macos")
            # Check for Homebrew
            if ! command -v brew >/dev/null 2>&1; then
                print_status "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
            fi
            
            # Install dependencies via Homebrew
            brew install \
                cmake \
                lcov \
                cppcheck \
                clang-format \
                python3 \
                pkg-config
            
            # Xcode command line tools
            if ! xcode-select -p >/dev/null 2>&1; then
                print_status "Installing Xcode command line tools..."
                xcode-select --install
            fi
            ;;
            
        "windows")
            print_warning "Windows development setup requires manual installation"
            print_info "Please install:"
            print_info "  - Visual Studio 2019/2022 with C++ support"
            print_info "  - CMake"
            print_info "  - Git for Windows"
            print_info "  - Python 3"
            ;;
            
        *)
            print_error "Unsupported operating system: $OSTYPE"
            exit 1
            ;;
    esac
}

# Setup Python environment
setup_python_env() {
    print_status "Setting up Python environment..."
    
    # Install Python tools
    pip3 install --user \
        pre-commit \
        cmake-format \
        cmakelang \
        safety \
        black \
        flake8 \
        pytest
    
    # Install pre-commit hooks
    cd "${PROJECT_DIR}"
    pre-commit install
    pre-commit install --hook-type commit-msg
    
    print_status "Python environment setup complete"
}

# Setup Git hooks
setup_git_hooks() {
    print_status "Setting up Git hooks..."
    
    cd "${PROJECT_DIR}"
    
    # Create pre-commit hook script
    cat > .git/hooks/pre-commit << 'EOF'
#!/bin/bash

# Run pre-commit hooks
pre-commit run --all-files

# Run fast tests
if [ -x "./test.sh" ]; then
    ./test.sh --fast
fi
EOF
    
    chmod +x .git/hooks/pre-commit
    
    # Create commit-msg hook for conventional commits
    cat > .git/hooks/commit-msg << 'EOF'
#!/bin/bash

# Check commit message format
commit_regex='^(feat|fix|docs|style|refactor|test|chore)(\(.+\))?: .{1,50}'
error_msg="Invalid commit message format. Use: type(scope): description"

if ! grep -qE "$commit_regex" "$1"; then
    echo "$error_msg" >&2
    exit 1
fi
EOF
    
    chmod +x .git/hooks/commit-msg
    
    print_status "Git hooks setup complete"
}

# Create development configuration
create_dev_config() {
    print_status "Creating development configuration..."
    
    cd "${PROJECT_DIR}"
    
    # Create .vscode directory with settings
    mkdir -p .vscode
    
    # VS Code settings
    cat > .vscode/settings.json << 'EOF'
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Unix Makefiles",
    "files.trimTrailingWhitespace": true,
    "files.insertFinalNewline": true,
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_style": "Google",
    "cmake.configureOnOpen": true,
    "cmake.buildTask": true,
    "cmake.testTask": true
}
EOF
    
    # VS Code tasks
    cat > .vscode/tasks.json << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build",
            "type": "shell",
            "command": "./build.sh",
            "group": "build",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Test",
            "type": "shell",
            "command": "./test.sh",
            "group": "test",
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "rm",
            "args": ["-rf", "build", "build-*"],
            "group": "build"
        }
    ]
}
EOF
    
    # VS Code launch configuration
    cat > .vscode/launch.json << 'EOF'
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Terminal",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/cross-terminal",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build"
        }
    ]
}
EOF
    
    print_status "Development configuration created"
}

# Setup Android development environment
setup_android_dev() {
    local setup_android=false
    
    read -p "Setup Android development environment? (y/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        setup_android=true
    fi
    
    if [ "$setup_android" = true ]; then
        print_status "Setting up Android development environment..."
        
        # Check for Android SDK
        if [ -z "${ANDROID_SDK_ROOT}" ]; then
            print_warning "ANDROID_SDK_ROOT not set"
            print_info "Please install Android Studio and set ANDROID_SDK_ROOT"
        fi
        
        # Check for Android NDK
        if [ -z "${ANDROID_NDK}" ]; then
            print_warning "ANDROID_NDK not set"
            print_info "Please install Android NDK and set ANDROID_NDK"
        fi
        
        # Create Android project files
        mkdir -p "${PROJECT_DIR}/android/app/src/main/java/com/crossplatform/terminal"
        mkdir -p "${PROJECT_DIR}/android/app/src/main/res/values"
        mkdir -p "${PROJECT_DIR}/android/app/src/test/java"
        mkdir -p "${PROJECT_DIR}/android/app/src/androidTest/java"
        
        print_status "Android development environment setup complete"
    fi
}

# Run initial build and tests
run_initial_tests() {
    print_status "Running initial build and tests..."
    
    cd "${PROJECT_DIR}"
    
    # Initial build
    if ./build.sh --debug; then
        print_status "Initial build successful"
    else
        print_warning "Initial build failed, but environment is set up"
    fi
    
    # Run tests if available
    if [ -x "./test.sh" ] && [ -d "build" ]; then
        if ./test.sh --unit; then
            print_status "Initial tests passed"
        else
            print_warning "Some tests failed, but environment is set up"
        fi
    fi
}

# Main setup function
main() {
    print_status "Setting up Cross-Terminal development environment..."
    
    # Change to project directory
    cd "${PROJECT_DIR}"
    
    # Install system dependencies
    install_dependencies
    
    # Setup Python environment and tools
    setup_python_env
    
    # Setup Git hooks
    setup_git_hooks
    
    # Create development configuration
    create_dev_config
    
    # Setup Android development (optional)
    setup_android_dev
    
    # Run initial build and tests
    run_initial_tests
    
    print_status "Development environment setup complete!"
    print_info ""
    print_info "Next steps:"
    print_info "1. Run './build.sh' to build the project"
    print_info "2. Run './test.sh' to run tests"
    print_info "3. Use your favorite IDE/editor with the project"
    print_info "4. Follow the testing rules in TESTING_RULES.md"
    print_info ""
    print_info "Happy coding! 🚀"
}

main "$@"