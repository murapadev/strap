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
```

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

## 🗺️ Roadmap

### v0.1 (Current) ✅

- [x] Core string and time utilities
- [x] Cross-platform compatibility
- [x] Comprehensive testing
- [x] CI/CD pipeline

### v0.2 (Upcoming)

- [ ] Performance optimizations
- [ ] Additional string utilities
- [ ] Enhanced error handling
- [ ] Benchmark suite

## 📄 License

MIT Licensed - see [LICENSE](LICENSE) for details.

---

**STRAP** - Making C string and time operations safer and more convenient.
