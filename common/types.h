/**
 * Authors: Isabel Rosa, isrosa@mit.edu and Jay Hilton, jhilton@mit.edu
 **/

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef struct {
  float x, y, z;
} vector_t;

typedef struct {
  float red, green, blue;
} color_t;

typedef struct {
  color_t diffuse;
  float reflection;
} material_t;

typedef struct {
  vector_t pos;
  vector_t vel;
  vector_t accel;
  float r;
  float mass;
  material_t mat;
} sphere_t;

typedef struct {
  vector_t origin;
  vector_t dir;
} ray_t;

typedef struct {
  vector_t pos;
  color_t intensity;
} light_t;

/**
 * @brief A representation of the input to start a simulation.
 *
 * For any value of spheres p, p must not be written to from anywhere other than
 * simulate and sort while init_simulate was most recently called with a spec
 * whose spheres pointer aliases p. The practical implication of this is that
 * all state based on the provided simulation spec at initialization only has
 * to be maintained for mutations that come from simulate and sort.
 */
typedef struct {
  // The spheres to simulate.
  const sphere_t *spheres;
  // The number of spheres to simulate; reading from spheres[0] to
  // spheres[n_spheres - 1] is safe.
  int n_spheres;

  // Gravitational acceleration.
  double g;
} simulator_spec_t;

/**
 * @brief A representation of the input to initialize sphere rendering.
 */
typedef struct {
  // The image resolution, in pixels. (Both image height and width are 
  // equal to this value.)
  int resolution;

  // The position of the eye.
  vector_t eye;

  // Vectors spanning and defining the projection plane.
  vector_t proj_plane_u, proj_plane_v;
  // The viewport size, in basis vector units on the projection plane.
  float viewport_size;

  // The number of lights used for rendering.
  int n_lights;
  // The lights to render with; reading from lights[0] to lights[n_lights - 1]
  // is safe.
  const light_t *lights;
} renderer_spec_t;

/**
 * @brief A representation of the state of the simulator
 * 
 * A forward declaration of the state of the simulator that
 * students will implement
*/
struct simulator_state;

/**
 * @brief A representation of the state of the renderer
 * 
 * A forward declaration of the state of the renderer that
 * students will implement
*/
struct renderer_state;

// Functions for comparing various types for equality.

bool vector_eq(const vector_t *a, const vector_t *b);

bool color_eq(const color_t *a, const color_t *b);

bool material_eq(const material_t *a, const material_t *b);

bool sphere_eq(const sphere_t *a, const sphere_t *b);

bool ray_eq(const ray_t *a, const ray_t *b);

bool light_eq(const light_t *a, const light_t *b);

/**
 * @brief De-allocate any memory associated with spec. Does not free `spec`
 * itself.
 *
 * @param spec the simulation spec to destroy
 */
void destroy_simulator_spec(simulator_spec_t *spec);

/**
 * @brief De-allocate any memory associated with spec. Does not free spec
 * itself.
 *
 * @param spec the render spec to destroy
 */
void destroy_renderer_spec(renderer_spec_t *spec);

#endif // TYPES_H
