/**
 * Authors: Isabel Rosa, isrosa@mit.edu and Jay Hilton, jhilton@mit.edu
 **/
 
#include "../include/misc_utils.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// CHANGING THE CODE IN THIS FILE CAN RESULT IN CHANGING THE OUTPUT OF STAFF CODE,
// MEANING THAT THE REFERENCE TESTS MAY NOT BE ACCURATE, PERHAPS IN SUBTLE WAYS. 
// YOU MAY GET WEIRD FLOATING-POINT ERROR WHEN STAFF TESTS OCCUR. OPTIMIZING 
// THESE SEPARATELY IS A SAFER APPROACH, IF YOU'D LIKE.

vector_t qsubtract_libstaff(vector_t v1, vector_t v2) {
  vector_t v;
  v.x = (float)((double)v1.x - (double)v2.x);
  v.y = (float)((double)v1.y - (double)v2.y);
  v.z = (float)((double)v1.z - (double)v2.z);
  return v;
}

float qdot_libstaff(vector_t v1, vector_t v2) {
  float x = v1.x * v2.x;
  float y = v1.y * v2.y;
  float z = v1.z * v2.z;
  return (float)((double)x + (double)y + (double)z);
}

vector_t qcross_libstaff(vector_t v1, vector_t v2) {
  vector_t v;
  v.x = (float)((double)(v1.y * v2.z) - (double)(v1.z * v2.y));
  v.y = (float)((double)(v1.z * v2.x) - (double)(v1.x * v2.z));
  v.z = (float)((double)(v1.x * v2.y) - (double)(v1.y * v2.x));
  return v;
}

float qsize_libstaff(vector_t v) {
  float x = v.x * v.x;
  float y = v.y * v.y;
  float z = v.z * v.z;
  return sqrt((float)((double)x + (double)y + (double)z));
}

vector_t scale_libstaff(float c, vector_t v1) {
  vector_t v;
  v.x = v1.x * c;
  v.y = v1.y * c;
  v.z = v1.z * c;
  return v;
}

vector_t qadd_libstaff(vector_t v1, vector_t v2) {
  vector_t v;
  v.x = (float)((double)v1.x + (double)v2.x);
  v.y = (float)((double)v1.y + (double)v2.y);
  v.z = (float)((double)v1.z + (double)v2.z);
  return v;
}

float qdist_libstaff(vector_t v1, vector_t v2) {
  double f = ((double)v1.x - (double)v2.x) * ((double)v1.x - (double)v2.x);
  f += ((double)v1.y - (double)v2.y) * ((double)v1.y - (double)v2.y);
  f += ((double)v1.z - (double)v2.z) * ((double)v1.z - (double)v2.z);
  return sqrt((float)f);
}

void *clone_libstaff(const void *src, size_t nbytes) {
  void *dst = malloc(nbytes);
  if (dst == NULL) return NULL;
  memcpy(dst, src, nbytes);
  return dst;
}

renderer_spec_t clone_renderer_spec_libstaff(const renderer_spec_t *src) {
  renderer_spec_t spec = *src;
  spec.lights = clone_libstaff(src->lights, sizeof(light_t) * src->n_lights);
  return spec;
}

simulator_spec_t clone_simulator_spec_libstaff(const simulator_spec_t *src) {
  simulator_spec_t spec = *src;
  spec.spheres = clone_libstaff(src->spheres, sizeof(sphere_t) * src->n_spheres);
  return spec;
}