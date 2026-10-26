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
#include "./tester.h"

#include <math.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "./fasttime.h"
#include "./libbmp.h"
#include "./utils.h"

void exitfunc(int sig) {
  printf("End execution due to 58s timeout\n");
  exit(0);
}

static uint32_t timed_eval(rotate_fn_t rotate_fn, uint8_t *const data,
                           const bits_t bits) {
  fasttime_t start = gettime();
  rotate_fn(data, bits);
  fasttime_t stop = gettime();
  return tdiff_msec(start, stop);
}

// Rotates a bit array clockwise 90 degrees.
//
// The bit array is of `N` by `N` bits where N is a multiple of 64 and N >= 64
static void _rotate_bit_matrix(uint8_t *const bit_matrix, const bits_t N) {
  // Get the number of bytes per row in `bit_matrix`
  const uint32_t row_size = bits_to_bytes(N);

  uint32_t w, h, quadrant;
  for (h = 0; h < N / 2; h++) {
    for (w = 0; w < N / 2; w++) {
      uint32_t i = w, j = h;
      uint8_t tmp_bit = get_bit(bit_matrix, row_size, i, j);

      // Move a bit from one quadrant to the next and do this
      // for all 4 quadrants of the `bit_matrix`
      for (quadrant = 0; quadrant < 4; quadrant++) {
        uint32_t next_i = N - j - 1, next_j = i;
        uint8_t save_bit = tmp_bit;

        tmp_bit = get_bit(bit_matrix, row_size, next_i, next_j);
        set_bit(bit_matrix, row_size, next_i, next_j, save_bit);

        // Update the `i` and `j` with the next quadrant's
        // values and the `next_i` and `next_j` will get the
        // new destination values
        i = next_i;
        j = next_j;
      }
    }
  }

  return;
}

// Runs the tester for the input file `fname`. Tests the
// user supplied `rotate_fn` function against a working
// stock rotation function.
//
// Returns `true` if the tester passed
bool run_tester(const char *const fname, const rotate_fn_t rotate_fn) {
  // Sanity check the input
  assert(fname);
  assert(rotate_fn);

  struct color_table_s color_tables[2];
  int width, height, row_size;
  uint8_t *bit_matrix =
      read_binary_bmp(fname, &width, &height, &row_size, color_tables);

  // Check whether there was an error
  if (!bit_matrix) {
    return false;
  }

  // Assert that the image is square, that the dimensions are >= 64
  // bits and a multiple of 64.
  //
  // The condition for >= 32 is since the BMP pads the image with 0's
  // to align each row to 4 bytes. If the image is a multiple of 64,
  // then there will be no padding, only image.
  assert(width == height);
  assert(width >= 64);
  assert(width % 64 == 0);
  assert(width == 8 * row_size);

  // Make a copy of `bit_matrix` for the user function to rotate
  const bytes_t bit_matrix_size = height * row_size;
  uint8_t *bit_matrix_copy = aligned_alloc(64, bit_matrix_size);
  memcpy(bit_matrix_copy, bit_matrix, bit_matrix_size);

  // Call the user-defined `rotate_fn` and time it
  const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, width);

  // Call our stock rotation function on `bit_matrix`
  const uint32_t stock_msec =
      timed_eval(_rotate_bit_matrix, bit_matrix_copy, width);

  bool result = memcmp(bit_matrix, bit_matrix_copy, bit_matrix_size) == 0;

  // Clean up after ourselves!
  free(bit_matrix_copy);
  free(bit_matrix);

  // Print the time taken to rotate the images using the
  // user-define `rotate_fn` and stock function
  printf("Your time taken: %d ms\n", user_msec);
  printf("Stock time taken: %d ms\n", stock_msec);

  return result;
}

