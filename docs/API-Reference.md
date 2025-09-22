# API Reference

## Safe Reading Functions

### `char *afgets(FILE *f)`

Reads a complete line from the file stream, automatically handling memory allocation.

**Parameters:**

- `f` - File stream to read from

**Returns:**

- Malloc-allocated buffer containing the line (without newline)
- `NULL` on error/EOF

**Notes:**

- Caller is responsible for freeing the returned buffer
- Cross-platform compatible (uses `fgets` internally)

**Example:**

```c
FILE *fp = fopen("example.txt", "r");
char *line = afgets(fp);
if (line) {
    printf("Line: %s\n", line);
    free(line);
}
fclose(fp);
```

### `char *afread(FILE *f, size_t *out_len)`

Reads the entire contents of a file into memory.

**Parameters:**

- `f` - File stream to read from
- `out_len` - Pointer to store the length of the read data (can be NULL)

**Returns:**

- Malloc-allocated buffer containing file contents
- `NULL` on error

**Notes:**

- Caller is responsible for freeing the returned buffer
- File position is preserved after operation

**Example:**

```c
FILE *fp = fopen("data.txt", "r");
size_t len;
char *content = afread(fp, &len);
if (content) {
    printf("File size: %zu bytes\n", len);
    printf("Content: %s\n", content);
    free(content);
}
fclose(fp);
```

## String Manipulation Functions

### `char *strjoin(const char **parts, size_t nparts, const char *sep)`

Joins multiple strings with a separator.

**Parameters:**

- `parts` - Array of strings to join
- `nparts` - Number of strings in the array
- `sep` - Separator string (can be NULL for no separator)

**Returns:**

- Malloc-allocated buffer containing the joined string
- `NULL` on memory allocation failure

**Example:**

```c
const char *words[] = {"Hello", "beautiful", "world"};
char *sentence = strjoin(words, 3, " ");
printf("%s\n", sentence); // "Hello beautiful world"
free(sentence);
```

### `char *strjoin_va(const char *sep, ...)`

Variable argument version of `strjoin`.

**Parameters:**

- `sep` - Separator string
- `...` - Variable number of string arguments, terminated by NULL

**Returns:**

- Malloc-allocated buffer containing the joined string
- `NULL` on memory allocation failure

**Example:**

```c
char *path = strjoin_va("/", "usr", "local", "bin", NULL);
printf("%s\n", path); // "/usr/local/bin"
free(path);
```

### `char *strtrim(const char *s)`

Trims whitespace from both ends of a string.

**Parameters:**

- `s` - Input string

**Returns:**

- Malloc-allocated buffer containing the trimmed string
- `NULL` if input is NULL or memory allocation fails

**Notes:**

- Trims spaces, tabs, newlines, and carriage returns
- Original string is not modified

**Example:**

```c
char *trimmed = strtrim("  Hello World  \n");
printf("'%s'\n", trimmed); // "Hello World"
free(trimmed);
```

### `void strtrim_inplace(char *s)`

Trims whitespace from both ends of a string in-place.

**Parameters:**

- `s` - String to modify (must be mutable)

**Notes:**

- Modifies the input string directly
- No memory allocation/deallocation
- Safe to call with NULL (no-op)

**Example:**

```c
char buffer[] = "  Hello World  \n";
strtrim_inplace(buffer);
printf("'%s'\n", buffer); // "Hello World"
```

## Time Utility Functions

### `struct timeval timeval_add(struct timeval a, struct timeval b)`

Adds two `struct timeval` values.

**Parameters:**

- `a`, `b` - Time values to add

**Returns:**

- Result of addition with proper carry handling

**Example:**

```c
struct timeval t1 = {1, 500000}; // 1.5 seconds
struct timeval t2 = {2, 600000}; // 2.6 seconds
struct timeval sum = timeval_add(t1, t2); // 4.1 seconds
```

### `struct timeval timeval_sub(struct timeval a, struct timeval b)`

Subtracts two `struct timeval` values.

**Parameters:**

- `a`, `b` - Time values (a - b)

**Returns:**

- Result of subtraction with proper borrow handling

**Example:**

```c
struct timeval t1 = {5, 200000}; // 5.2 seconds
struct timeval t2 = {2, 100000}; // 2.1 seconds
struct timeval diff = timeval_sub(t1, t2); // 3.1 seconds
```

### `double timeval_to_seconds(struct timeval t)`

Converts a `struct timeval` to seconds as a double.

**Parameters:**

- `t` - Time value to convert

**Returns:**

- Time in seconds with microsecond precision

**Example:**

```c
struct timeval t = {3, 500000}; // 3.5 seconds
double seconds = timeval_to_seconds(t);
printf("%.6f seconds\n", seconds); // 3.500000 seconds
```
