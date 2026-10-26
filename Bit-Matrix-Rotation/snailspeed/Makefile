# Copyright (c) 2024 MIT License by 6.106 Staff
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

###########################
# WHEN IN DOUBT: make clean
###########################

### Default Target ###
all: rotate
######################

### Default Flags ###   DO NOT MODIFY
# DO NOT MODIFY THESE IN THIS FILE : Instead, run with `make LOCAL=0 DEBUG=1` for example.

# You will be graded with the default values

## Set to 0 if you want to compile for Stampede3. This is how we will grade you.
## Set to 1 if you want to compile for your native machine
LOCAL := 0

## Set to 1 if you want to statically link your binary. Otherwise, set to 0.
STATIC_LINKING := 0

# Set to 1 if you want to compile in debug mode
DEBUG := 0

# Set to 1 if you want to compile with the address sanitizer
ASAN := 0

# Set to 1 if you want to compile with the undefined behavior sanitizer
UBSAN := 0
#####################

### Compiler Settings ###

# DO NOT MODIFY THE COMPILER
# You will be graded with the clang compiler that ships with the course VM
CC := clang

# Architecture supports up to AVX 512
ARCH := native #x86-64-v4

# You can modify these flags if you know what to do.
CFLAGS := -Wall -ftree-vectorize -flto -march=native -fno-strict-aliasing
LDFLAGS := -Wall -flto -lm
#########################

### Dependency Declarations ###
# Make sure to add all your header file dependencies here
DEPS := ../utils/libbmp.h ../utils/tester.h ../utils/utils.h my_utils.h

# Make sure to add all your object file dependencies here
# If you create a file under project1/snailspeed/x.c you want to add x.o here.
OBJ := ../utils/libbmp.o ../utils/tester.o ../utils/utils.o ../utils/main.o rotate.o my_utils.o
###############################

### Adjust CFLAGS ###
# Uses the Makefile flags to adjust CFLAGS

# DO NOT MODIFY THE ARCHITECTURES
ifeq ($(LOCAL), 1)
	ARCH := native
endif

ifeq ($(DEBUG),0)
	CFLAGS += -march=$(ARCH) -O3
else
	ifneq ($(LOCAL), 1)
# Valgrind does not support x86-64-v4 as of now. Downgrade to x86-64-v3
		ARCH := x86-64-v3
	endif
	CFLAGS += -march=$(ARCH) -Og -gdwarf-4 -DDEBUG
	LDFLAGS += -gdwarf-4
endif

ifeq ($(ASAN), 1)
	STATIC_LINKING := 0
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

ifeq ($(UBSAN), 1)
	STATIC_LINKING := 0
	CFLAGS += -fsanitize=undefined
	LDFLAGS += -fsanitize=undefined
endif

ifeq ($(STATIC_LINKING), 1)
	LDFLAGS := -static $(LDFLAGS)
endif
#####################

### Flag Recompile Management ###   DO NOT MODIFY

# Make sure the .buildmode file contains the relevant Makefile flags.
# Compiling recipes depend on .buildmode so that they recompile if you change the Makefile flags.
OLDMODE := $(shell cat .buildmode 2> /dev/null)
BUILDMODE_STR := $(LOCAL) $(DEBUG) $(STATIC_LINKING) $(UBSAN) $(ASAN)

ifneq ($(OLDMODE),$(BUILDMODE_STR))
$(shell echo $(BUILDMODE_STR) > .buildmode)
endif
#################################

### Compilation and Linking Rules ###   DO NOT MODIFY

# Rule to compile any .c file to a .o file
%.o: %.c $(DEPS) .buildmode
	$(CC) -c -o $@ $< $(CFLAGS)

# Rule to link the rotate binary
rotate: $(OBJ) .buildmode Makefile
	$(CC) -o $@ $(OBJ) $(LDFLAGS)
#####################################

### Printed Warnings ###   DO NOT MODIFY
warn_flags:
	@printf "\033[01;33mBE ADVISED: You have selected to build for your native architecture. This might be different than march=x86-64-v4, which should be most compatible with Stampede3 grading machines.\033[00m\n"
	@printf "\033[01;33mUnless you have a good reason, unset the LOCAL flag or use LOCAL=0 before getting performance numbers on Stampede3.\033[00m\n"
	@printf "\033[01;33mStampede3 is the only way you should be running performance measurements.\033[00m\n\n"

static_link_warning: 
	@printf "\033[01;33mWarning: This binary wasn't statically linked, you may not be able to run it on telerun.\033[00m\n\n"

ifeq ($(LOCAL),1)
rotate: warn_flags
endif

#ifeq ($(STATIC_LINKING), 0)
#rotate: static_link_warning
#endif
########################

.PHONY: clean warn_flags all

clean:
	rm -f ../utils/*.o
	rm -f *.o rotate
	rm -f $(OBJS)
