/**
 * Authors: Jordan Pauzie (jpauzi01), Julian Savini (jsavin01)
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

typedef struct renderer_state {
  renderer_spec_t r_spec;
  float *img;
} renderer_state_t;

/********** init_renderer ********
 *
 * Initializes a renderer_state_t structure based on the provided
 * renderer_spec_t configuration. Allocates memory for the image buffer
 * and stores rendering specifications.
 *
 * Parameters:
 *      const renderer_spec_t *spec:  Rendering specification defining
 *                                    resolution, eye position, viewport, etc.
 *
 * Returns:
 *      Pointer to a newly allocated renderer_state_t with initialized
 *      image memory and copied specification.
 *
 * Expects:
 *      spec is not NULL.
 *      spec->resolution > 0.
 *      Sufficient memory is available for allocation.
 *
 ****************************/

renderer_state_t* init_renderer(const renderer_spec_t *spec) {
  renderer_state_t *state = (renderer_state_t*)malloc(sizeof(renderer_state_t));
  state->r_spec = *spec;
  int n_pixels = state->r_spec.resolution * state->r_spec.resolution;
  state->img = calloc(3ull * (size_t) n_pixels, sizeof(float));
  assert(state->img != NULL);
  return state;
}

/********** destroy_renderer ********
 *
 * Frees all memory allocated for a renderer_state_t, including the
 * internal image buffer.
 *
 * Parameters:
 *      renderer_state_t *state:  Renderer state to free.
 *
 * Returns:
 *      None.
 *
 * Expects:
 *      state is a valid pointer created by init_renderer().
 *
 ****************************/
void destroy_renderer(renderer_state_t *state) {
  free(state->img);
  free(state);
}

/********** origin_to_pixel ********
 *
 * Computes a normalized ray from the camera origin to the pixel (x, y)
 * in image coordinates. Used to determine the direction each pixel
 * represents in world space.
 *
 * Parameters:
 *      renderer_state_t *state:  Current renderer configuration.
 *      int x, y:                 Pixel coordinates.
 *
 * Returns:
 *      A ray_t representing the viewing ray from the eye to pixel (x, y).
 *
 * Expects:
 *      state is valid.
 *      0 <= x, y < state->r_spec.resolution.
 *
 ****************************/
ray_t origin_to_pixel(renderer_state_t *state, int x, int y) {
  const float pixel_size = state->r_spec.viewport_size / state->r_spec.resolution;

  // Center image frame
  float half_res = -state->r_spec.resolution / 2;
  
  ray_t viewingRay;
  viewingRay.origin = state->r_spec.eye;
  vector_t unscaled_dir =
    qsubtract(
      qadd(
        scale((half_res + x) * pixel_size, state->r_spec.proj_plane_u), 
        scale((half_res + y) * pixel_size, state->r_spec.proj_plane_v)
      ),
      viewingRay.origin
    );
  viewingRay.dir = scale(1 / qsize(unscaled_dir), unscaled_dir);

  return viewingRay;
}

/********** ray_sphere_intersection ********
 *
 * Determines whether a ray intersects a sphere. If so, the distance
 * to the closest intersection point is written to *out.
 *
 * Parameters:
 *      ray_t *r:          The ray to test.
 *      const sphere_t *s: The sphere to test against.
 *      float *out:        Pointer to store intersection distance.
 *
 * Returns:
 *      1 if the ray intersects the sphere, 0 otherwise.
 *
 * Expects:
 *      r, s, and out are valid pointers.
 *
 ****************************/
