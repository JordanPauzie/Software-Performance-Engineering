/**
 * Authors: Jordan Pauzie (jpauzi01), Julian Savini (jsavin01)
 * Optimized version exploiting force symmetry
 **/

#include <assert.h>
#include <cilk/cilk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../common/simulate.h"
#include "../include/misc_utils.h"
#include "../include/libstudent.h"

/********** init_simulator_libstudent ********
 *
 * Initializes a simulator_state_t and duplicates the sphere array
 * for double-buffering of positions and velocities.
 *
 * Parameters:
 *      const simulator_spec_t *spec:  Simulation parameters and spheres.
 *
 * Returns:
 *      Newly allocated simulator_state_t.
 *
 * Expects:
 *      spec is valid.
 *      spec->n_spheres > 0.
 *
 ****************************/
simulator_state_t* init_simulator_libstudent(const simulator_spec_t *spec) {
  simulator_state_t *state = (simulator_state_t*)malloc(sizeof(simulator_state_t));
  state->s_spec = *spec;
  state->spheres = malloc(2 * spec->n_spheres * sizeof(sphere_t));
  assert(state->spheres != NULL);
  memcpy(state->spheres, spec->spheres, sizeof(sphere_t) * spec->n_spheres);
  memcpy(state->spheres + state->s_spec.n_spheres, spec->spheres, sizeof(sphere_t) * state->s_spec.n_spheres);
  return state;
}

/********** destroy_simulator_libstudent ********
 *
 * Frees all dynamically allocated memory in a simulator_state_t.
 *
 * Parameters:
 *      simulator_state_t *state:  Simulator state to destroy.
 *
 * Returns:
 *      None.
 *
 * Expects:
 *      state was initialized with init_simulator_libstudent().
 *
 ****************************/
void destroy_simulator_libstudent(simulator_state_t* state) {
  free(state->spheres);
  free(state);
}

/********** update_accelerations_libstudent ********
 *
 * Updates accelerations for all spheres using Newtonian gravity.
 * Exploits pairwise symmetry when possible for optimization.
 *
 * Parameters:
 *      sphere_t *spheres:  Array of spheres (duplicated storage layout).
 *      int n_spheres:      Number of spheres.
 *      double g:           Gravitational constant.
 *
 * Returns:
 *      None. Writes acceleration values to second half of sphere array.
 *
 * Expects:
 *      spheres and n_spheres valid.
 *
 ****************************/
void update_accelerations_libstudent(sphere_t *spheres, int n_spheres, double g) {
  // Check for extreme mass ratios that could cause numerical issues, these cause floating rounding errors
  double max_mass = 0.0;
  double min_mass = 1e100;
  for (int i = 0; i < n_spheres; i++) {
    if (spheres[i].mass > max_mass) max_mass = spheres[i].mass;
    if (spheres[i].mass < min_mass) min_mass = spheres[i].mass;
  }
  
  double mass_ratio = max_mass / min_mass;
  
  // If mass ratio exceeds 100:1, use the original algorithm
  if (mass_ratio > 100.0) {
    cilk_for (int i = 0; i < n_spheres; i++) {
      double ax = 0, ay = 0, az = 0;
      
      for (int j = 0; j < n_spheres; j++) {
        if (i != j) {
          vector_t i_minus_j = qsubtract(spheres[i].pos, spheres[j].pos);
          double dist = qsize(i_minus_j);
          double dist_cubed = dist * dist * dist;
          
          vector_t j_minus_i = scale(-1, i_minus_j);
          vector_t force = scale(g * spheres[j].mass / dist_cubed, j_minus_i);
          
          ax += (double)force.x;
          ay += (double)force.y;
          az += (double)force.z;
        }
      }
      
      const vector_t v = {.x = ax, .y = ay, .z = az};
      spheres[i + n_spheres].accel = v;
    }
    return;
  }
  
  // exploit symmetry for normal mass distributions
  vector_t **force_cache = (vector_t**)malloc(n_spheres * sizeof(vector_t*));
  for (int i = 0; i < n_spheres; i++) {
    force_cache[i] = (vector_t*)calloc(n_spheres, sizeof(vector_t));
  }

  // Pre-compute all pairwise forces
  for (int i = 0; i < n_spheres; i++) {
    for (int j = i + 1; j < n_spheres; j++) {
      vector_t i_to_j = qsubtract(spheres[j].pos, spheres[i].pos);
      double distance = qsize(i_to_j);
      double distance_cubed = distance * distance * distance;
      
      vector_t force_direction = scale(g / distance_cubed, i_to_j);

      // Force that j exerts on i
      force_cache[i][j] = scale(spheres[j].mass, force_direction);
      
      // Force which i exerts on j
      force_cache[j][i] = scale(-spheres[i].mass, force_direction);
    }
  }

  // Now accumulate forces in the SAME ORDER as original algorithm
  cilk_for (int i = 0; i < n_spheres; i++) {
    double ax = 0, ay = 0, az = 0;
    
    for (int j = 0; j < n_spheres; j++) {
      if (i != j) {
        ax += (double)force_cache[i][j].x;
        ay += (double)force_cache[i][j].y;
        az += (double)force_cache[i][j].z;
      }
    }
    
    const vector_t v = {.x = ax, .y = ay, .z = az};
    spheres[i + n_spheres].accel = v;
  }

  // free cache
  for (int i = 0; i < n_spheres; i++) {
    free(force_cache[i]);
  }
  free(force_cache);
}

