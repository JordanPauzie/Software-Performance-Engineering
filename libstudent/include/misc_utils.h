/**
 * Authors: Jordan Pauzie (jpauzi01), Julian Savini (jsavin01)
 **/
 
#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <stddef.h>
#include "../../common/types.h"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

vector_t qsubtract(vector_t v1, vector_t v2);

float qdot(vector_t v1, vector_t v2);

vector_t qcross(vector_t v1, vector_t v2);

float qsize(vector_t v);

vector_t scale(float c, vector_t v1);

vector_t qadd(vector_t v1, vector_t v2);

float qdist(vector_t v1, vector_t v2);

/**
 * @brief Copy the first nbytes of src into a new, heap-allocated array, and return the new array. Returns NULL if allocation fails.
 * 
 * @param src array to copy
 * @param nbytes number of bytes to copy from src
 * @return the new, heap-allocated array, or NULL on allocation failure
 */
void *clone(const void *src, size_t nbytes);

renderer_spec_t clone_renderer_spec(const renderer_spec_t *src);

simulator_spec_t clone_simulator_spec(const simulator_spec_t *src);

#endif // MISC_UTILS_H