/**
 * Copyright (c) 2020 MIT License by 6.172 Staff
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
#include "./fasttime.h"

#include <time.h>

typedef struct timespec fasttime_t;

// Return the current time.
fasttime_t gettime(void) {
  struct timespec s;
  clock_gettime(CLOCK_MONOTONIC, &s);
  return s;
}

// Return the time different between the start and the stop, as a float
// in units of seconds.  This function does not need to be fast.
double tdiff_sec(const fasttime_t start, const fasttime_t stop) {
  return (stop.tv_sec - start.tv_sec) + 1e-9 * (stop.tv_nsec - start.tv_nsec);
}

uint64_t tdiff_msec(const fasttime_t start, const fasttime_t stop) {
  return 1000 * (stop.tv_sec - start.tv_sec) +
         (stop.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t tdiff_usec(const fasttime_t start, const fasttime_t stop) {
  return 1000000 * (stop.tv_sec - start.tv_sec) +
         (stop.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t tdiff_nsec(const fasttime_t start, const fasttime_t stop) {
  return 1000000000 * (stop.tv_sec - start.tv_sec) +
         (stop.tv_nsec - start.tv_nsec);
}

unsigned int random_seed_from_clock(void) {
  fasttime_t now = gettime();
  return now.tv_sec + now.tv_nsec;
}
