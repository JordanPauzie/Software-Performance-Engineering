/**
 * Author: Isabel Rosa, isarosa@mit.edu, 
 * Jay Hilton, jhilton@mit.edu, 
 * Krit Boonsiriseth, talkon@mit.edu
 **/

#include <assert.h>
#include <cilk/cilk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../common/render.h"
#include "../include/misc_utils.h"
#include "../include/libstaff.h"

renderer_state_t* init_renderer_libstaff(const renderer_spec_t *spec) {
  renderer_state_t *state = (renderer_state_t*)malloc(sizeof(renderer_state_t));
  state->r_spec = *spec;
  int n_pixels = state->r_spec.resolution * state->r_spec.resolution;
  state->img = calloc(3ull * (size_t) n_pixels, sizeof(float));
  assert(state->img != NULL);
  return state;
}

void destroy_renderer_libstaff(renderer_state_t *state) {
  free(state->img);
  free(state);
}

// Computes the ray from a given origin (usually the eye location) to the pixel (x, y)
// in image coordinates.
ray_t origin_to_pixel_libstaff(renderer_state_t *state, int x, int y) {
  ray_t viewingRay;

  const float pixel_size = state->r_spec.viewport_size / state->r_spec.resolution;
  
  // Center image frame
  float us = -state->r_spec.resolution / 2 + x;
  float vs = -state->r_spec.resolution / 2 + y;

  viewingRay.origin = state->r_spec.eye;
  viewingRay.dir =
    qsubtract_libstaff(
      qadd_libstaff(
        scale_libstaff(us * pixel_size, state->r_spec.proj_plane_u), 
        scale_libstaff(vs * pixel_size, state->r_spec.proj_plane_v)
      ),
      viewingRay.origin
    );
  viewingRay.dir = scale_libstaff(1 / qsize_libstaff(viewingRay.dir), viewingRay.dir);

  return viewingRay;
}

// Determines whether the ray r and the sphere s intersect.
// 
// If the ray and the sphere intersect, writes the distance to the closer intersection
// to `out`, and returns 1. Otherwise, returns 0.
int ray_sphere_intersection_libstaff(ray_t *r, const sphere_t* s, float *out) {
  vector_t dist = qsubtract_libstaff(r->origin, s->pos);
  // Uses quadratic formula to compute intersection
  float a = qdot_libstaff(r->dir, r->dir);
  float b = 2 * qdot_libstaff(r->dir, dist);
  float c = (float)((double)qdot_libstaff(dist, dist) - (double)(s->r * s->r));
  float discr = (float)((double)(b * b) - (double)(4 * a * c));

  if (discr >= 0) {
    // Ray hits sphere
    float sqrtdiscr = sqrtf(discr);

    float min_dist;
    if (b >= 0) {
      float sol1 = (float)((double)-b - (double)sqrtdiscr) / (2 * a);
      float sol2 = (float)(2 * (double)c) / ((double)-b - (double)sqrtdiscr);
      min_dist = min_libstaff(sol1, sol2);
    } else {
      float sol1 = (float)(2 * (double)c) / ((double)-b + (double)sqrtdiscr);
      float sol2 = (float)((double)-b + (double)sqrtdiscr) / (2 * a);
      min_dist = min_libstaff(sol1, sol2);
    }

    // If new_t > 0 and smaller than original t, we
    // found a new, closer ray-sphere intersection
    if (min_dist > 0) {
      *out = min_dist;
      return 1;
    }
  }

  return 0;
}

// Sorts given spheres by length of the tangent (NOT in-place). 
// Returns a pointer to the sorted spheres. Returned pointer must be freed.
//
// Since the spheres are non-intersecting, this ensures that 
// if sphere S comes before sphere T in this ordering, then 
// sphere S is in front of sphere T in the rendering.
sphere_t* sort_libstaff(renderer_state_t *state, const sphere_t *spheres, int n_spheres) {
  sphere_t key;
  sphere_t* out = clone_libstaff(spheres, sizeof(sphere_t) * n_spheres);
  for (int i = 1; i < n_spheres; i++) {
    key = out[i];
    int j = i - 1;

    float d_ke = qdist_libstaff(key.pos, state->r_spec.eye);
    float r_k = key.r;
    
    while (j >= 0 && 
          (qdist_libstaff(out[j].pos, state->r_spec.eye) * qdist_libstaff(out[j].pos, state->r_spec.eye)
          - out[j].r * out[j].r) > 
          (d_ke * d_ke - r_k * r_k)) {
      out[j + 1] = out[j];
      j = j - 1;
    }
    out[j + 1] = key;
  }

  return out;
}

void set_pixel_libstaff(renderer_state_t *state, int x, int y, float red, float green, float blue) {
  state->img[(x + y * state->r_spec.resolution) * 3 + 0] = min_libstaff((float)red, 1.0);
  state->img[(x + y * state->r_spec.resolution) * 3 + 1] = min_libstaff((float)green, 1.0);
  state->img[(x + y * state->r_spec.resolution) * 3 + 2] = min_libstaff((float)blue, 1.0);
}

const float* render_libstaff(renderer_state_t *state, const sphere_t *spheres, int n_spheres) {
  sphere_t* sorted_spheres = sort_libstaff(state, spheres, n_spheres);
  assert(sorted_spheres != NULL);
  ray_t r;

  for (int y = 0; y < state->r_spec.resolution; y++) {
    for (int x = 0; x < state->r_spec.resolution; x++) {
      r = origin_to_pixel_libstaff(state, x, y);

      // Finds the first ray-sphere intersection.
      // Since spheres are sorted, the first intersection will also
      // be the closest intersection.
      float t = INFINITY;
      int currentSphere = -1;

      for (int i = 0; i < n_spheres; i++) {
        if (ray_sphere_intersection_libstaff(&r, &sorted_spheres[i], &t)) {
          currentSphere = i;
          break;
        }
      }

      // If ray does not intersect any sphere, color the pixel black
      if (currentSphere == -1) {
        set_pixel_libstaff(state, x, y, 0, 0, 0);
        continue;
      }

      material_t currentMat = sorted_spheres[currentSphere].mat;

      vector_t intersection = qadd_libstaff(r.origin, scale_libstaff(t, r.dir));

      // Normal vector at intersection point, perpendicular to the surface
      // of the sphere
      vector_t normal = qsubtract_libstaff(intersection, sorted_spheres[currentSphere].pos);
      float n_size = qsize_libstaff(normal);
      // Note: n_size should be the radius of the sphere, which is nonzero.
      normal = scale_libstaff(1 / n_size, normal);

      double red = 0;
      double green = 0;
      double blue = 0;

      for (int j = 0; j < state->r_spec.n_lights; j++) {
        light_t currentLight = state->r_spec.lights[j];
        vector_t intersection_to_light = qsubtract_libstaff(currentLight.pos, intersection);
        if (qdot_libstaff(normal, intersection_to_light) <= 0)
          continue;

        ray_t lightRay;
        lightRay.origin = intersection;
        lightRay.dir = scale_libstaff(1 / qsize_libstaff(intersection_to_light), intersection_to_light);

        // Calculate Lambert diffusion
        float lambert = qdot_libstaff(lightRay.dir, normal);
        red += (double)(currentLight.intensity.red * currentMat.diffuse.red *
                        lambert);
        green += (double)(currentLight.intensity.green *
                          currentMat.diffuse.green * lambert);
        blue += (double)(currentLight.intensity.blue * currentMat.diffuse.blue *
                         lambert);
      }

      set_pixel_libstaff(state, x, y, red, green, blue);
    }
  }
  free(sorted_spheres);
  return state->img;
}