int ray_sphere_intersection(ray_t *r, const sphere_t* s, float *out) {
  vector_t dist = qsubtract(r->origin, s->pos);
  // Uses quadratic formula to compute intersection
  vector_t dir = r->dir;
  float a = qdot(dir, dir);
  float b = 2 * qdot(dir, dist);
  float rad = s->r;
  float c = (float)((double)qdot(dist, dist) - (double)(rad * rad));
  float discr = (float)((double)(b * b) - (double)(4 * a * c));

  if (discr >= 0) {
    // Ray hits sphere
    float sqrtdiscr = sqrtf(discr);

    float min_dist;
    if (b >= 0) {
      min_dist = min((float)((double)-b - (double)sqrtdiscr) / (2 * a), (float)(2 * (double)c) / ((double)-b - (double)sqrtdiscr));
    } else {
      min_dist = min((float)(2 * (double)c) / ((double)-b + (double)sqrtdiscr), (float)((double)-b + (double)sqrtdiscr) / (2 * a));
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

/********** metric ********
 *
 * Computes a sorting metric for a sphere relative to the camera,
 * based on squared distance minus radius squared.
 *
 * Parameters:
 *      sphere_t s:                The sphere.
 *      renderer_state_t *state:   Current renderer configuration.
 *
 * Returns:
 *      A float value used for sorting spheres by depth.
 *
 * Expects:
 *      state is valid.
 *
 ****************************/
float metric(sphere_t s, renderer_state_t *state) {
  float d = qdist(s.pos, state->r_spec.eye);
  float r = s.r;
  return (d * d ) - (r * r);
}

/********** sort ********
 *
 * Sorts an array of spheres based on their depth relative to the eye.
 * Uses parallel mergesort for large inputs, serial insertion sort for small ones.
 *
 * Parameters:
 *      renderer_state_t *state:  Current renderer configuration.
 *      const sphere_t *spheres:  Array of spheres to sort.
 *      int n_spheres:            Number of spheres.
 *
 * Returns:
 *      Newly allocated sorted array of spheres (must be freed by caller).
 *
 * Expects:
 *      spheres is valid.
 *      n_spheres > 0.
 *
 ****************************/
sphere_t* sort(renderer_state_t *state, const sphere_t *spheres, int n_spheres) {
  if (n_spheres <= 16) {
    // Base case: small arrays, just sort serially (insertion sort)
    sphere_t* out = clone(spheres, sizeof(sphere_t) * n_spheres);
    for (int i = 1; i < n_spheres; i++) {
      sphere_t key = out[i];
      float key_metric = metric(key, state);
      int j = i - 1;
      while (j >= 0 && metric(out[j], state) > key_metric) {
        out[j + 1] = out[j];
        j--;
      }
      out[j + 1] = key;
    }
    return out;
  }

  int mid = n_spheres / 2;
  sphere_t* left;
  cilk_spawn left = sort(state, spheres, mid);
  sphere_t* right = sort(state, spheres + mid, n_spheres - mid);
  cilk_sync;

  // Merge step
  sphere_t* out = malloc(sizeof(sphere_t) * n_spheres);
  int i = 0, j = 0, k = 0;
  while (i < mid && j < n_spheres - mid) {
    if (metric(left[i], state) <= metric(right[j], state)) {
      out[k++] = left[i++];
    } else {
      out[k++] = right[j++];
    }
  }
  while (i < mid) out[k++] = left[i++];
  while (j < n_spheres - mid) out[k++] = right[j++];

  free(left);
  free(right);

  return out;
}

/********** set_pixel ********
 *
 * Sets the RGB color of a pixel in the renderer image buffer,
 * clamping values between 0 and 1.
 *
 * Parameters:
 *      renderer_state_t *state:  Current renderer state.
 *      int x, y:                 Pixel coordinates.
 *      float red, green, blue:   Color components.
 *
 * Returns:
 *      None. Updates state->img in place.
 *
 * Expects:
 *      state->img is valid.
 *      0 <= x, y < resolution.
 *
 ****************************/
void set_pixel(renderer_state_t *state, int x, int y, float red, float green, float blue) {
  state->img[(x + y * state->r_spec.resolution) * 3 + 0] = min((float)red, 1.0);
  state->img[(x + y * state->r_spec.resolution) * 3 + 1] = min((float)green, 1.0);
  state->img[(x + y * state->r_spec.resolution) * 3 + 2] = min((float)blue, 1.0);
}

/********** compute_bounds ********
 *
 * Computes the conservative pixel-space bounding box for a given sphere.
 * Used to limit per-sphere rendering work to affected pixels.
 *
 * Parameters:
 *      const renderer_state_t *state:  Renderer state.
 *      const sphere_t *sphere:         Sphere to compute bounds for.
 *      int *min_x, *min_y:             Output lower bounds.
 *      int *max_x, *max_y:             Output upper bounds.
 *
 * Returns:
 *      None. Writes computed bounds into provided pointers.
 *
 * Expects:
 *      All pointer arguments are valid.
 *
 ****************************/
void compute_bounds(const renderer_state_t *state, const sphere_t *sphere, int *min_x, int *min_y, int *max_x, int *max_y) {
  int res = state->r_spec.resolution;
  float viewport_half = state->r_spec.viewport_size / 2.0f;

  vector_t e = state->r_spec.eye;
  vector_t u = state->r_spec.proj_plane_u;
  vector_t v = state->r_spec.proj_plane_v;
  vector_t n = qcross(u, v);

  // Distance from eye to projection plane
  float d_p = fabs(qdot(n, qsubtract((vector_t){0,0,0}, e)));

  // Distance from eye to sphere center
  vector_t eye_to_center = qsubtract(sphere->pos, e);
  float d_c = fabs(qdot(n, eye_to_center));

  vector_t projected = qadd(e, scale(d_p / d_c, eye_to_center));

  // Convert to pixel coordinates (center can be offscreen)
  int cx = (int)roundf((((qdot(projected, u)) / viewport_half) + 1.0f) * (res / 2.0f) - 0.5f);
  int cy = (int)roundf((((qdot(projected, v)) / viewport_half) + 1.0f) * (res / 2.0f) - 0.5f);

  // Projected radius in pixels
  int r_pixels = (int)ceilf((sphere->r * (d_p / d_c)) / (state->r_spec.viewport_size / (float)res)) * 1.5;

  // Compute bounds with padding
  *min_x = fmax(0, cx - r_pixels);
  *max_x = fmin(res, cx + r_pixels);
  *min_y = fmax(0, cy - r_pixels);
  *max_y = fmin(res, cy + r_pixels);
}

/********** render ********
 *
 * Renders the given list of spheres into the image buffer using
 * Lambertian lighting and parallelization (Cilk). 
 *
 * Parameters:
 *      renderer_state_t *state:  Renderer configuration and output buffer.
 *      const sphere_t *spheres:  Array of spheres to render.
 *      int n_spheres:            Number of spheres.
 *
 * Returns:
 *      Pointer to the rendered RGB float buffer.
 *
 * Expects:
 *      All inputs valid, spheres non-overlapping.
 *      state->img allocated.
 *
 ****************************/
const float* render(renderer_state_t *state, const sphere_t *spheres, int n_spheres) {
  sphere_t* sorted_spheres = sort(state, spheres, n_spheres);
  int res = state->r_spec.resolution;
  bool *filled = calloc(res * res, sizeof(bool));
  ray_t* rays = malloc(sizeof(ray_t) * res * res);

  cilk_for (int y = 0; y < res; y++) {
    for (int x = 0; x < res; x++) {
      rays[x + y * res] = origin_to_pixel(state, x, y);
      int idx = (x + y * res) * 3;
      state->img[idx + 0] = 0.0f;
      state->img[idx + 1] = 0.0f;
      state->img[idx + 2] = 0.0f;
    }
  }

  for (int i = 0; i < n_spheres; i++) {
    sphere_t current_sphere = sorted_spheres[i];

    int min_x, min_y, max_x, max_y;
    compute_bounds(state, &current_sphere, &min_x, &min_y, &max_x, &max_y);   

    cilk_for (int y = min_y; y < max_y; y++) {
      for (int x = min_x; x < max_x; x++) {
        int index = x + y * res;
        if (filled[index]) continue;

        ray_t r = rays[x + y * res];
        float t = INFINITY;

        if (!ray_sphere_intersection(&r, &current_sphere, &t)) {
          continue; 
        }

        vector_t intersection = qadd(r.origin, scale(t, r.dir));

        vector_t normal = qsubtract(intersection, current_sphere.pos);
        normal = scale(1 / qsize(normal), normal);

        material_t currentMat = current_sphere.mat;

        double red = 0;
        double green = 0;
        double blue = 0;

        for (int j = 0; j < state->r_spec.n_lights; j++) {
          light_t currentLight = state->r_spec.lights[j];
          vector_t intersection_to_light = qsubtract(currentLight.pos, intersection);

          // Lambert is dot product of normalized light dir with normal
          float lambert = qdot(scale(1.0f / sqrtf(qdot(intersection_to_light, intersection_to_light)), intersection_to_light), normal);
          if (lambert > 0.0f) {
            red += (double)(currentLight.intensity.red * currentMat.diffuse.red * lambert);
            green += (double)(currentLight.intensity.green * currentMat.diffuse.green * lambert);
            blue += (double)(currentLight.intensity.blue * currentMat.diffuse.blue * lambert);
          }
        }
        set_pixel(state, x, y, red, green, blue);
        filled[index] = true;
      }
    }
  }

  free(rays);
  free(filled);
  free(sorted_spheres);
  return state->img;
}