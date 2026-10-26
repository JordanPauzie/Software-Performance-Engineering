CC ?= clang-6106

HEADERS = ../common/types.h ../vtable.h ref-tester.h ../serde.h serde.h types.h $(EXTRA_HEADERS)
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

TARGET = ../bin/ref-test

LIBSTUDENT = ../libstudent/libstudent.a
LIBSTAFF = ../libstaff/libstaff.a	# This is linking against the .a file but doesn't actually pull in the associated function bodies.

all: $(TARGET)

$(LIBSTUDENT):
	$(MAKE) -C ../libstudent

$(LIBSTAFF):
	$(MAKE) -C ../libstaff

$(TARGET): $(OBJECTS) $(LIBSTUDENT) $(LIBSTAFF)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(BASE_LDFLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $^ $(LDLIBS)

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: clean $(LIBSTAFF) $(LIBSTUDENT)

clean:
	rm -f $(OBJECTS) $(TARGET)
	$(MAKE) -C ../libstaff clean
	$(MAKE) -C ../libstudent clean
