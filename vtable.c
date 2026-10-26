#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/render.h"
#include "common/simulate.h"
#include "vtable.h"
#include "libstaff/include/libstaff.h"


static void init_staff_renderer(vtable_t *p) {
  p->init_renderer = init_renderer_libstaff;
  p->destroy_renderer = destroy_renderer_libstaff;
  p->render = render_libstaff;
}

static void init_staff_simulator(vtable_t *p) {
  p->init_simulator = init_simulator_libstaff;
  p->destroy_simulator = destroy_simulator_libstaff;
  p->simulate = simulate_libstaff;
}

#undef INIT_VTABLE_PTR

vtable_t staff_all() {
  static bool initialized = false;
  static vtable_t table_p[1];

  if (!initialized) {
    initialized = true;

    init_staff_renderer(table_p);
    init_staff_simulator(table_p);
  }
  return *table_p;
}

vtable_t student_renderer() {
  static bool initialized = false;
  static vtable_t table_p[1];

  if (!initialized) {
    initialized = true;

    table_p->init_renderer = init_renderer;
    table_p->destroy_renderer = destroy_renderer;
    table_p->render = render;

    init_staff_simulator(table_p);
  }
  return *table_p;
}

vtable_t student_simulator() {
  static bool initialized = false;
  static vtable_t table_p[1];

  if (!initialized) {
    initialized = true;

    init_staff_renderer(table_p);

    table_p->init_simulator = init_simulator;
    table_p->destroy_simulator = destroy_simulator;
    table_p->simulate = simulate;
  }
  return *table_p;
}

vtable_t student_all() {
  static vtable_t table = {
      .init_renderer = init_renderer,
      .destroy_renderer = destroy_renderer,
      .render = render,

      .init_simulator = init_simulator,
      .destroy_simulator = destroy_simulator,
      .simulate = simulate,
  };
  return table;
}

void init_impl(vtable_t *impl, const renderer_spec_t *r_spec,
               const simulator_spec_t *s_spec) {
  impl->simulate_this = impl->init_simulator(s_spec);
  impl->renderer_this = impl->init_renderer(r_spec);
}

void destroy_impl(vtable_t *impl) {
  impl->destroy_renderer(impl->renderer_this);
  impl->destroy_simulator(impl->simulate_this);
}
