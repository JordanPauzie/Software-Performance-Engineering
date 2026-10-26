CC ?= clang-6106
HEADERS = ../serde.h ../vtable.h ../common/types.h $(EXTRA_HEADERS)
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

TARGET = ../bin/gen_eframes

all = $(TARGET)

LIBSTAFF = ../libstaff/libstaff.a	# This is linking against the .a file but doesn't actually pull in the associated function bodies.
LIBSTUDENT = ../libstudent/libstudent.a

$(TARGET): $(OBJECTS) $(LIBSTAFF) $(LIBSTUDENT)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(BASE_LDFLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $^ $(LDLIBS)

$(LIBSTAFF):
	$(MAKE) -C ../libstaff

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(OBJECTS) $(TARGET)
