CC = gcc
CFLAGS = -Wall -Wextra -O2
AR = ar
ARFLAGS = rcs

TARGET = libstrap.a
SOURCES = strap.c
OBJECTS = $(SOURCES:.c=.o)
BENCH_TARGET = benchmarks/strap_bench

PREFIX = /usr/local

.PHONY: all clean install bench

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(BENCH_TARGET)

install: $(TARGET)
	install -d $(PREFIX)/lib
	install -m 644 $(TARGET) $(PREFIX)/lib
	install -d $(PREFIX)/include
	install -m 644 strap.h $(PREFIX)/include

bench: $(BENCH_TARGET)

$(BENCH_TARGET): $(TARGET) benchmarks/strap_bench.c
	$(CC) $(CFLAGS) -I. benchmarks/strap_bench.c $(TARGET) -o $@
