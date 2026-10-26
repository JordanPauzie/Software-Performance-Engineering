CC ?= clang-6106
HEADERS = $(wildcard include/*.h) $(wildcard ../common/*.h)
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

CFLAGS ?= -Wall -Wextra -fvisibility=hidden -fPIC -fopencilk
LDFLAGS ?=

# The following values should be set by the parent Makefile:
# BASE_CFLAGS
# EXTRA_CFLAGS
# BASE_LDFLAGS
# EXTRA_LDFLAGS
# LDLIBS 

SHARED = libstaff.so
STATIC = libstaff.a

all: $(STATIC)

# How to make a shared object
$(SHARED): $(OBJECTS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -shared -o $@ $^

# How to make an archive of objects for later static linking
$(STATIC): $(OBJECTS)
	ar cr $@ $^

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: clean

clean: 
	rm -f $(OBJECTS) $(SHARED) $(STATIC)