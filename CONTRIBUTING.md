# Contributing to STRAP

Thank you for your interest in contributing to STRAP! We welcome contributions from the community.

## How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Commit your changes (`git commit -m 'Add some amazing feature'`)
7. Push to the branch (`git push origin feature/amazing-feature`)
8. Open a Pull Request

## Development Setup

```bash
git clone https://github.com/murapadev/strap.git
cd strap
make
# Run tests
cd tests
gcc -I.. -L.. -o test_strap test_strap.c -lstrap
./test_strap
```

## Code Style

- Use consistent indentation (4 spaces)
- Add comments for complex logic
- Follow C99 standards where possible
- Write clear, descriptive function and variable names

## Reporting Issues

Please use the bug report template when creating issues. Include as much detail as possible to help us reproduce and fix the issue.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
