/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/

#ifndef SIMULATE_H
#define SIMULATE_H

#include "./types.h"

#define EXPORT __attribute__((visibility("default")))

EXPORT
/**
 * @brief Initialize the simulator with the provided starting spec. May be
 * called more than once.
 *
 * @param[in] spec initial spec
 */
struct simulator_state* init_simulator(const simulator_spec_t *spec);

EXPORT
/**
 * @brief Reset the simulator, de-allocating any allocated memory.
 */
void destroy_simulator(struct simulator_state *state);

EXPORT
/**
 * @brief Advance the simulator by one time step.
 *
 * @return a pointer to the spheres to be rendered
 */
sphere_t* simulate(struct simulator_state *state);

#endif // SIMULATE_H
