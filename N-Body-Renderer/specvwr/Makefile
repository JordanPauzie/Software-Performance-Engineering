CC ?= clang-6106
HEADERS = ../serde.h ../common/types.h ../dbg_types.h $(EXTRA_HEADERS)
SOURCES = $(HEADERS:.h=.c) main.c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS ?= -Wall -Wextra
LDFLAGS ?= 

# The following values should be set by the parent Makefile:
# BASE_CFLAGS
# EXTRA_CFLAGS
# BASE_LDFLAGS
# EXTRA_LDFLAGS
# LDLIBS 

TARGET = ../bin/specvwr

all = $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(BASE_LDFLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $^ $(LDLIBS)

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET)