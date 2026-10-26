#ifndef LIBSTAFF_H
#define LIBSTAFF_H

#include <stddef.h>
#include "../../common/types.h"

typedef struct renderer_state {
  renderer_spec_t r_spec;
  float *img;
} renderer_state_t;

typedef struct simulator_state {
  simulator_spec_t s_spec;
  sphere_t *spheres;
} simulator_state_t;

renderer_state_t* init_renderer_libstaff(const renderer_spec_t *spec);
void destroy_renderer_libstaff(renderer_state_t* state);
const float* render_libstaff(renderer_state_t *state, const sphere_t *spheres, int n_spheres);
simulator_state_t* init_simulator_libstaff(const simulator_spec_t *spec);
void destroy_simulator_libstaff(simulator_state_t* state);
sphere_t* simulate_libstaff(simulator_state_t* state);

#endif