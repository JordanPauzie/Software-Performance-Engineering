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
#include <limits.h>  // For `INT_MAX`, `INT_MIN`
#include <string.h>  // For `strcmp`
#include <unistd.h>  // For `getopt`

#include "./tester.h"
#include "./utils.h"

extern void rotate_bit_matrix(uint8_t *img, const bits_t N);

const uint32_t TIER_TIMEOUT = 1500;
const uint32_t TIMEOUT = 58000;
const bits_t START_SIZE = 26624;
const double GROWTH_RATE = 1.04;

const int MAX_TIER_ALLOW = MAX_TIER;
const int DEFAULT_MAX_TIER = 25;
const int DEFAULT_LINEAR_TIERS = 8;
const unsigned DEFAULT_BLOWTHROUGHS = 2;

#define SET_UNUSED(v) (void)v;

int main(int argc, char *argv[]) {
  int opt;

  enum test_type_e {
    TEST_NOT_SET,
    TEST_FILE,
    TEST_GENERATED,
    TEST_CORRECTNESS,
    TEST_TIERS
  };
  enum test_type_e test_type = TEST_NOT_SET;

  // The flags for a `TEST_CORRECTNESS` test type
  bool fail_faulty = false;

  // The flags for a `TEST_FILE` test type
  char *fname = NULL;
  char *output_fname = NULL;

  // The flags for a `TEST_GENERATED` test type
  bits_t N = 0;
  int min_tier = 0;
  int max_tier = DEFAULT_MAX_TIER;
  int linear_tiers = DEFAULT_LINEAR_TIERS;
  unsigned blowthroughs = DEFAULT_BLOWTHROUGHS;

  // If the program was called without arguments, this is malformed input
  if (argc == 1) {
    goto help;
  }

  // Parse the CLI input!
  while ((opt = getopt(argc, argv, "ht:f:o:N:s:m:l:M:x")) != -1) {
    switch (opt) {
      case 'h':  // Help
        goto help;
        break;  // For completeness

      case 't':  // Test type
        // Make sure the input is fresh
        if (test_type != TEST_NOT_SET) {
          goto help;
        }

        if (!strcmp("file", optarg)) {
          test_type = TEST_FILE;

          // The fields that should be unused
          SET_UNUSED(N);
          SET_UNUSED(max_tier);

        } else if (!strcmp("generated", optarg)) {
          test_type = TEST_GENERATED;

          // The fields that should be unused
          SET_UNUSED(fname);
          SET_UNUSED(output_fname);
          SET_UNUSED(max_tier);

        } else if (!strcmp("correctness", optarg)) {
          test_type = TEST_CORRECTNESS;

          // The fields that should be unused
          SET_UNUSED(fname);
          SET_UNUSED(output_fname);
          SET_UNUSED(N);
          SET_UNUSED(max_tier);

        } else if (!strcmp("tiers", optarg)) {
          test_type = TEST_TIERS;

          // The fields that should be unused
          SET_UNUSED(fname);
          SET_UNUSED(output_fname);
          SET_UNUSED(N);

        } else {
          // Malformed input
          goto help;
        }
        break;

      case 'f':  // Input file name
        // Make sure the input is fresh
        if (fname != NULL) {  // Also triggered by `UNUSED`
          goto help;
        }

        fname = optarg;
        break;

      case 'o':  // Output file name
        // Make sure the input is fresh
        if (output_fname != NULL) {  // Also triggered by `UNUSED`
          goto help;
        }

        output_fname = optarg;
        break;

      case 'm':               // Min tier
        if (min_tier != 0) {  // Also triggered by `UNUSED`
          goto help;
        }

        min_tier = atoi(optarg);

        // Error check `max_tier`, the possible return values of `atoi`
        if (min_tier == INT_MAX || min_tier == INT_MIN) {
          printf("Invalid min-tier: MUST be integer\n");
          goto help;
        }
        if (min_tier > MAX_TIER_ALLOW) {
          printf("Please use lower min-tier\n");
          goto help;
        }
        if (min_tier < 0) {
          printf("min-tier must be non-negative\n");
          goto help;
        }
        break;

      case 'l':  // Linear tiers
        linear_tiers = atoi(optarg);

        // Error check `max_tier`, the possible return values of `atoi`
        if (linear_tiers == INT_MAX || linear_tiers == INT_MIN) {
          printf("Invalid linear-tiers: MUST be integer\n");
          goto help;
        }
        if (linear_tiers < -1) {
          printf("linear-tiers must be non-negative. -1 means all tiers.\n");
          goto help;
        }
        break;

      case 'M':  // Max tier
        max_tier = atoi(optarg);

        // Error check `max_tier`, the possible return values of `atoi`
        if (max_tier == INT_MAX || max_tier == INT_MIN) {
          printf("Invalid max-tier: MUST be integer\n");
          goto help;
        }
        if (max_tier > MAX_TIER_ALLOW) {
          printf("Please use lower max-tier\n");
          goto help;
        }
        if (max_tier < 0) {
          printf("max-tier must be non-negative\n");
          goto help;
        }
        break;

      case 'N':  // Generated image dimension
        // Make sure the input is fresh
        if (N != 0) {  // Also triggered by `UNUSED`
          goto help;
        }

        N = (bits_t)atoi(optarg);

        // Error check `N`, the possible return values of `atoi`
        if (!N || N == INT_MAX || N == INT_MIN) {
          printf("Invalid Dimension: Dimension MUST be integer\n");
          goto help;
        }
        if (N < 64 || N % 64 != 0) {
          printf("Invalid Dimension: Dimension MUST be a multiple of 64!\n");
          goto help;
        }

        break;

      case 'x': // Fail on incorrect
        fail_faulty = true;
        break;

      default:
        goto help;
    }
  }

  // There should not be any extra arguments to be parsed,
  // otherwise this likely is a malformed input
  if (optind < argc) {
    goto help;
  }

  if (min_tier > max_tier) {
    printf("min-tier (%d) cannot be larger than max-tier (%d).\n", min_tier,
           max_tier);
    goto help;
  }

  if (min_tier + linear_tiers > max_tier) {
    printf(
        "min-tier (%d) + linear-tiers (%d) cannot be larger than max-tier "
        "(%d).\n",
        min_tier, linear_tiers, max_tier);
    goto help;
  }

  // Execute the respective tester function based on the CLI input
  switch (test_type) {
    case TEST_FILE: {
      // The `fname` is a required argument
      if (fname == NULL) {
        goto help;
      }

      // Whether to disregard the output or not
      if (!output_fname) {
        bool result = run_tester(fname, rotate_bit_matrix);
        printf("Result: %s\n", result ? PASS_STR : FAIL_STR);
      } else {
        bool result = run_tester_save_output(fname, output_fname,
                                             rotate_bit_matrix, true);
        printf("Result: %s\n", result ? PASS_STR : FAIL_STR);
      }

      break;
    }
    case TEST_GENERATED: {
      // The `N` is a required argument
      if (N == 0) {
        goto help;
      }

      bool result = run_tester_generated_bit_matrix(rotate_bit_matrix, N);

      printf("Result: %s\n", result ? PASS_STR : FAIL_STR);

      break;
    }
    case TEST_CORRECTNESS: {
      const bits_t START_SIZE = 64;

      bool correctness = run_correctness_tester(rotate_bit_matrix, START_SIZE);
      if (correctness)
        printf(PASS_STR ": Congrats! You pass all correctness tests\n");
      else
        printf(FAIL_STR ": Too bad. You have to fix bugs :'(\n");
      
      if (fail_faulty)
        assert(correctness);

      break;
    }
    case TEST_TIERS: {
      if (max_tier == -1) {
        max_tier = DEFAULT_MAX_TIER;
      }

      printf("FYI: the max tier you can be graded on is %d.\n", MAX_TIER_ALLOW);

      uint32_t tier = run_tester_tiers(rotate_bit_matrix, TIER_TIMEOUT, TIMEOUT,
                                       START_SIZE, GROWTH_RATE, min_tier,
                                       max_tier, linear_tiers, blowthroughs);

      if (tier == -1) {
        printf(FAIL_STR ": too slow for any tiers\n");
        if (min_tier > 0) printf("      try decreasing the minimum tier\n");
      } else {
        printf("Result: reached tier %d\n", tier);
      }

      break;
    }
    default:
      // If the `test_type` was not set, this is malformed input
      goto help;
  }

  // Success!
  return 0;

help:
  printf(
      "usage:\n"
      "\t"
      "-t {file|generated|       \t Select a test type                    \t "
      "Required to select test type\n"
      "\t"
      "    correctness|tiers}\n"
      "\t"
      "-f file-name              \t Input file name                       \t "
      "Required for \"file\" test type\n"
      "\t"
      "-o output-file-name       \t Output file name                      \t "
      "Optional for \"file\" test type\n"
      "\t"
      "-N dimension              \t Generated image dimension             \t "
      "Required for \"generated\" test type\n"
      "\t"
      "-m min-tier               \t Minimum tier                          \t "
      "Optional for \"tiers\" test type. Default is 0.\n"
      "\t"
      "-l linear-tiers           \t Number of tiers to search linearly    \t "
      "Optional for \"tiers\" test type. Default is %d. Set to -1 for all "
      "tiers to be linearly searched. \n"
      "\t"
      "-M max-tier               \t Maximum tier                          \t "
      "Optional for \"tiers\" test type. Default is %d. Maximum is %d.\n"
      "\t"
      "-x                        \t Fail for incorrect                    \t "
      "Optional for \"correctness\" test type. Fails with non-zero exit code "
      "if faulty.\n"
      "\t"
      "-h                        \t This help message\n",
      DEFAULT_LINEAR_TIERS, DEFAULT_MAX_TIER, MAX_TIER_ALLOW);

  return 1;
}
