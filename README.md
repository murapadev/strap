# STRAP — STRAP Trims, Rejoins And Parses

[![CI](https://github.com/murapadev/strap/workflows/CI/badge.svg)](https://github.com/murapadev/strap/actions)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C](https://img.shields.io/badge/language-C-blue.svg)](<https://en.wikipedia.org/wiki/C_(programming_language)>)

A small, efficient C library providing missing utilities for safe and comfortable string manipulation and time helpers. Designed to be lightweight, portable, and easy to integrate into C projects.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [API Reference](#api-reference)
- [Examples](#examples)
- [Building and Testing](#building-and-testing)
- [Contributing](#contributing)
- [Roadmap](#roadmap)
- [License](#license)

## Features

- **Safe File I/O**: Robust functions for reading lines and entire files with automatic memory management
- **String Utilities**: Powerful string joining, trimming, and manipulation functions
- **Time Operations**: Convenient utilities for `struct timeval` arithmetic and conversions
- **Memory Safe**: All functions handle memory allocation and deallocation appropriately
- **Cross-Platform**: Tested on Linux, macOS, and Windows
- **Minimal Dependencies**: Only standard C libraries required
- **Well Tested**: Comprehensive unit tests included

## Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/murapadev/strap.git
cd strap

# Build the library
make

# Install system-wide (requires sudo)
sudo make install
```

This installs the header file to `/usr/local/include/strap.h` and the library to `/usr/local/lib/libstrap.a`.

### Integration

To use STRAP in your project:

1. Include the header: `#include <strap.h>` (or `#include "strap.h"` if local)
2. Link the library: Add `-lstrap` to your compiler flags
3. Ensure the library path is included if installed in a non-standard location

## API Reference

### Safe Reading Functions

#### `char *afgets(FILE *f)`

Reads a complete line from the file stream, automatically handling memory allocation.

- **Parameters**: `f` - File stream to read from
- **Returns**: Malloc-allocated buffer containing the line (without newline), or NULL on error/EOF
- **Note**: Caller is responsible for freeing the returned buffer

#### `char *afread(FILE *f, size_t *out_len)`

Reads the entire contents of a file into memory.

- **Parameters**:
  - `f` - File stream to read from
  - `out_len` - Pointer to store the length of the read data (can be NULL)
- **Returns**: Malloc-allocated buffer containing file contents, or NULL on error
- **Note**: Caller is responsible for freeing the returned buffer

### String Manipulation Functions

#### `char *strjoin(const char **parts, size_t nparts, const char *sep)`

Joins multiple strings with a separator.

- **Parameters**:
  - `parts` - Array of strings to join
  - `nparts` - Number of strings in the array
  - `sep` - Separator string (can be NULL for no separator)
- **Returns**: Malloc-allocated buffer containing the joined string
- **Note**: Caller is responsible for freeing the returned buffer

#### `char *strjoin_va(const char *sep, ...)`

Variable argument version of `strjoin`.

- **Parameters**:
  - `sep` - Separator string
  - `...` - Variable number of string arguments, terminated by NULL
- **Returns**: Malloc-allocated buffer containing the joined string
- **Note**: Caller is responsible for freeing the returned buffer

#### `char *strtrim(const char *s)`

Trims whitespace from both ends of a string.

- **Parameters**: `s` - Input string
- **Returns**: Malloc-allocated buffer containing the trimmed string
- **Note**: Trims spaces, tabs, newlines, and carriage returns

#### `void strtrim_inplace(char *s)`

Trims whitespace from both ends of a string in-place.

- **Parameters**: `s` - String to modify (must be mutable)
- **Note**: Modifies the input string directly

### Time Utility Functions

#### `struct timeval timeval_add(struct timeval a, struct timeval b)`

Adds two `struct timeval` values.

- **Parameters**: `a`, `b` - Time values to add
- **Returns**: Result of addition with proper carry handling

#### `struct timeval timeval_sub(struct timeval a, struct timeval b)`

Subtracts two `struct timeval` values.

- **Parameters**: `a`, `b` - Time values (a - b)
- **Returns**: Result of subtraction with proper borrow handling

#### `double timeval_to_seconds(struct timeval t)`

Converts a `struct timeval` to seconds as a double.

- **Parameters**: `t` - Time value to convert
- **Returns**: Time in seconds with microsecond precision

## Examples

See the [`examples/`](examples/) directory for complete working examples:

- [`readline_example.c`](examples/readline_example.c) - Demonstrates safe line reading and trimming
- [`join_example.c`](examples/join_example.c) - Shows string joining functionality

### Basic Usage

```c
#include <stdio.h>
#include <stdlib.h>
#include "strap.h"

int main(void) {
    // Read a line safely
    char *line = afgets(stdin);
    if (line) {
        // Trim whitespace
        char *trimmed = strtrim(line);
        printf("Trimmed input: '%s'\n", trimmed);

        // Clean up
        free(line);
        free(trimmed);
    }

    // Join strings
    const char *parts[] = {"Hello", "world", "from", "STRAP"};
    char *joined = strjoin(parts, 4, " ");
    if (joined) {
        printf("%s\n", joined);
        free(joined);
    }

    return 0;
}
```

## Building and Testing

### Building

```bash
make          # Build the library
make clean    # Clean build artifacts
make install  # Install system-wide
```

### Running Tests

```bash
cd tests
gcc -I.. -L.. -o test_strap test_strap.c -lstrap
./test_strap
```

### Requirements

- C99 compatible compiler (gcc, clang, etc.)
- GNU Make
- Standard C library

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:

- Reporting bugs
- Requesting features
- Submitting pull requests
- Development setup

## Roadmap

### v0.1 (Current) ✅

- [x] Implement core functions: `afgets`, `afread`, `strtrim`, `strtrim_inplace`, `strjoin`
- [x] Add `strjoin_va` for variable argument joining
- [x] Implement `timeval_*` helper functions
- [x] Comprehensive unit tests
- [x] CI/CD with GitHub Actions (Linux/macOS/Windows)
- [x] Complete documentation and examples
- [x] Contribution guidelines and issue templates

### v0.2 (Upcoming)

- [ ] Performance optimizations for large file handling
- [ ] Additional string utilities (split, replace, etc.)
- [ ] Error handling improvements with custom error codes
- [ ] Windows-specific optimizations
- [ ] Benchmark suite
- [ ] API documentation generation (Doxygen)

### Future Releases

- [ ] Shared library support (.so/.dll)
- [ ] Package manager distributions (apt, brew, etc.)
- [ ] Extended platform support (embedded systems)
- [ ] Localization support
- [ ] Plugin/extension system

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**STRAP** - Making C string and time operations safer and more convenient.
