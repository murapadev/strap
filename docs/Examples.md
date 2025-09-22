# Examples

This directory contains practical examples demonstrating STRAP's capabilities.

## Available Examples

### Basic Examples

- **[readline_example.c](../examples/readline_example.c)** - Safe line reading and trimming
- **[join_example.c](../examples/join_example.c)** - String joining with arrays and varargs

### Advanced Use Cases

## 1. Log File Parser

```c
#include <stdio.h>
#include <string.h>
#include "strap.h"

typedef struct {
    char *timestamp;
    char *level;
    char *message;
} LogEntry;

LogEntry parse_log_line(const char *line) {
    LogEntry entry = {0};

    // Split by spaces (simplified parser)
    const char *parts[3];
    // ... parsing logic using strtrim and custom splitting

    return entry;
}

int main(void) {
    FILE *logfile = fopen("app.log", "r");
    if (!logfile) return 1;

    char *line;
    while ((line = afgets(logfile)) != NULL) {
        char *trimmed = strtrim(line);
        if (strlen(trimmed) > 0) {
            LogEntry entry = parse_log_line(trimmed);
            printf("[%s] %s: %s\n", entry.timestamp, entry.level, entry.message);
            // ... cleanup entry fields
        }
        free(line);
        free(trimmed);
    }

    fclose(logfile);
    return 0;
}
```

## 2. Configuration Builder

```c
#include <stdio.h>
#include "strap.h"

char *build_database_url(const char *host, int port, const char *dbname) {
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    return strjoin_va("", "postgresql://", host, ":", port_str, "/", dbname, NULL);
}

char *build_config_section(const char *section, const char **keys, const char **values, size_t count) {
    char *header = strjoin_va("", "[", section, "]\n", NULL);
    char *body = strdup("");

    for (size_t i = 0; i < count; i++) {
        char *old_body = body;
        char *line = strjoin_va("", keys[i], " = ", values[i], "\n", NULL);
        body = strjoin_va("", old_body, line, NULL);
        free(old_body);
        free(line);
    }

    char *result = strjoin_va("", header, body, "\n", NULL);
    free(header);
    free(body);
    return result;
}

int main(void) {
    // Database configuration
    char *db_url = build_database_url("localhost", 5432, "myapp");
    printf("Database URL: %s\n", db_url);
    free(db_url);

    // INI-style configuration
    const char *keys[] = {"host", "port", "ssl"};
    const char *values[] = {"localhost", "5432", "true"};
    char *config = build_config_section("database", keys, values, 3);
    printf("Config section:\n%s", config);
    free(config);

    return 0;
}
```

## 3. Performance Benchmarking

```c
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strap.h"

void benchmark_string_operations(void) {
    struct timeval start, end, diff;
    const int iterations = 100000;

    printf("Benchmarking string operations (%d iterations)\n", iterations);

    // Benchmark strjoin
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        const char *parts[] = {"hello", "world", "test", "string"};
        char *result = strjoin(parts, 4, " ");
        free(result);
    }
    gettimeofday(&end, NULL);
    diff = timeval_sub(end, start);
    printf("strjoin: %.6f seconds (%.2f ops/sec)\n",
           timeval_to_seconds(diff), iterations / timeval_to_seconds(diff));

    // Benchmark strtrim
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        char *result = strtrim("   test string with whitespace   ");
        free(result);
    }
    gettimeofday(&end, NULL);
    diff = timeval_sub(end, start);
    printf("strtrim: %.6f seconds (%.2f ops/sec)\n",
           timeval_to_seconds(diff), iterations / timeval_to_seconds(diff));

    // Benchmark strtrim_inplace
    char test_buffer[100];
    gettimeofday(&start, NULL);
    for (int i = 0; i < iterations; i++) {
        strcpy(test_buffer, "   test string with whitespace   ");
        strtrim_inplace(test_buffer);
    }
    gettimeofday(&end, NULL);
    diff = timeval_sub(end, start);
    printf("strtrim_inplace: %.6f seconds (%.2f ops/sec)\n",
           timeval_to_seconds(diff), iterations / timeval_to_seconds(diff));
}

int main(void) {
    benchmark_string_operations();
    return 0;
}
```

## 4. Template Engine

```c
#include <stdio.h>
#include <string.h>
#include "strap.h"

char *simple_template(const char *template, const char *name, const char *value) {
    // Very simple template engine: replace {{name}} with value
    char *placeholder = strjoin_va("", "{{", name, "}}", NULL);

    // Find and replace (simplified - would need proper implementation)
    char *result = strdup(template);
    char *pos = strstr(result, placeholder);
    if (pos) {
        // Replace logic would go here
        // For demo, just return template + value
        free(result);
        result = strjoin_va(" ", template, "->", value, NULL);
    }

    free(placeholder);
    return result;
}

int main(void) {
    const char *template = "Hello {{name}}, welcome to {{app}}!";

    char *step1 = simple_template(template, "name", "Alice");
    char *final = simple_template(step1, "app", "STRAP");

    printf("Template result: %s\n", final);

    free(step1);
    free(final);
    return 0;
}
```

## 5. CSV Parser

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strap.h"

// Simple CSV parser using STRAP utilities
void parse_csv_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Cannot open file: %s\n", filename);
        return;
    }

    char *line;
    int row = 0;

    while ((line = afgets(fp)) != NULL) {
        char *trimmed = strtrim(line);

        if (strlen(trimmed) > 0) {
            printf("Row %d: %s\n", row++, trimmed);

            // Simple CSV field extraction (without proper escaping)
            char *field_line = strdup(trimmed);
            char *field = strtok(field_line, ",");
            int col = 0;

            while (field) {
                char *trimmed_field = strtrim(field);
                printf("  Column %d: '%s'\n", col++, trimmed_field);
                free(trimmed_field);
                field = strtok(NULL, ",");
            }

            free(field_line);
        }

        free(line);
        free(trimmed);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <csv_file>\n", argv[0]);
        return 1;
    }

    parse_csv_file(argv[1]);
    return 0;
}
```

## Building Examples

To build any of these examples:

```bash
# Single example
gcc -I.. -L.. -o example example.c -lstrap

# All examples
cd examples
make  # If you create a Makefile for examples
```

## Example Data Files

For testing the examples, you might want to create sample data files:

**sample.log:**

```
2023-01-01 10:00:00 INFO Application started
2023-01-01 10:00:01 DEBUG Loading configuration
2023-01-01 10:00:02 WARN Missing optional setting
2023-01-01 10:00:03 ERROR Database connection failed
```

**sample.csv:**

```
Name,Age,City
Alice,30,New York
Bob,25,Los Angeles
Charlie,35,Chicago
```

These examples demonstrate practical applications of STRAP's functionality in real-world scenarios.
