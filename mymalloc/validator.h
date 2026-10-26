/*
 * validator.h - 6.172 Malloc Validator
 *
 * Validates a malloc/free/realloc implementation.
 *
 * Copyright (c) 2010, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */

#ifndef MM_VALIDATOR_H
#define MM_VALIDATOR_H

#include <stdint.h>

#include "./mdriver.h"

// Returns true if p is R_ALIGNMENT-byte aligned
#if (__WORDSIZE == 64)
#define IS_ALIGNED(p) ((((uint64_t)(p)) % R_ALIGNMENT) == 0)
#else
#define IS_ALIGNED(p) ((((uint32_t)(p)) % R_ALIGNMENT) == 0)
#endif

// Range list data structure

// Records the extent of each block's payload
typedef struct range_t {
  char* lo;              // low payload address
  char* hi;              // high payload address
  struct range_t* next;  // next list element
} range_t;

// eval_mm_valid - Check the malloc package for correctness
int eval_mm_valid(const malloc_impl_t* impl, trace_t* trace, int tracenum);
#endif  // MM_VALIDATOR_H
