#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "allocator_interface.h"
#include "memlib.h"

static int initialized = 0;

__attribute__((always_inline)) static void init() {
  if (initialized) return;
  initialized = 1;
  mem_init();
  my_init();
}

void* calloc(size_t count, size_t size) {
  init();
  void* ptr = malloc(count * size);
  assert(ptr && "calloc nomemory");
  bzero(ptr, count * size);
  return ptr;
}

void* malloc(size_t size) {
  init();
  void* ptr = my_malloc(size);
  assert(ptr);
  return ptr;
}

void free(void* ptr) { my_free(ptr); }

void* realloc(void* ptr, size_t size) {
  init();
  ptr = my_realloc(ptr, size);
  assert(ptr && "malloc no memory");
  return ptr;
}