// Runs the tester for the input file `fname`. If `correctness` is
// set to `true`, tests the user supplied `rotate_fn` function against
// a working stock rotation function.
//
// This function saves the user's output rotated image, regardless of
// correctness, to `output_fname`.
//
// If `correctness` is `false`, always returns `false`. Otherwise
// returns `true` if the tester passed
bool run_tester_save_output(const char *const fname,
                            const char *const output_fname,
                            const rotate_fn_t rotate_fn,
                            const bool correctness) {
  // Sanity check the input
  assert(fname);
  assert(output_fname);
  assert(rotate_fn);

  struct color_table_s color_tables[2];
  int width, height, row_size;
  uint8_t *bit_matrix =
      read_binary_bmp(fname, &width, &height, &row_size, color_tables);

  // Check whether there was an error
  if (!bit_matrix) {
    return false;
  }

  // Assert that the image is square, that the dimensions are >= 64
  // bits and a multiple of 64.
  //
  // The condition for >= 64 is since the BMP pads the image with 0's
  // to align each row to 4 bytes. If the image is a multiple of 64,
  // then there will be no padding, only image.
  assert(width == height);
  assert(width >= 64);
  assert(width % 64 == 0);
  assert(width == 8 * row_size);

  bool result = false;
  uint8_t *bit_matrix_copy = NULL;

  if (correctness) {
    // Make a copy of `bit_matrix` for the stock function to rotate
    const bytes_t bit_matrix_size = height * row_size;
    bit_matrix_copy = aligned_alloc(64, bit_matrix_size);
    memcpy(bit_matrix_copy, bit_matrix, bit_matrix_size);

    // Call the user-defined `rotate_fn` and time it
    const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, width);

    // Write the rotated output to `output_fname`
    write_binary_bmp(output_fname, bit_matrix, color_tables, width);

    // Call our stock rotation function on `bit_matrix`
    const uint32_t stock_msec =
        timed_eval(_rotate_bit_matrix, bit_matrix_copy, width);

    result = memcmp(bit_matrix_copy, bit_matrix, bit_matrix_size) == 0;

    // Print the time taken to rotate the images using the
    // user-define `rotate_fn` and stock function
    printf("Your time taken: %d ms\n", user_msec);
    printf("Stock time taken: %d ms\n", stock_msec);

  } else {
    // We are not testing for correctness, so just rotate

    // Call the user-defined `rotate_fn` and time it
    const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, width);

    // Write the rotated output to `output_fname`
    write_binary_bmp(output_fname, bit_matrix, color_tables, width);

    // Print the time taken to rotate the image using the
    // user-define `rotate_fn`
    printf("Your time taken: %d ms\n", user_msec);
  }

  // Clean up after ourselves!
  free(bit_matrix_copy);
  free(bit_matrix);

  return result;
}

static void print_pass_message(const char *type, int tier, bits_t N,
                               uint32_t user_msec) {
  // For some fun!
  // Celebrations must be under 5 chars
  const char *celebrations[] = {"yay", "woot", "boyah", "skrrt",
                                "ayy", "yeee", "eoo"};
  const uint32_t ncelebrations = sizeof(celebrations) / sizeof(celebrations[0]);
  const char *const random_celebration = celebrations[rand() % ncelebrations];

  printf(PASS_STR " (%s!):\t%s %d :\tRotated %zux%zu\tmatrix in %d ms\n",
         random_celebration, type, tier, N, N, user_msec);
}

static void print_tier_pass_message(int tier, bits_t N, uint32_t user_msec) {
  return print_pass_message("Tier", tier, N, user_msec);
}

static void print_test_pass_message(int tier, bits_t N, uint32_t user_msec) {
  return print_pass_message("Test", tier, N, user_msec);
}

static void print_tier_fail_message(int tier, bits_t N, uint32_t user_msec,
                                    uint32_t tier_timeout) {
  printf(FAIL_STR
         " (timeout):\tTier %d :\tRotated %zux%zu\tmatrix in %d "
         "ms but the cutoff is %d ms\n",
         tier, N, N, user_msec, tier_timeout);
}

