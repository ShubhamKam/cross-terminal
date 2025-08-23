# Testing Rules and Guidelines

## Mandatory Testing Requirements

**RULE 1: No feature delivery without testing**
- Every feature MUST pass unit tests before implementation completion
- Integration tests MUST be written for cross-component interactions  
- System tests MUST verify end-to-end functionality
- All tests MUST pass before feature is considered complete

**RULE 2: Testing pyramid compliance**
- 70% Unit tests (fast, isolated, comprehensive)
- 20% Integration tests (component interactions)
- 10% System tests (full application scenarios)

**RULE 3: Platform testing requirements**
- Each platform implementation MUST have platform-specific tests
- Cross-platform features MUST be tested on all target platforms
- Hardware features MUST be tested on actual devices when possible

## Testing Framework Structure

```
tests/
├── unit/           # Unit tests for individual components
├── integration/    # Integration tests for component interactions
├── system/         # End-to-end system tests
├── platform/       # Platform-specific tests
├── hardware/       # Hardware control tests
└── mocks/          # Mock implementations for testing
```

## Test Execution Rules

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

**Before release:**
1. Full test suite passes on all platforms
2. Hardware tests pass on real devices
3. Load testing completed
4. Security testing completed

## Automated Testing Integration

- GitHub Actions CI/CD pipeline
- Pre-commit hooks for test execution
- Automated test reporting
- Coverage tracking (minimum 80% code coverage)
- Performance regression detection

## Test Development Standards

- Tests MUST be deterministic and repeatable
- Tests MUST clean up after themselves
- Tests MUST not depend on external services
- Tests MUST use mocks for hardware when unavailable
- Tests MUST include both positive and negative cases