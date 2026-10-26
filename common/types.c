#include "./types.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// A brief note: some of these functions behave weirdly when their fields are
// NaN. This might cause issues if you're relying on these functions for
// semantic equality rather than bitwise equality.

bool vector_eq(const vector_t *a, const vector_t *b) {
  static_assert(sizeof(vector_t) == sizeof(float) * 3,
                "vector must be packed rep for memcmp to be correct");
  return memcmp(a, b, sizeof(vector_t)) == 0;
}

bool color_eq(const color_t *a, const color_t *b) {
  static_assert(sizeof(color_t) == sizeof(float) * 3,
                "color must be packed rep for memcmp to be correct");
  return memcmp(a, b, sizeof(color_t)) == 0;
}

bool material_eq(const material_t *a, const material_t *b) {
  return a->reflection == b->reflection && color_eq(&a->diffuse, &b->diffuse);
}

bool sphere_eq(const sphere_t *a, const sphere_t *b) {
  return a->mass == b->mass && a->r == b->r && material_eq(&a->mat, &b->mat) &&
         vector_eq(&a->pos, &b->pos) && vector_eq(&a->vel, &b->vel) &&
         vector_eq(&a->accel, &b->accel);
}

bool ray_eq(const ray_t *a, const ray_t *b) {
  return vector_eq(&a->dir, &b->dir) && vector_eq(&a->origin, &b->origin);
}

bool light_eq(const light_t *a, const light_t *b) {
  return color_eq(&a->intensity, &b->intensity) && vector_eq(&a->pos, &b->pos);
}

void destroy_simulator_spec(simulator_spec_t *spec) {
  free((sphere_t *)spec->spheres);
  spec->spheres = NULL;
}

void destroy_renderer_spec(renderer_spec_t *spec) {
  // You can't free a const pointer, so we cast away the const.
  free((light_t *)spec->lights);
  spec->lights = NULL;
}