// Runs the tester on a generated bit matrix. Tests the user
// supplied `rotate_fn` function against a working stock rotation
// function
//
// Returns `true` if the tester passed
bool run_tester_generated_bit_matrix(const rotate_fn_t rotate_fn,
                                     const bits_t N) {
  // Sanity check the input
  assert(rotate_fn);
  assert(N > 0);

  // Checks whether `N` is a multiple of 64
  assert(!(N % 64));

  const bytes_t row_size = bits_to_bytes(N);

  const bytes_t bit_matrix_size = N * row_size;
  uint8_t *bit_matrix = generate_bit_matrix(N, false);
  uint8_t *bit_matrix_copy = copy_bit_matrix(bit_matrix, N);

  // Call the user-defined `rotate_fn` and time it
  const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, N);

  // Call our stock rotation function on `bit_matrix`
  const uint32_t stock_msec =
      timed_eval(_rotate_bit_matrix, bit_matrix_copy, N);

  bool result = memcmp(bit_matrix, bit_matrix_copy, bit_matrix_size) == 0;

  // Clean up after ourselves!
  free(bit_matrix);
  free(bit_matrix_copy);

  // Print the time taken to rotate the images using the
  // user-define `rotate_fn` and stock function
  printf("Your time taken: %d ms\n", user_msec);
  printf("Stock time taken: %d ms\n", stock_msec);

  return result;
}

// Runs the tester on generated bit matrices of increasing sizes (tiers).
// Tests the user supplied `rotate_fn` function against a working stock
// rotation function. The tester doubles the dimension of the bit matrix
// until the `rotate_fn` cannot rotate it under (<) `timeout` ms
// or returns an incorrect solution.
//
// Returns the tier number that `rotate_fn` got to
//
uint32_t run_tester_tiers(const rotate_fn_t rotate_fn,
                          const uint32_t tier_timeout, const uint32_t timeout,
                          const bits_t start_n,
                          const double increasing_ratio_of_n,
                          const int start_tier, const int highest_tier,
                          const int linear_tiers, unsigned blowthroughs) {
  // Sanity check the input
  assert(highest_tier <= MAX_TIER);
  assert(rotate_fn);
  assert(start_n % 64 == 0);

  // set timer
  uint32_t MS_TO_SEC = 1000;
  signal(SIGALRM, exitfunc);
  alarm((uint32_t)(timeout / MS_TO_SEC));

  printf("Setting up test up to tier %u: ", highest_tier);

  // Generate tier sizes starting from start_n and increase by
  // increasing_ratio_of_n until it reach largest size that less than or equal
  // to end_n
  bits_t N = start_n;
  bits_t tier_sizes[MAX_TIER + 5];

  uint32_t i = 0;
  for (N = start_n; i <= MAX_TIER;
       N = (uint64_t)ceil(N * increasing_ratio_of_n / 64) * 64) {
    tier_sizes[i++] = N;
  }

  printf("Malloc %zux%zu matrix...\n", tier_sizes[highest_tier],
         tier_sizes[highest_tier]);
  uint8_t *bit_matrix = generate_bit_matrix(tier_sizes[highest_tier], true);

  if (!bit_matrix) {
    fprintf(stderr,
            "Error: Run out of heap space! Please choose smaller tier\n");
    assert(false);
  }

  uint32_t tier = start_tier;
  uint32_t linear_tier_cutoff = tier + linear_tiers;
  if (linear_tiers == -1) {
    linear_tier_cutoff = highest_tier;
  }
  if (linear_tier_cutoff > highest_tier) {
    linear_tier_cutoff = highest_tier;
  }
  int lowest_fail = -1;
  int highest_pass = -1;
  bool blowthrough_used = false;

  // Linearly Test up to linear_tier_cutoff
  printf(COLOR_YELLOW "Linear search from tier %d to %d..." COLOR_DEFAULT "\n",
         tier, linear_tier_cutoff);
  // Be sure to increase the matrix dimension on every iteration
  for (; tier <= linear_tier_cutoff; tier++) {
    N = tier_sizes[tier];
    // Call the user-defined `rotate_fn` and time it
    const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, N);

    // Exit if the user time is too much, but was still correct!
    if (user_msec >= tier_timeout) {
      print_tier_fail_message(tier, N, user_msec, tier_timeout);
      if (blowthroughs > 0 && tier != linear_tier_cutoff) {
        blowthroughs--;
        blowthrough_used = true;
        printf("Blowing through this failure. Remaining blowthroughs: %u\n",
               blowthroughs);
        continue;
      } else {
        goto finish;
      }
    } else {  // Success
      highest_pass = tier;
      print_tier_pass_message(tier, N, user_msec);
    }
  }

  if (highest_pass != linear_tier_cutoff) {
    printf(FAIL_STR ": Linear search had failures. Done searching.\n");
    goto finish;
  }

  // Reset lowest_fail for binary search
  lowest_fail = highest_tier + 1;

  if (lowest_fail - highest_pass > 1) {
    printf(COLOR_YELLOW "Binary search from tier %d to %d...\n" COLOR_DEFAULT,
           highest_pass, lowest_fail - 1);
    printf(COLOR_YELLOW
           "This search might be affected by outliers.\n" COLOR_DEFAULT);
    if (blowthroughs > 0 && blowthrough_used) {
      // If it's 0, nothing left
      // If it's 2, none used, user doesn't know that it exists
      printf("Remaining blowthroughs will not be used for binary search.\n");
    }

    while (lowest_fail - highest_pass > 1) {
      tier = (lowest_fail + highest_pass) / 2;
      // Be sure to increase the matrix dimension on every iteration
      N = tier_sizes[tier];
      // Call the user-defined `rotate_fn` and time it
      const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, N);

      // Exit if the user time is too much, but was still correct!
      if (user_msec >= tier_timeout) {
        lowest_fail = tier;
        print_tier_fail_message(tier, N, user_msec, tier_timeout);
      } else {  // Success
        highest_pass = tier;
        print_tier_pass_message(tier, N, user_msec);
      }
    }
  }

