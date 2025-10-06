# STRAP — STRAP Trims, Rejoins And Parses

[![CI](https://github.com/murapadev/strap/workflows/CI/badge.svg)](https://github.com/murapadev/strap/actions)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C](https://img.shields.io/badge/language-C-blue.svg)](<https://en.wikipedia.org/wiki/C_(programming_language)>)

A small, efficient C library providing missing utilities for safe and comfortable string manipulation and time helpers. Designed to be lightweight, portable, and easy to integrate into C projects.

## 📚 Documentation

- **[📖 Full Documentation](../../wiki)** - Complete API reference, examples, and guides
- **[🚀 Quick Start Guide](../../wiki/Quick-Start)** - Get up and running in minutes
- **[📋 API Reference](../../wiki/API-Reference)** - Detailed function documentation
- **[💡 Examples](../../wiki/Examples)** - Code examples and use cases
- **[🧰 Cookbook](../../wiki/Cookbook)** - Practical recipes for common tasks

## 🚀 Quick Start

```bash
# Clone and build
git clone https://github.com/murapadev/strap.git
cd strap
make

# Install (optional)
sudo make install

# Benchmarks (optional)
make bench
./benchmarks/strap_bench
```

### Windows (MSVC + CMake)

```powershell
# From a Visual Studio Developer Command Prompt
git clone https://github.com/murapadev/strap.git
cd strap
scripts\build_windows_msvc.bat Release
```

The helper script configures the Visual Studio generator via CMake, builds the static library, and runs the unit tests. Adjust the generator string or build type inside the script to match your environment.

## 🔔 What's New in v0.2

- Faster joins and file reads through tighter allocation strategies
- New helpers: `strstartswith`, `strendswith`, and `strreplace`
- Unified error reporting via `strap_last_error()` and `strap_error_string()`
- Scriptable micro-benchmarks with `make bench`

### Basic Usage

```c
#include <stdio.h>
#include "strap.h"

int main(void) {
    // Safe line reading
    char *line = afgets(stdin);
    if (line) {
        char *trimmed = strtrim(line);
        printf("Trimmed: '%s'\n", trimmed);
        free(line);
        free(trimmed);
    }

    // String joining
    const char *parts[] = {"Hello", "world", "from", "STRAP"};
    char *joined = strjoin(parts, 4, " ");
    printf("%s\n", joined);
    free(joined);

    return 0;
}
```

> 📚 **See the [Wiki](../../wiki) for complete documentation, examples, and advanced usage.**

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## ⚠️ Error Handling

All STRAP APIs set a thread-local error code. On failure, query the cause:

```c
char *trimmed = strtrim(raw_line);
if (!trimmed) {
    fprintf(stderr, "STRAP error: %s\n", strap_error_string(strap_last_error()));
    return EXIT_FAILURE;
}
```

Call `strap_clear_error()` when you want to reset the status manually.

## 🗺️ Roadmap

### v0.2 (Current) ✅

- [x] Performance optimizations
- [x] Additional string utilities
- [x] Enhanced error handling
- [x] Benchmark suite

### v0.3 (In Progress)

- [x] Extended locale-aware helpers
- [x] Optional arena allocator for transient strings
- [x] Timezone-aware time helpers

### v0.4 (Upcoming)

- [x] Additional performance optimizations
  - [x] SIMD-accelerated paths for common hot functions (`strtrim`, `strjoin`)
  - [x] Streaming-friendly buffering for `afgets` and file helpers (`strap_line_buffer_read`)
- [x] Extended string manipulation utilities
  - [x] High-level split helpers with delimiter limits and predicates (`strsplit_limit`, `strsplit_predicate`)
  - [x] Case-insensitive comparison helpers (`strap_strcasecmp`, `strcaseeq`)
- [x] Cross-platform improvements
  - [x] Official Windows/MSVC build scripts and CI coverage
  - [x] Locale/timezone shims for BSD and musl-targeted systems
- [x] Enhanced documentation
  - [x] Cookbook-style guides for common patterns (log parsing, CSV wrangling)
  - [x] API reference refresh with usage notes and complexity details

## 📄 License

MIT Licensed - see [LICENSE](LICENSE) for details.

---

**STRAP** - Making C string and time operations safer and more convenient.
