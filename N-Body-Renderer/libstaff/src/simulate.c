/**
 * Author: Isabel Rosa, isrosa@mit.edu
 **/

#include <assert.h>
#include <cilk/cilk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/simulate.h"
#include "../include/misc_utils.h"
#include "../include/libstaff.h"

simulator_state_t* init_simulator_libstaff(const simulator_spec_t *spec) {
  simulator_state_t *state = (simulator_state_t*)malloc(sizeof(simulator_state_t));
  state->s_spec = *spec;
  state->spheres = malloc(2 * spec->n_spheres * sizeof(sphere_t));
  assert(state->spheres != NULL);
  memcpy(state->spheres, spec->spheres, sizeof(sphere_t) * spec->n_spheres);
  memcpy(state->spheres + state->s_spec.n_spheres, spec->spheres, sizeof(sphere_t) * state->s_spec.n_spheres);
  return state;
}

void destroy_simulator_libstaff(simulator_state_t* state) {
  free(state->spheres);
  free(state);
}

void update_accel_sphere_libstaff(sphere_t *spheres, int n_spheres, double g, int i) {
  double rx = 0;
  double ry = 0;
  double rz = 0;

  const vector_t zero_vec = {0, 0, 0};
  spheres[i + n_spheres].accel = zero_vec;
  for (int j = 0; j < n_spheres; j++) {
    if (i != j) {
      vector_t i_minus_j = qsubtract_libstaff(spheres[i].pos, spheres[j].pos);
      vector_t j_minus_i = scale_libstaff(-1, i_minus_j);
      vector_t force =
          scale_libstaff(g * spheres[j].mass / pow(qsize_libstaff(i_minus_j), 3), j_minus_i);
      rx += (double)force.x;
      ry += (double)force.y;
      rz += (double)force.z;
    }
  }
  const vector_t v = {.x = rx, .y = ry, .z = rz};
  spheres[i + n_spheres].accel = v;
}

void update_accelerations_libstaff(sphere_t *spheres, int n_spheres, double g) {
  for (int i = 0; i < n_spheres; i++) {
    update_accel_sphere_libstaff(spheres, n_spheres, g, i);
  }
}

void update_velocities_libstaff(sphere_t *spheres, int n_spheres, float t) {
  for (int i = 0; i < n_spheres; i++) {
    spheres[i + n_spheres].vel = qadd_libstaff(spheres[i].vel, scale_libstaff(t, spheres[i].accel));
  }
}

void update_positions_libstaff(sphere_t *spheres, int n_spheres, float t) {
  for (int i = 0; i < n_spheres; i++) {
    spheres[i + n_spheres].pos = qadd_libstaff(spheres[i].pos, scale_libstaff(t, spheres[i].vel));
  }
}

// runs simulation for minCollisionTime timesteps
// perform collision between spheres at indices i and j
void do_ministep_libstaff(sphere_t *spheres, int n_spheres, double g, float minCollisionTime, int i, int j) {
  update_accelerations_libstaff(spheres, n_spheres, g);
  update_velocities_libstaff(spheres, n_spheres, minCollisionTime);
  update_positions_libstaff(spheres, n_spheres, minCollisionTime);

  for (int k = 0; k < n_spheres; k++) {
    spheres[k] = spheres[k + n_spheres];
  }

  if (i == -1 || j == -1) {
    return;
  }

  vector_t distVec = qsubtract_libstaff(spheres[i].pos, spheres[j].pos);
  float scale1 = 2 * spheres[j].mass /
                 (float)((double)spheres[i].mass + (double)spheres[j].mass);
  float scale2 = 2 * spheres[i].mass /
                 (float)((double)spheres[i].mass + (double)spheres[j].mass);
  float distNorm = qdot_libstaff(distVec, distVec);
  vector_t velDiff = qsubtract_libstaff(spheres[i].vel, spheres[j].vel);
  vector_t scaledDist = scale_libstaff(qdot_libstaff(velDiff, distVec) / distNorm, distVec);
  spheres[i].vel = qsubtract_libstaff(spheres[i].vel, scale_libstaff(scale1, scaledDist));
  spheres[j].vel = qsubtract_libstaff(spheres[j].vel, scale_libstaff(-1 * scale2, scaledDist));
}

// Check if the spheres at indices i and j collide in the next
// timeToCollision timesteps
// 
// If so, modifies timeToCollision to be the time until spheres i and j collide.
int check_for_collision_libstaff(sphere_t *spheres, int i, int j, float *timeToCollision) {
  vector_t distVec = qsubtract_libstaff(spheres[i].pos, spheres[j].pos);
  float dist = qsize_libstaff(distVec);
  float sumRadii = (float)((double)spheres[i].r + (double)spheres[j].r);

  // Shift frame of reference to act like sphere i is stationary
  // Not adjusting for acceleration because our simulation does not adjust for acceleration
  vector_t movevec = qsubtract_libstaff(spheres[j].vel, spheres[i].vel);

  // Distance that sphere j moves in timeToCollision time
  float moveDist = (float)((double)qsize_libstaff(movevec) * (double)*timeToCollision);

  // Break if the length the sphere moves in timeToCollision time is less than
  // distance between the centers of these spheres minus their radii
  if ((double)moveDist < (double)dist - (double)sumRadii ||
      (movevec.x == 0 && movevec.y == 0 && movevec.z == 0)) {
    return 0;
  }

  vector_t unitMovevec = scale_libstaff(1 / qsize_libstaff(movevec), movevec);

  // distAlongMovevec = ||distVec|| * cos(angle between unitMovevec and distVec)
  float distAlongMovevec = qdot_libstaff(unitMovevec, distVec);

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

  *timeToCollision = distance / qsize_libstaff(movevec);
  return 1;
}

void do_timestep_libstaff(simulator_state_t* state, float timeStep) {
  float timeLeft = timeStep;

  // If collisions are getting too frequent, we cut time step early
  // This allows for smoother rendering without losing accuracy
  while (timeLeft > 0.000001) {
    float minCollisionTime = timeLeft;
    int indexCollider1 = -1;
    int indexCollider2 = -1;

    for (int i = 0; i < state->s_spec.n_spheres; i++) {
      for (int j = i + 1; j < state->s_spec.n_spheres; j++) {
        if (check_for_collision_libstaff(state->spheres, i, j, &minCollisionTime)) {
          indexCollider1 = i;
          indexCollider2 = j;
        }
      }
    }

    do_ministep_libstaff(state->spheres, state->s_spec.n_spheres, state->s_spec.g, minCollisionTime, indexCollider1, indexCollider2);

    timeLeft = timeLeft - minCollisionTime;
  }
}

sphere_t* simulate_libstaff(simulator_state_t* state) {
  int n_spheres = state->s_spec.n_spheres;
  float timeStep = n_spheres > 1 ? (1 / log(n_spheres)) : 1;
  do_timestep_libstaff(state, timeStep);
  return state->spheres;
}