/********** update_velocities_libstudent ********
 *
 * Updates velocities for all spheres using the previously computed
 * accelerations and timestep t.
 *
 * Parameters:
 *      sphere_t *spheres:  Array of spheres (duplicated layout).
 *      int n_spheres:      Number of spheres.
 *      float t:            Time step.
 *
 * Returns:
 *      None. Writes updated velocities to second half of sphere array.
 *
 * Expects:
 *      spheres valid and accelerations computed.
 *
 ****************************/
void update_velocities_libstudent(sphere_t *spheres, int n_spheres, float t) {
  for (int i = 0; i < n_spheres; i++) {
    spheres[i + n_spheres].vel = qadd(spheres[i].vel, scale(t, spheres[i].accel));
  }
}

/********** update_positions_libstudent ********
 *
 * Updates positions for all spheres based on their velocities and timestep t.
 *
 * Parameters:
 *      sphere_t *spheres:  Array of spheres (duplicated layout).
 *      int n_spheres:      Number of spheres.
 *      float t:            Time step.
 *
 * Returns:
 *      None. Writes new positions to second half of sphere array.
 *
 * Expects:
 *      spheres valid and velocities computed.
 *
 ****************************/
void update_positions_libstudent(sphere_t *spheres, int n_spheres, float t) {
  for (int i = 0; i < n_spheres; i++) {
    spheres[i + n_spheres].pos = qadd(spheres[i].pos, scale(t, spheres[i].vel));
  }
}

/********** do_ministep_libstudent ********
 *
 * Advances the simulation by a given small timestep, updating all
 * accelerations, velocities, and positions. Handles sphere collisions
 * between indices i and j when specified.
 *
 * Parameters:
 *      sphere_t *spheres:  Array of spheres.
 *      int n_spheres:      Number of spheres.
 *      double g:           Gravitational constant.
 *      float minCollisionTime:  Time step to advance.
 *      int i, j:           Indices of colliding spheres (-1 if none).
 *
 * Returns:
 *      None. Updates sphere positions and velocities in place.
 *
 * Expects:
 *      spheres valid.
 *      n_spheres > 0.
 *      minCollisionTime >= 0.
 *
 ****************************/
void do_ministep_libstudent(sphere_t *spheres, int n_spheres, double g, float minCollisionTime, int i, int j) {
  update_accelerations_libstudent(spheres, n_spheres, g);
  update_velocities_libstudent(spheres, n_spheres, minCollisionTime);
  update_positions_libstudent(spheres, n_spheres, minCollisionTime);

  for (int k = 0; k < n_spheres; k++) {
    spheres[k] = spheres[k + n_spheres];
  }

  if (i == -1 || j == -1) {
    return;
  }

  vector_t distVec = qsubtract(spheres[i].pos, spheres[j].pos);
  float massSum = (float)((double)spheres[i].mass + (double)spheres[j].mass);
  float scale1 = 2 * spheres[j].mass / massSum;
  float scale2 = 2 * spheres[i].mass / massSum;
  float distNorm = qdot(distVec, distVec);
  vector_t velDiff = qsubtract(spheres[i].vel, spheres[j].vel);
  vector_t scaledDist = scale(qdot(velDiff, distVec) / distNorm, distVec);
  spheres[i].vel = qsubtract(spheres[i].vel, scale(scale1, scaledDist));
  spheres[j].vel = qsubtract(spheres[j].vel, scale(-1 * scale2, scaledDist));
}

