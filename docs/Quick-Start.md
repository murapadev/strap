# Quick Start Guide

## Installation

### Prerequisites

- C99 compatible compiler (gcc, clang, MSVC)
- GNU Make (Linux/macOS) or compatible build system
- Standard C library

### Building from Source

1. **Clone the repository:**

   ```bash
   git clone https://github.com/murapadev/strap.git
   cd strap
   ```

2. **Build the library:**

   ```bash
   make
   ```

   This creates `libstrap.a` in the project directory.

3. **Install system-wide (optional):**
   ```bash
   sudo make install
   ```
   This installs:
   - Header: `/usr/local/include/strap.h`
   - Library: `/usr/local/lib/libstrap.a`

### Integration into Your Project

#### Method 1: System Installation

If you installed STRAP system-wide:

```c
#include <strap.h>
```

Compile with:

```bash
gcc -o myprogram myprogram.c -lstrap
```

#### Method 2: Local Include

If using STRAP locally in your project:

```c
#include "path/to/strap.h"
```

Compile with:

```bash
gcc -o myprogram myprogram.c -L/path/to/strap -lstrap
```

## Your First STRAP Program

Create a file called `hello_strap.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include "strap.h"

int main(void) {
    printf("Enter your name: ");

    // Safe line reading
    char *name = afgets(stdin);
    if (!name) {
        fprintf(stderr, "Error reading input\n");
        return 1;
    }

    // Trim whitespace
    char *trimmed_name = strtrim(name);

    // Create greeting
    char *greeting = strjoin_va(" ", "Hello,", trimmed_name, "!", NULL);

    printf("%s\n", greeting);

    // Cleanup
    free(name);
    free(trimmed_name);
    free(greeting);

    return 0;
}
```

Compile and run:

```bash
gcc -I. -L. -o hello_strap hello_strap.c -lstrap
./hello_strap
```

## Common Use Cases

### 1. Configuration File Reading

```c
#include <stdio.h>
#include "strap.h"

void read_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;

    char *line;
    while ((line = afgets(fp)) != NULL) {
        char *trimmed = strtrim(line);
        if (trimmed[0] != '#' && trimmed[0] != '\0') {
            printf("Config line: %s\n", trimmed);
        }
        free(line);
        free(trimmed);
    }

    fclose(fp);
}
```

### 2. Path Construction

```c
#include "strap.h"

char *build_path(const char *base, const char *file) {
    return strjoin_va("/", base, file, NULL);
}

int main(void) {
    char *config_path = build_path("/etc/myapp", "config.ini");
    printf("Config at: %s\n", config_path);
    free(config_path);
    return 0;
}
```

### 3. Time Measurements

```c
#include <sys/time.h>
#include "strap.h"

void benchmark_operation(void) {
    struct timeval start, end, diff;

    gettimeofday(&start, NULL);

    // Your operation here
    for (int i = 0; i < 1000000; i++) {
        // Some work
    }

    gettimeofday(&end, NULL);
    diff = timeval_sub(end, start);

    printf("Operation took %.6f seconds\n", timeval_to_seconds(diff));
}
```

## Testing Your Installation

Run the included tests to verify everything works:

```bash
cd tests
gcc -I.. -L.. -o test_strap test_strap.c -lstrap
./test_strap
```

You should see:

```
strtrim tests passed
strtrim_inplace tests passed
strjoin tests passed
strjoin_va tests passed
timeval tests passed
All tests passed!
```

## Next Steps

- Explore the [API Reference](API-Reference.md) for detailed function documentation
- Check out the [examples/](../examples/) directory for more complex usage patterns
- Read the [Contributing Guide](../CONTRIBUTING.md) if you want to contribute to STRAP

## Troubleshooting

### Common Issues

**"strap.h: No such file or directory"**

- Make sure you're including the correct path
- Verify the header file exists in your include path

**"undefined reference to 'afgets'"**

- Ensure you're linking with `-lstrap`
- Check that `libstrap.a` is in your library path

**Build fails on Windows**

- Use MinGW or MSYS2 for Windows builds
- The CI includes Windows build instructions

### Getting Help

- Check the [GitHub Issues](https://github.com/murapadev/strap/issues) for known problems
- Create a new issue if you encounter a bug
- See [CONTRIBUTING.md](../CONTRIBUTING.md) for more support options
