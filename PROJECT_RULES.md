# Cross-Terminal Project Rules and Guidelines

## Repository Management Rules

### RULE 1: Automatic Repository Synchronization
- The GitHub repository MUST be updated periodically as code is added to the project
- Use `./scripts/sync-repo.sh --auto-push` for automatic synchronization
- All changes MUST be committed with meaningful commit messages following conventional commit format
- Repository sync MUST include running tests before pushing (unless explicitly skipped)

### RULE 2: Commit Standards
- All commits MUST follow conventional commit format: `type(scope): description`
- Commit types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`
- Each commit MUST be signed with Claude Code attribution
- Commit messages MUST be descriptive and under 50 characters for the subject line

### RULE 3: Branch Management
- Main development happens on `main` branch
- Feature branches MUST be used for significant new features
- All branches MUST pass CI tests before merging
- No direct pushes to main branch without proper testing

## Testing Rules (MANDATORY)

### RULE 4: No Code Delivery Without Testing
- Every feature MUST pass unit tests before implementation completion
- Integration tests MUST be written for cross-component interactions
- System tests MUST verify end-to-end functionality
- All tests MUST pass before any code is considered complete or committed

### RULE 5: Test Coverage Requirements
- Minimum 80% code coverage for all production code
- 100% of public APIs MUST be covered by tests
- Platform-specific code MUST have platform-specific tests
- Hardware control features MUST be tested on actual devices when possible

### RULE 6: Test Execution Rules
**Before every commit:**
1. Run all unit tests - MUST pass 100%
2. Run integration tests for affected components
3. Run platform-specific tests for target platform

**Before feature completion:**
1. All unit tests pass
2. All integration tests pass
3. System tests demonstrate feature works end-to-end
4. Performance benchmarks meet requirements
5. Memory leak detection passes

## Development Workflow Rules

### RULE 7: Build and Test Pipeline
- Every code change MUST be buildable on all target platforms
- CI/CD pipeline MUST pass before any merge
- Pre-commit hooks MUST be used and passing
- Code formatting MUST follow project standards (clang-format)

### RULE 8: Platform Implementation Rules
- Android platform implementations MUST be tested on real devices
- macOS platform implementations MUST be tested on macOS systems
- Cross-platform features MUST work consistently across all platforms
- Hardware control features MUST gracefully handle missing permissions

### RULE 9: Documentation Requirements
- All public APIs MUST be documented
- README files MUST be kept up-to-date with build instructions
- Architecture decisions MUST be documented
- Testing procedures MUST be clearly documented

## Code Quality Rules

### RULE 10: Security Standards
- No secrets or API keys MUST be committed to repository
- All hardware access MUST request appropriate permissions
- Input validation MUST be performed on all external data
- Security scanning MUST pass in CI pipeline

### RULE 11: Performance Standards
- Memory leaks are not acceptable in production code
- Performance benchmarks MUST not regress
- Resource usage MUST be monitored and optimized
- Hardware control operations MUST not block the UI thread

### RULE 12: Error Handling
- All system calls MUST have proper error handling
- Hardware operations MUST fail gracefully when permissions are denied
- Network operations MUST handle connectivity issues
- File operations MUST handle permission and space issues

## Automation Rules

### RULE 13: Continuous Integration
- GitHub Actions CI MUST run on all pull requests
- All platforms MUST be tested in CI environment
- Security scanning MUST be automated
- Performance regression detection MUST be automated

### RULE 14: Periodic Tasks
- Repository synchronization MUST happen automatically
- Dependencies MUST be regularly updated and tested
- Security vulnerabilities MUST be automatically scanned
- Performance benchmarks MUST be run periodically

## Project Maintenance Rules

### RULE 15: Version Management
- Semantic versioning MUST be followed
- Release notes MUST be maintained
- Breaking changes MUST be clearly documented
- Migration guides MUST be provided for major updates

### RULE 16: Issue and Bug Tracking
- All bugs MUST be tracked in GitHub issues
- Feature requests MUST be documented and prioritized
- Security issues MUST be handled with appropriate urgency
- Performance issues MUST be benchmarked and tracked

## Development Environment Rules

### RULE 17: Environment Setup
- Development environment setup MUST be automated via `scripts/setup-dev.sh`
- All required dependencies MUST be documented
- Environment consistency MUST be maintained across developers
- Docker/container support SHOULD be provided for consistent builds

### RULE 18: IDE and Tooling
- VS Code configuration MUST be provided and maintained
- Code formatting MUST be automated and consistent
- Static analysis tools MUST be integrated and passing
- Debugging configurations MUST be provided for all platforms

## Enforcement

These rules are MANDATORY and will be enforced through:
- Pre-commit hooks
- CI/CD pipeline checks
- Code review requirements
- Automated testing
- Repository sync automation

Violations of these rules will result in:
- Failed CI builds
- Rejected pull requests
- Required fixes before merge
- Repository sync failures

## Quick Reference Commands

```bash
# Setup development environment
./scripts/setup-dev.sh

# Build and test
./build.sh && ./test.sh

# Sync to GitHub repository
./scripts/sync-repo.sh --auto-push

# Run specific tests
./test.sh --unit              # Unit tests only
./test.sh --platform          # Platform tests
./test.sh --coverage          # With coverage

# Platform-specific testing
./android/test-android.sh     # Android tests
./macos/test-macos.sh        # macOS tests
```