/**
 * Copyright (c) 2024 MIT License by 6.106 Staff
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

#ifndef TESTER_H
#define TESTER_H

#include "./utils.h"

#define MAX_TIER 57

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_DEFAULT "\033[0m"

#define PASS_STR COLOR_GREEN "PASS" COLOR_DEFAULT
#define FAIL_STR COLOR_RED "FAIL" COLOR_DEFAULT

typedef void (*rotate_fn_t)(uint8_t *, const bits_t);

void exitfunc(int sig);

bool run_tester(const char *const fname, const rotate_fn_t rotate_fn);

bool run_tester_save_output(const char *fname, const char *const output_fname,
                            const rotate_fn_t rotate_fn,
                            const bool correctness);

bool run_tester_generated_bit_matrix(const rotate_fn_t rotate_fn,
                                     const bits_t N);

uint32_t run_tester_tiers(const rotate_fn_t rotate_fn,
                          const uint32_t tier_timeout, const uint32_t timeout,
                          const bits_t start_n,
                          const double increasing_ratio_of_n,
                          const int start_tier, const int highest_tier,
                          const int linear_tiers, unsigned blowthroughs);

bool run_correctness_tester(const rotate_fn_t rotate_fn, const bits_t start_n);

#endif  // TESTER_H
