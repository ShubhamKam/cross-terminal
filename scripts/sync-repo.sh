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
    echo -e "${GREEN}[SYNC]${NC} $1"
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

# Configuration
GITHUB_USERNAME=""
REPO_NAME="cross-terminal"
COMMIT_MESSAGE=""
AUTO_PUSH=false
RUN_TESTS=true
FORCE_PUSH=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --username)
            GITHUB_USERNAME="$2"
            shift 2
            ;;
        --repo)
            REPO_NAME="$2"
            shift 2
            ;;
        --message|-m)
            COMMIT_MESSAGE="$2"
            shift 2
            ;;
        --auto-push)
            AUTO_PUSH=true
            shift
            ;;
        --no-tests)
            RUN_TESTS=false
            shift
            ;;
        --force)
            FORCE_PUSH=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --username USER    GitHub username"
            echo "  --repo REPO        Repository name (default: cross-terminal)"
            echo "  --message MSG      Commit message"
            echo "  --auto-push        Automatically push to remote"
            echo "  --no-tests         Skip running tests before sync"
            echo "  --force            Force push (use with caution)"
            echo "  -h, --help         Show this help message"
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Get GitHub username if not provided
get_github_username() {
    if [ -z "$GITHUB_USERNAME" ]; then
        # Try to get from git config
        GITHUB_USERNAME=$(git config --global user.name 2>/dev/null || echo "")
        
        if [ -z "$GITHUB_USERNAME" ]; then
            read -p "Enter your GitHub username: " GITHUB_USERNAME
        fi
    fi
    
    if [ -z "$GITHUB_USERNAME" ]; then
        print_error "GitHub username is required"
        exit 1
    fi
}

# Initialize Git repository if needed
init_git_repo() {
    cd "$PROJECT_DIR"
    
    if [ ! -d ".git" ]; then
        print_status "Initializing Git repository..."
        git init
        git branch -M main
    fi
    
    # Set user config if not set
    if [ -z "$(git config user.name)" ]; then
        git config user.name "$GITHUB_USERNAME"
    fi
    
    if [ -z "$(git config user.email)" ]; then
        read -p "Enter your GitHub email: " EMAIL
        git config user.email "$EMAIL"
    fi
}

# Check if remote repository exists
check_remote_repo() {
    local remote_url="https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
    
    if git ls-remote "$remote_url" >/dev/null 2>&1; then
        print_info "Remote repository exists: $remote_url"
        return 0
    else
        print_warning "Remote repository does not exist: $remote_url"
        return 1
    fi
}

# Create GitHub repository using gh CLI
create_github_repo() {
    print_status "Creating GitHub repository..."
    
    if command -v gh >/dev/null 2>&1; then
        # Use GitHub CLI
        gh repo create "$REPO_NAME" \
            --description "Cross-platform terminal with hardware control capabilities" \
            --public \
            --clone=false
        
        print_status "Repository created successfully"
    else
        print_warning "GitHub CLI (gh) not found"
        print_info "Please create the repository manually at:"
        print_info "https://github.com/new"
        print_info "Repository name: $REPO_NAME"
        print_info "Then press Enter to continue..."
        read
    fi
}

# Add remote origin if not exists
setup_remote() {
    local remote_url="https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
    
    if ! git remote get-url origin >/dev/null 2>&1; then
        print_status "Adding remote origin..."
        git remote add origin "$remote_url"
    else
        # Update remote URL if needed
        current_url=$(git remote get-url origin)
        if [ "$current_url" != "$remote_url" ]; then
            print_status "Updating remote origin URL..."
            git remote set-url origin "$remote_url"
        fi
    fi
}

# Run tests before syncing
run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        print_status "Running tests before sync..."
        
        if [ -x "./test.sh" ]; then
            if ./test.sh --fast; then
                print_status "Tests passed"
            else
                print_error "Tests failed. Aborting sync."
                print_info "Use --no-tests to skip test validation"
                exit 1
            fi
        else
            print_warning "Test script not found, skipping tests"
        fi
    fi
}

