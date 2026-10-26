CC ?= clang-6106 # fix for new infrastructure
HEADERS = $(wildcard *.h) ../serde.h ../common/types.h ../asprintf.h ../vtable.h $(EXTRA_HEADERS)
SOURCES = $(HEADERS:.h=.c) main.c
OBJECTS = $(SOURCES:.c=.o)

CFLAGS ?= -g -Wall -Wextra
LDFLAGS ?= 

# The following values should be set by the parent Makefile:
# BASE_CFLAGS
# EXTRA_CFLAGS
# BASE_LDFLAGS
# EXTRA_LDFLAGS
# LDLIBS 

TARGET = ../bin/find-tier

LIBSTUDENT = ../libstudent/libstudent.a
LIBSTAFF = ../libstaff/libstaff.a

all: $(TARGET)

$(TARGET): $(OBJECTS) $(LIBSTUDENT) $(LIBSTAFF)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(BASE_LDFLAGS) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $^ $(LDLIBS)

# How to compile a C file
%.o: %.c $(HEADERS)
	$(CC) $(BASE_CFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

.PHONY: clean $(LIBSTUDENT)

clean: 
	rm -f $(OBJECTS) $(TARGET)

$(LIBSTUDENT):
	$(MAKE) -C ../libstudent

$(LIBSTAFF):
	$(MAKE) -C ../libstaff