// Check if the spheres at indices i and j collide in the next
// timeToCollision timesteps
// 
// If so, modifies timeToCollision to be the time until spheres i and j collide.
int check_for_collision_libstudent(sphere_t *spheres, int i, int j, float *timeToCollision) {
  vector_t distVec = qsubtract(spheres[i].pos, spheres[j].pos);
  float dist = qsize(distVec);
  float sumRadii = (float)((double)spheres[i].r + (double)spheres[j].r);

  // Shift frame of reference to act like sphere i is stationary
  // Not adjusting for acceleration because our simulation does not adjust for acceleration
  vector_t movevec = qsubtract(spheres[j].vel, spheres[i].vel);

  float moveSpeed = qsize(movevec);
  float moveDist = (float)((double)moveSpeed * (double)*timeToCollision);

  // Break if the length the sphere moves in timeToCollision time is less than
  // distance between the centers of these spheres minus their radii
  if ((double)moveDist < (double)dist - (double)sumRadii ||
      (movevec.x == 0 && movevec.y == 0 && movevec.z == 0)) {
    return 0;
  }

  vector_t unitMovevec = scale(1 / moveSpeed, movevec);

  // distAlongMovevec = ||distVec|| * cos(angle between unitMovevec and distVec)
  float distAlongMovevec = qdot(unitMovevec, distVec);

  // Check that sphere j is moving towards sphere i
  if (distAlongMovevec <= 0) {
    return 0;
  }

  float jToMovevecDistSq =
      (float)((double)(dist * dist) -
              (double)(distAlongMovevec * distAlongMovevec));

  // Break if the closest that sphere j will get to sphere i is more than
  // the sum of their radii
  float sumRadiiSquared = sumRadii * sumRadii;
  if (jToMovevecDistSq >= sumRadiiSquared) {
    return 0;
  }

  // We now have jToMovevecDistSq and sumRadii, two sides of a right triangle.
  // Use these to find the third side, sqrt(T)
  float extraDist = (float)((double)sumRadiiSquared - (double)jToMovevecDistSq);

  if (extraDist < 0) {
    return 0;
  }

  // Draw out the spheres to check why this is the distance sphere j moves
  // before hitting sphere i;)
  float distance = (float)((double)distAlongMovevec - (double)sqrt(extraDist));

  // Break if the distance sphere j has to move to touch sphere i is too big
  if (distance < 0 || moveDist < distance) {
    return 0;
  }

  *timeToCollision = distance / moveSpeed;
  return 1;
}

void do_timestep_libstudent(simulator_state_t* state, float timeStep) {
  float timeLeft = timeStep;

  // If collisions are getting too frequent, we cut time step early
  // This allows for smoother rendering without losing accuracy
  while (timeLeft > 0.000001) {
    float minCollisionTime = timeLeft;
    int indexCollider1 = -1;
    int indexCollider2 = -1;

    // parallelization of collision detection
    // remove n from for loop
    int n = state->s_spec.n_spheres;
    // allocate a flat array for collision times
    float *collision_times = malloc(n * n * sizeof(float));

    cilk_for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        collision_times[i * n + j] = timeLeft;
        check_for_collision_libstudent(state->spheres, i, j, &collision_times[i * n + j]);
      }
    }

    // find minimum collision time
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (collision_times[i * n + j] < minCollisionTime) {
          minCollisionTime = collision_times[i * n + j];
          indexCollider1 = i;
          indexCollider2 = j;
        }
      }
    }

    free(collision_times);

    do_ministep_libstudent(state->spheres, state->s_spec.n_spheres, state->s_spec.g, minCollisionTime, indexCollider1, indexCollider2);

    timeLeft = timeLeft - minCollisionTime;
  }
}

sphere_t* simulate_libstudent(simulator_state_t* state) {
  int n_spheres = state->s_spec.n_spheres;
  float timeStep = n_spheres > 1 ? (1 / log(n_spheres)) : 1;
  do_timestep_libstudent(state, timeStep);
  return state->spheres;
}

// Wrapper functions for the linker
simulator_state_t* init_simulator(const simulator_spec_t *spec) {
  return init_simulator_libstudent(spec);
}

void destroy_simulator(simulator_state_t* state) {
  destroy_simulator_libstudent(state);
}

sphere_t* simulate(simulator_state_t* state) {
  return simulate_libstudent(state);
}