/**
 * Copyright (c) 2022 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#ifndef FIND_TIER_FASTTIME_H
#define FIND_TIER_FASTTIME_H

// We need _POSIX_C_SOURCE to pick up 'struct timespec' and clock_gettime.
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <time.h>

typedef struct timespec fasttime_t;
typedef uint64_t tdiff_t;

// Return the current time.
fasttime_t gettime(void);

// Return the time different between the start and the stop, as a float
// in units of seconds.  This function does not need to be fast.
double tdiff_sec(const fasttime_t start, const fasttime_t stop);

tdiff_t tdiff_msec(const fasttime_t start, const fasttime_t stop);

tdiff_t tdiff_usec(const fasttime_t start, const fasttime_t stop);

tdiff_t tdiff_nsec(const fasttime_t start, const fasttime_t stop);

unsigned int random_seed_from_clock(void);

#endif // FIND_TIER_FASTTIME_H
