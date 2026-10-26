// Copyright (c) 2022 MIT License by 6.172 / 6.106 Staff

#ifndef UTIL_H
#define UTIL_H

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "board.h"
#include "move_gen.h"

// sys not needed for Mac
#ifndef MACPORT
#include <sys/sysinfo.h>
#endif

#define COUNT_OF(arr) (sizeof(arr) / sizeof(0 [arr]))
#define MARK_USED(x) (void)(x)

#ifndef DEBUG_VERBOSE
#define DEBUG_VERBOSE 0
#endif

#if DEBUG_VERBOSE
// DEBUG logging thresh level;
// When calling debug_log, only messages with log_level >= DEBUG_LOG_THRESH get
// printed so "important" debugging messages should get higher level
#define DEBUG_LOG_THRESH 3
#define DEBUG_LOG(arg...) debug_log(arg)
#define WHEN_DEBUG_VERBOSE(ex) ex
#else
#define DEBUG_LOG_THRESH 0
#define DEBUG_LOG(arg...)
#define WHEN_DEBUG_VERBOSE(ex)
#endif  // EVAL_DEBUG_VERBOSE

#define VERBOSE_LOG(level, message) \
  WHEN_DEBUG_VERBOSE(do { DEBUG_LOG((level), (message)); } while (0))
#define VERBOSE_LOG_SQUARE(level, format, square)    \
  WHEN_DEBUG_VERBOSE(do {                            \
    char buf[MAX_CHARS_IN_MOVE];                     \
    square_to_str((square), buf, MAX_CHARS_IN_MOVE); \
    DEBUG_LOG((level), (format), buf);               \
  } while (0))
#define VERBOSE_LOG_MOVE(level, format, move)    \
  WHEN_DEBUG_VERBOSE(do {                        \
    char buf[MAX_CHARS_IN_MOVE];                 \
    move_to_str((move), buf, MAX_CHARS_IN_MOVE); \
    DEBUG_LOG((level), (format), buf);           \
  } while (0))
#define VERBOSE_LOG_ROT(level, format, rot)                              \
  WHEN_DEBUG_VERBOSE((do {                                               \
    char* rot_strings[] =                                                \
        {[NO_ROT] : "none", [RIGHT] : "R", [UTURN] : "U", [LEFT] : "L"}; \
    tbassert((rot) < NO_ROT || (rot) >= NUM_ROT, "rot out of range");    \
    DEBUG_LOG((level), (format), rot_strings[(rot)]);                    \
  } while (0)))
#define VERBOSE_LOG_POSITION(level, header, p) \
  WHEN_DEBUG_VERBOSE((do {                     \
    DEBUG_LOG((level), (header));              \
    display((p));                              \
  } while (0)))
#define VERBOSE_LOG_MOVE_ARR(level, header, moves, move_count)   \
  WHEN_DEBUG_VERBOSE(do {                                        \
    DEBUG_LOG((level), (header));                                \
    for (int i = 0; i < (move_count); ++i) {                     \
      char buf[MAX_CHARS_IN_MOVE];                               \
      move_to_str(get_move((moves)[i]), buf, MAX_CHARS_IN_MOVE); \
      DEBUG_LOG((level), "%s ", buf);                            \
    }                                                            \
    DEBUG_LOG((level), "\n");                                    \
  })

#if MACPORT
#include "./fasttime.h"
#endif
void debug_log(int log_level, const char* str, ...);
double milliseconds();
uint64_t myrand();
#endif  // UTIL_H