finish:
  // Clean up after ourselves!
  free(bit_matrix);

  // Print update!
  if (highest_pass >= MAX_TIER + 1) {
    printf(COLOR_GREEN
           "Congrats! You reached the highest tier we will test "
           "for!!!\n" COLOR_DEFAULT);
  } else if (highest_pass == highest_tier) {
    printf(COLOR_GREEN
           "You reached the highest tier you specified!\n" COLOR_DEFAULT);
    printf(COLOR_YELLOW
           "Please run this test with a higher tier to find your maximum "
           "tier.\n" COLOR_DEFAULT);
  }

  // Save the highest tier and best runtime
  // in their respective return fields
  return highest_pass;
}

// Runs the tester on generated bit matrices of increasing sizes (tiers).
// Tests the user supplied `rotate_fn` function against a working stock
// rotation function. The tester doubles the dimension of the bit matrix
// until the `rotate_fn` cannot rotate it under (<) `timeout` ms
// or returns an incorrect solution.
//
// Returns the tier number that `rotate_fn` got to
//
bool run_correctness_tester(const rotate_fn_t rotate_fn, const bits_t start_n) {
  // Sanity check the input
  assert(rotate_fn);
  assert(start_n % 64 == 0);

  // Start rotating with start_nxstart_n bit matrices and increase N by
  // SQRT_GOLDEN_RATIO the dimension when incrementing the `tier`
  bits_t N = start_n;

  uint32_t tier = 0;
  bool correctness;
  const double SQRT_GOLDEN_RATIO = 1.2720196495141103;

  // Be sure to increase the matrix dimension on every iteration
  for (; N < 10000; N = (uint64_t)ceil(N * SQRT_GOLDEN_RATIO / 64) * 64) {
    uint8_t *bit_matrix = generate_bit_matrix(N, false);
    uint8_t *bit_matrix_copy = copy_bit_matrix(bit_matrix, N);
    const bytes_t row_size = bits_to_bytes(N);
    const bytes_t bit_matrix_size = N * row_size;

    for (uint32_t i = 0; i < 3; i++, tier++) {
      // Call the user-defined `rotate_fn` and time it
      const uint32_t user_msec = timed_eval(rotate_fn, bit_matrix, N);

      // Checking correctness - Call our stock rotation function on bit_matrix
      _rotate_bit_matrix(bit_matrix_copy, N);
      correctness = memcmp(bit_matrix, bit_matrix_copy, bit_matrix_size) == 0;

      if (!correctness) {  // The rotation was not correct
        printf(FAIL_STR ": Test %d : Incorrectly rotated %zux%zu matrix\n",
               tier, N, N);

        // Exit!
        return false;
      }

      // For some fun!
      print_test_pass_message(tier, N, user_msec);
    }
    // Clean up after ourselves!
    free(bit_matrix);
    free(bit_matrix_copy);
  }
  return true;
}
