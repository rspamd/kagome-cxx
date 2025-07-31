# GitHub Actions CI/CD Setup

This document describes the GitHub Actions workflows configured for the Kagome C++ project.

## Overview

The project includes a comprehensive CI/CD pipeline that ensures code quality, builds packages, and automates releases. All workflows are located in `.github/workflows/`.

## Workflows

### 1. CI Pipeline (`ci.yml`)

**Triggers**: Push/PR to main/master/develop branches

**Features**:
- **Multi-platform builds**: Ubuntu 20.04/22.04, macOS latest
- **Compiler matrix**: GCC 11/12, Clang 14/15
- **Build configurations**: Release builds with comprehensive testing
- **Sanitizer testing**: AddressSanitizer, ThreadSanitizer, UndefinedBehaviorSanitizer
- **Static analysis**: clang-tidy, cppcheck, clang-format validation
- **Package building**: Debian packages with installation testing
- **Performance benchmarking**: Speed and memory usage measurements

**Jobs**:
1. `build-linux` - Multi-compiler Linux builds
2. `build-macos` - macOS universal binaries
3. `build-sanitizers` - Memory safety validation
4. `static-analysis` - Code quality checks
5. `package-build` - Debian package creation and testing
6. `performance-test` - Performance benchmarking

### 2. Release Pipeline (`release.yml`)

**Triggers**: Git tags matching `v*` pattern

**Features**:
- **Automated releases**: Creates GitHub releases with detailed notes
- **Multi-platform binaries**: Linux (Ubuntu 20.04/22.04), macOS (Universal)
- **Debian packages**: Complete package suite (.deb files)
- **Checksums**: SHA256 verification for all artifacts
- **Version management**: Automatic version updates in build files

**Jobs**:
1. `create-release` - GitHub release creation with formatted notes
2. `build-release-packages` - Debian package building
3. `build-release-binaries` - Cross-platform binary compilation
4. `upload-to-release` - Artifact upload to GitHub releases

### 3. Security Analysis (`codeql.yml`)

**Triggers**: Push/PR to main branches, weekly schedule (Fridays)

**Features**:
- **CodeQL analysis**: Advanced security and quality scanning
- **Vulnerability detection**: Automated security issue identification
- **Extended queries**: Security and quality rule sets

### 4. Documentation (`docs.yml`)

**Triggers**: Changes to markdown files or documentation

**Features**:
- **Link validation**: Broken link detection in documentation
- **Markdown linting**: Consistent formatting enforcement
- **Package documentation**: Required file structure validation
- **README structure**: Section validation for completeness

## Configuration Files

### Issue Templates

1. **Bug Report** (`.github/ISSUE_TEMPLATE/bug_report.md`)
   - Environment information collection
   - Reproduction steps
   - Sample input/output
   - Impact assessment

2. **Feature Request** (`.github/ISSUE_TEMPLATE/feature_request.md`)
   - Use case description
   - Implementation details
   - Compatibility considerations
   - Priority assessment

### Pull Request Template

**PR Template** (`.github/PULL_REQUEST_TEMPLATE.md`)
- Change type classification
- Testing checklist
- Documentation updates
- Performance impact assessment
- Rspamd integration validation

### Automation

1. **Dependabot** (`.github/dependabot.yml`)
   - Weekly GitHub Actions updates
   - Monthly Git submodule updates
   - Automated dependency PRs

2. **Link Checker Config** (`.github/markdown-link-check.json`)
   - URL pattern ignoring
   - Header configuration
   - Timeout and retry settings

## Badges and Status

The main README includes CI status badges:
```markdown
[![CI](https://github.com/rspamd/kagome-cxx/workflows/CI/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/ci.yml)
[![CodeQL](https://github.com/rspamd/kagome-cxx/workflows/CodeQL/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/codeql.yml)
[![Release](https://github.com/rspamd/kagome-cxx/workflows/Release/badge.svg)](https://github.com/rspamd/kagome-cxx/actions/workflows/release.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
```

## Release Process

### Automatic Release (Recommended)

1. **Tag creation**: `git tag -a v1.0.0 -m "Release v1.0.0"`
2. **Push tag**: `git push origin v1.0.0`
3. **Automated workflow**: 
   - Creates GitHub release
   - Builds packages for all platforms
   - Uploads artifacts with checksums
   - Generates release notes

### Manual Release Steps

If needed for troubleshooting:

1. **Build packages locally**:
   ```bash
   dpkg-buildpackage -us -uc
   ```

2. **Create release manually**:
   - Go to GitHub Releases
   - Create new release from tag
   - Upload artifacts manually

## CI Performance

### Build Times
- **Linux builds**: ~5-10 minutes per job
- **macOS builds**: ~8-15 minutes
- **Sanitizer builds**: ~10-20 minutes
- **Package builds**: ~15-25 minutes

### Resource Usage
- **Parallel jobs**: Up to 20 concurrent jobs
- **Artifact storage**: 30-day retention for release artifacts
- **Cache usage**: CMake and compiler cache for faster rebuilds

## Debugging CI Issues

### Common Issues

1. **Dependency installation failures**:
   - Check package availability on target OS
   - Verify package names and versions
   - Review CMake find_package calls

2. **Compiler errors**:
   - Check C++23 feature usage compatibility
   - Verify header includes and namespaces
   - Review template instantiation issues

3. **Test failures**:
   - Check dictionary file availability
   - Verify test environment setup
   - Review timeout settings for sanitizer builds

4. **Package build failures**:
   - Check debian/control dependencies
   - Verify file installation paths
   - Review package conflicts

### Debugging Commands

```bash
# Local CI simulation
act -j build-linux  # Requires 'act' tool

# Package testing
dpkg-buildpackage -us -uc
sudo dpkg -i ../kagome-*.deb

# Static analysis
clang-format --dry-run --Werror src/*.cpp
clang-tidy -p build src/*.cpp
```

## CI Metrics and Monitoring

### Success Rates
- Target: >95% successful builds
- Monitor via GitHub Actions dashboard
- Track failure patterns and common issues

### Performance Tracking
- Build time trends
- Test execution time
- Package size monitoring
- Memory usage benchmarks

### Quality Metrics
- Code coverage (planned)
- Static analysis warnings
- Security vulnerability counts
- Documentation completeness

## Future Enhancements

### Planned Improvements
1. **Code coverage reporting**: Integration with codecov.io
2. **Fuzz testing**: OSS-Fuzz integration for security testing
3. **Cross-compilation**: ARM64, RISC-V support
4. **Container builds**: Docker image creation and publishing
5. **Performance regression testing**: Automated performance monitoring

### Integration Opportunities
1. **Rspamd CI integration**: Test with actual Rspamd versions
2. **Distribution packaging**: Integration with distro package systems
3. **Documentation deployment**: Automated docs site generation
4. **Notification systems**: Slack/Discord integration for failures

## Maintenance

### Regular Tasks
- **Monthly**: Review and update GitHub Actions versions
- **Quarterly**: Update compiler versions and OS images
- **Annually**: Review and update CI strategy

### Monitoring
- **GitHub Actions usage**: Monitor billing and limits
- **Failure notifications**: Set up alerts for critical failures
- **Performance trends**: Track build time and resource usage

The CI/CD setup provides comprehensive quality assurance and automated release management, ensuring the project maintains high standards while enabling efficient collaboration and distribution.