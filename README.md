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

### v0.3 (Planned)

- [ ] Extended locale-aware helpers
- [ ] Optional arena allocator for transient strings
- [ ] Timezone-aware time helpers

## 📄 License

MIT Licensed - see [LICENSE](LICENSE) for details.

---

**STRAP** - Making C string and time operations safer and more convenient.