# Generate commit message if not provided
generate_commit_message() {
    if [ -z "$COMMIT_MESSAGE" ]; then
        # Generate based on changes
        local added_files=$(git diff --cached --name-only --diff-filter=A | wc -l)
        local modified_files=$(git diff --cached --name-only --diff-filter=M | wc -l)
        local deleted_files=$(git diff --cached --name-only --diff-filter=D | wc -l)
        
        if [ "$added_files" -gt 0 ] && [ "$modified_files" -eq 0 ] && [ "$deleted_files" -eq 0 ]; then
            COMMIT_MESSAGE="feat: add initial project structure and implementation"
        elif [ "$modified_files" -gt 0 ]; then
            COMMIT_MESSAGE="feat: update implementation and add new features"
        else
            COMMIT_MESSAGE="chore: update project files"
        fi
        
        print_info "Generated commit message: $COMMIT_MESSAGE"
    fi
}

# Stage and commit changes
commit_changes() {
    print_status "Staging changes..."
    
    # Add all files except those in .gitignore
    git add .
    
    # Check if there are changes to commit
    if git diff --cached --quiet; then
        print_warning "No changes to commit"
        return 1
    fi
    
    generate_commit_message
    
    print_status "Committing changes..."
    git commit -m "$COMMIT_MESSAGE

🤖 Generated with Claude Code

Co-Authored-By: Claude <noreply@anthropic.com>"
    
    return 0
}

# Push to remote repository
push_to_remote() {
    print_status "Pushing to remote repository..."
    
    if [ "$FORCE_PUSH" = true ]; then
        git push --force-with-lease origin main
    else
        # Try normal push first
        if git push origin main 2>/dev/null; then
            print_status "Push successful"
        else
            # If push fails, try with upstream
            print_status "Setting upstream and pushing..."
            git push -u origin main
        fi
    fi
}

# Create or update GitHub repository metadata
update_repo_metadata() {
    if command -v gh >/dev/null 2>&1; then
        print_status "Updating repository metadata..."
        
        # Update description and topics
        gh repo edit "$GITHUB_USERNAME/$REPO_NAME" \
            --description "Cross-platform terminal emulator with hardware control capabilities for Android, iOS, macOS, Windows, and Linux" \
            --add-topic "terminal" \
            --add-topic "cross-platform" \
            --add-topic "cpp" \
            --add-topic "android" \
            --add-topic "macos" \
            --add-topic "hardware-control" \
            --add-topic "iot" || true
    fi
}

# Setup automatic sync
setup_automatic_sync() {
    print_status "Setting up automatic repository sync..."
    
    # Create a sync configuration file
    cat > .sync-config << EOF
# Cross-Terminal Repository Sync Configuration
GITHUB_USERNAME="$GITHUB_USERNAME"
REPO_NAME="$REPO_NAME"
AUTO_SYNC_ENABLED=true
SYNC_INTERVAL=daily
RUN_TESTS_BEFORE_SYNC=true
EOF
    
    # Create a sync hook script
    cat > scripts/auto-sync.sh << 'EOF'
#!/bin/bash

# Automatic sync script for Cross-Terminal project
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

# Source configuration
if [ -f ".sync-config" ]; then
    source .sync-config
else
    echo "Sync configuration not found"
    exit 1
fi

if [ "$AUTO_SYNC_ENABLED" = true ]; then
    echo "Running automatic sync..."
    ./scripts/sync-repo.sh --auto-push --message "chore: automatic periodic sync"
fi
EOF
    
    chmod +x scripts/auto-sync.sh
    
    print_info "Automatic sync configured"
    print_info "To enable periodic sync, add this to your crontab:"
    print_info "0 0 * * * cd $PROJECT_DIR && ./scripts/auto-sync.sh"
}

# Main execution
main() {
    print_status "Starting repository synchronization..."
    
    cd "$PROJECT_DIR"
    
    get_github_username
    init_git_repo
    
    # Check if remote repo exists, create if needed
    if ! check_remote_repo; then
        create_github_repo
    fi
    
    setup_remote
    run_tests
    
    if commit_changes; then
        if [ "$AUTO_PUSH" = true ]; then
            push_to_remote
            update_repo_metadata
            print_status "Repository synchronized successfully!"
        else
            read -p "Push changes to GitHub? (y/n): " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                push_to_remote
                update_repo_metadata
                print_status "Repository synchronized successfully!"
            else
                print_info "Changes committed locally. Push manually with: git push origin main"
            fi
        fi
        
        setup_automatic_sync
        
    else
        print_info "No changes to sync"
    fi
    
    print_info ""
    print_info "Repository URL: https://github.com/$GITHUB_USERNAME/$REPO_NAME"
    print_info "Clone command: git clone https://github.com/$GITHUB_USERNAME/$REPO_NAME.git"
    print_info ""
    print_info "Future syncs: ./scripts/sync-repo.sh --auto-push"
}

main "$@"