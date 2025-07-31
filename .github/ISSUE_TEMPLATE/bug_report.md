---
name: Bug report
about: Create a report to help us improve
title: "[BUG] "
labels: bug
assignees: ''

---

## Bug Description
A clear and concise description of what the bug is.

## Environment
- **OS**: [e.g. Ubuntu 22.04, macOS 13.0, etc.]
- **Compiler**: [e.g. GCC 12, Clang 15, etc.]
- **Kagome C++ Version**: [e.g. 1.0.0]
- **Rspamd Version** (if applicable): [e.g. 3.6]

## Steps to Reproduce
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

## Expected Behavior
A clear and concise description of what you expected to happen.

## Actual Behavior
A clear and concise description of what actually happened.

## Sample Input
If applicable, provide sample Japanese text that triggers the bug:
```
すもももももももものうち
```

## Error Messages
```
Paste any error messages, stack traces, or logs here
```

## Configuration
If using with Rspamd, include relevant configuration:
```ucl
custom_tokenizers {
    kagome {
        enabled = true;
        path = "/path/to/kagome_rspamd_tokenizer.so";
        priority = 60.0;
    }
}
```

## Additional Context
Add any other context about the problem here.

## Attempted Solutions
What have you tried to resolve the issue?

## Impact
- [ ] Blocks Rspamd functionality
- [ ] Causes incorrect tokenization
- [ ] Performance issue
- [ ] Memory leak
- [ ] Crash/segmentation fault
- [ ] Other: ___________