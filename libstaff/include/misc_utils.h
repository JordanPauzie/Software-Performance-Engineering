/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/

#ifndef MISC_UTILS_H_libstaff
#define MISC_UTILS_H_libstaff

#include <stddef.h>
#include "../../common/types.h"

// CHANGING THE CODE IN THIS FILE CAN RESULT IN CHANGING THE OUTPUT OF STAFF CODE,
// MEANING THAT THE REFERENCE TESTS MAY NOT BE ACCURATE, PERHAPS IN SUBTLE WAYS. 
// YOU MAY GET WEIRD FLOATING-POINT ERROR WHEN STAFF TESTS OCCUR. OPTIMIZING 
// THESE SEPARATELY IS A SAFER APPROACH, IF YOU'D LIKE.

#define min_libstaff(a, b) (((a) < (b)) ? (a) : (b))
#define max_libstaff(a, b) (((a) > (b)) ? (a) : (b))

vector_t qsubtract_libstaff(vector_t v1, vector_t v2);

float qdot_libstaff(vector_t v1, vector_t v2);

vector_t qcross_libstaff(vector_t v1, vector_t v2);

float qsize_libstaff(vector_t v);

vector_t scale_libstaff(float c, vector_t v1);

vector_t qadd_libstaff(vector_t v1, vector_t v2);

float qdist_libstaff(vector_t v1, vector_t v2);

/**
 * @brief Copy the first nbytes of src into a new, heap-allocated array, and return the new array. Returns NULL if allocation fails.
 * 
 * @param src array to copy
 * @param nbytes number of bytes to copy from src
 * @return the new, heap-allocated array, or NULL on allocation failure
 */
void *clone_libstaff(const void *src, size_t nbytes);

renderer_spec_t clone_renderer_spec_libstaff(const renderer_spec_t *src);

simulator_spec_t clone_simulator_spec_libstaff(const simulator_spec_t *src);

#endif // MISC_UTILS_H