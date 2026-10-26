/**
 * Copyright (c) 2015 MIT License by 6.172 Staff
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

#include "./allocator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "./allocator_interface.h"
#include "./memlib.h"

#define NUM_BINS 60
#define BASE 1.5
#define MIN_BLOCK_SIZE 32

static uint32_t bin_sizes[NUM_BINS];

static void init_binsizes() {
    double size = 32;
    for (int i = 0; i < NUM_BINS; i++) {
        bin_sizes[i] = ALIGN((uint32_t)size);
        size *= BASE;
    }
}

static inline int get_bin(uint32_t n) {
  for (int i = 0; i < NUM_BINS; i++) {
      if (bin_sizes[i] >= n) return i;
  }
  return NUM_BINS - 1;
}

static inline uint32_t bin_to_blocksize(int bin) {
  return bin_sizes[bin];
}

void print_block(void *block) {
  int payload_size = block_size(block) - ALIGNMENT;
  printf("[%p -> %p (%d bytes)]", (char *)block + ALIGNMENT, (char *)block + ALIGNMENT + payload_size, payload_size);
}

void print_bin(void *bin, int k) {
  printf("Blocks of Size %d: ", bin_to_blocksize(k));
  void *curr = bin;
  while (curr != NULL) {
    print_block(curr);
    curr = next_block(curr);
    if (curr != NULL) {
      printf(" -> ");
    }
    if (curr == next_block(curr)) {
      break;
    }
  }
  printf("\n");
}

void print_bfl() {
  printf("====================\n");
  void **bins = (void **)mem_heap_lo();
  for (int i = 0; i < NUM_BINS; i++) {
    print_bin(bins[i], i);
  }
}


// Don't call libc malloc!
// #define malloc(...) (USE_MY_MALLOC)
// #define free(...) (USE_MY_FREE)
// #define realloc(...) (USE_MY_REALLOC)

// check - This checks our invariant that the size_t header before every
// block points to either the beginning of the next block, or the end of the
// heap.
int my_check() {
  char* p;
  char* lo = (char*)mem_heap_lo();
  char* hi = (char*)mem_heap_hi() + 1;
  size_t size = 0;

  p = lo;
  while (lo <= p && p < hi) {
    size = ALIGN(*(size_t*)p + SIZE_T_SIZE);
    p += size;
  }

  if (p != hi) {
    printf("Bad headers did not end at heap_hi!\n");
    printf("heap_lo: %p, heap_hi: %p, size: %lu, p: %p\n", lo, hi, size, p);
    return -1;
  }

  return 0;
}

static inline void *malloc_free_block(int bin) {
  unsigned int size = ALIGN(bin_to_blocksize(bin));
  void *result = mem_sbrk(size);
  *(uint32_t*)result = size;
  set_next_block(result, NULL);
  return result;
}

// init - Initialize the malloc package.  Called once before any other
// calls are made.  Since this is a very simple implementation, we just
// return success.
int my_init() {
  init_binsizes();
  size_t bins_size = ALIGN(NUM_BINS * sizeof(void *));
  void *result = mem_sbrk(bins_size);
  assert(result == mem_heap_lo());

  void **bins = (void **)result;
  // set all bins to empty
  for (int i = 0; i < NUM_BINS; i++) {
    bins[i] = NULL;
  }
  return 0;
}

static void recursive_split(void *block, void **bins) {
  uint32_t cur_size = block_size(block);
  int cur_bin = get_bin(cur_size);

  // Compute small_size using your bin table (cur_bin - 1)
  if (cur_bin == 0) {
      // Can't split the smallest bin
      free_block(&bins[cur_bin], block);
      return;
  }

  uint32_t small_size = bin_to_blocksize(cur_bin - 1);

  // If we cannot split because the small child would be too small or invalid:
  if (small_size < MIN_BLOCK_SIZE || small_size >= cur_size) {
      free_block(&bins[cur_bin], block);
      return;
  }

  // Prepare split
  void *small_block = block;
  *(uint32_t *)small_block = small_size;

  uint32_t remainder_size = cur_size - small_size;
  void *remainder_block = (char *)block + small_size;
  *(uint32_t *)remainder_block = remainder_size;

  // Add small block directly to its bin
  free_block(&bins[cur_bin - 1], small_block);

  if (remainder_size < MIN_BLOCK_SIZE) {
      // remainder is too small to form a valid block
      // discard it and stop recursion
      return;
  }

  // Recursively split the remainder
  recursive_split(remainder_block, bins);
}

// take a big block, split out a block that fits into target_bin, then divvy up the remainder amongst 
// smaller bins so it doesn't go to waste
static void *split_out_block(void *big_block, int target_bin, void **bins) {
  uint32_t target_size = bin_to_blocksize(target_bin);
  uint32_t full_size = block_size(big_block);

  // Carve out the target block
  void *result = big_block;
  *(uint32_t*)result = target_size;

  uint32_t rem_size = full_size - target_size;

  if (rem_size >= MIN_BLOCK_SIZE) {
      void *rem_block = (char*)big_block + target_size;
      *(uint32_t*)rem_block = rem_size;

      // Split the leftover
      recursive_split(rem_block, bins);
  }

  // Return pointer to usable payload, not header
  return (char*)result + ALIGNMENT;
}

//  malloc - Allocate a block by incrementing the brk pointer.
//  Always allocate a block whose size is a multiple of the alignment.
void* my_malloc(size_t size) {
  // remember that the block is 16 bytes of header, then the payload
  int aligned_size = ALIGN(size + ALIGNMENT); // so this represents the size of the requested payload + header + padding for alignment
  
  int bin = get_bin(aligned_size);
  void **bins = (void **)mem_heap_lo();
  void *free = bins[bin];

  if (free == NULL) {
    // the free list was empty, so get it from bigger bin
    int found = -1;
    for (int i = bin; i < NUM_BINS; i++) {
      if (bins[i] != NULL) {
        // we found a free block of size 2^i
        found = i;
        break;
      }
    }
    if (found < 0) {
      // we need more memory
      void *block = (void*)((char*)malloc_free_block(bin) + ALIGNMENT);
      return block;
    }

   
    // Remove the large block that we will split
    void *big = allocate_block(&bins[found]);
    big = (char*)big - ALIGNMENT;


    // The target bin now has blocks available
    return split_out_block(big, bin, bins);
  } else {
    // we have a block in the free list, return that
    
  
    return allocate_block(&bins[bin]);
    
  }
  return NULL;
}

// free - Freeing a block does nothing.
void my_free(void* ptr) {
  void *block = (void*)((char*)ptr - ALIGNMENT);
  
  uint32_t size = *(uint32_t *)block;
  int bin = get_bin(size);

  void **bins = (void **)mem_heap_lo();
  free_block(&(bins[bin]), block);
}


// realloc - Implemented simply in terms of malloc and free
void* my_realloc(void* ptr, size_t size) {
  if (!ptr) return my_malloc(size);

  void* newptr;
  
  void *block_ptr = (void*)((char*)ptr - ALIGNMENT);
  uint32_t copy_size = block_size(block_ptr);

  // don't bother mallocing new memory if its all still going to fit in the same block
  int aligned_size = ALIGN(size + ALIGNMENT);
  int bin = get_bin(aligned_size);
  int new_blocksize = bin_to_blocksize(bin);
  if (new_blocksize == copy_size) {
    return ptr;
  }

  // Allocate a new chunk of memory, and fail if that allocation fails.
  newptr = my_malloc(size);
  if (NULL == newptr) {
    return NULL;
  }

  // If the new block is smaller than the old one, we have to stop copying
  // early so that we don't write off the end of the new block of memory.
  if (size < copy_size) {
    copy_size = size;
  }

  // This is a standard library call that performs a simple memory copy.
  memcpy(newptr, ptr, copy_size);

  // Release the old block.
  my_free(ptr);

  // Return a pointer to the new block.
  return newptr;
}
