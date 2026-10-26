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

/**
 * allocator.h
 *
 * You may include macros, inline helper function definitions, and helper
 * function prototypes here instead of in allocator.c, especially if you like to
 * test them in allocator_test.c.
 **/

#ifndef MM_ALLOCATOR_H
#define MM_ALLOCATOR_H

// All blocks must have a specified minimum alignment.
// The alignment requirement (from config.h) is >= 16 bytes.
#ifndef ALIGNMENT
#define ALIGNMENT 16
#endif

// Rounds up to the nearest multiple of ALIGNMENT.
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))

// The smallest aligned size that will hold a size_t value.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// blocks are 4 bytes to represent the block size N, 8 bytes for the free_list 
// next pointer, 4 bytes of padding to hit 16-byte alignment, then N - 16 bytes of
// payload (available to be used by application as heap memory)

static inline void *next_block(void *free) {
    if (free == NULL) return NULL;
    return *(void **)((char *)free + sizeof(uint32_t));
}

static inline void set_next_block(void *free, void *block) {
    *(void **)((char *)free + sizeof(uint32_t)) = block;
}

// TODO - consider slide 29 for mitigating data fragmentation
static inline void *allocate_block(void **free_list) {
    void *block = *free_list;
    *free_list = next_block(block);
    return (char *)block + ALIGNMENT; // skip header to payload
}

static inline uint32_t block_size(void *free_block) {
    return *(uint32_t *)free_block;
}

static inline void free_block(void **free_list, void *block) {
    set_next_block(block, *free_list);
    *free_list = block;
}

#endif  // MM_ALLOCATOR_H
