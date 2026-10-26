#ifndef VTABLE_H
#define VTABLE_H

#include "common/types.h"

/**
 * @brief A structure containing all functions associated with an implementation
 * of the renderer and simulator.
 *
 */
typedef struct vtable_t {
  struct renderer_state *(*init_renderer)(const renderer_spec_t *);
  void (*destroy_renderer)(struct renderer_state *);
  const float *(*render)(struct renderer_state *, const sphere_t *, int);
  struct renderer_state *renderer_this;

  struct simulator_state *(*init_simulator)(const simulator_spec_t *);
  void (*destroy_simulator)(struct simulator_state *);
  sphere_t* (*simulate)(struct simulator_state *);
  struct simulator_state *simulate_this;
} vtable_t;

/**
 * @brief Provide a vtable for the staff implementation.
 *
 * @return a vtable w/ staff renderer and simulator
 */
vtable_t staff_all();

/**
 * @brief Provide a vtable for an implementation with the student renderer and
 * staff simulator.
 *
 * @return a vtable w/ student renderer and staff simulator
 */
vtable_t student_renderer();

/**
 * @brief Provide a vtable for an implementation with the staff simulator and
 * student renderer.
 *
 * @return a vtable w/ staff renderer and student simulator
 */
vtable_t student_simulator();

/**
 * @brief Provide a vtable for the student implementation.
 *
 * @return a vtable w/ staff renderer and simulator
 */
vtable_t student_all();

/**
 * @brief Initialize an implementation with the provided renderer spec and
 * simulator spec.
 *
 * @param impl implementation to initialize
 * @param r_spec renderer spec to initialize with
 * @param s_spec simulator spec to initialize with
 */
void init_impl(vtable_t *impl, const renderer_spec_t *r_spec,
               const simulator_spec_t *s_spec);

/**
 * @brief Destroy the provided implementation.
 *
 * @param impl implementation to destroy
 */
void destroy_impl(vtable_t *impl);

#endif // VTABLE_H