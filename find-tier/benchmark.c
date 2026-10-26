#include "./benchmark.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../asprintf.h"
#include "../common/render.h"
#include "../common/simulate.h"
#include "../serde.h"

#define UNUSED(v) ((void)v)
#define AWS_TIERS_ABSPATH "/var/6106/project2-tiers/"

typedef struct tier_desc_t {
  renderer_spec_t r_spec;
  simulator_spec_t s_spec;
} tier_desc_t;

// initialized[0] is always false -> should not be read.
bool initialized[MAX_TIER + 1];
tier_desc_t tiers[MAX_TIER + 1];

const dim_t START_SIZE = 512;
const double GROWTH_RATE = 1.08;

static FILE *spec_file(const tier_t tier, const char *const spec_type) {
  char *filename;
  asprintf(&filename, "tiers/%d/%s", tier, spec_type);
  FILE *file = fopen(filename, "rb");
  
  free(filename);
  if (file != NULL) {
    return file;
  }
  
  // We might be on AWS: let's try to look at the absolute path.
  asprintf(&filename, AWS_TIERS_ABSPATH "%d/%s", tier, spec_type);
  file = fopen(filename, "rb");
  
  free(filename);
  return file;
}

static FILE *renderer_spec_file(const tier_t tier) {
  FILE *file = spec_file(tier, "r");
  if (file == NULL) {
    fprintf(stderr, "could not find renderer spec for tier %d\n", tier);
  }

  return file;
}

static FILE *simulator_spec_file(const tier_t tier) {
  FILE *file = spec_file(tier, "s");
  if (file == NULL) {
    fprintf(stderr, "could not find simulator spec for tier %d\n", tier);
  }

  return file;
}
  

static deser_error_e init_tier(const tier_t tier) {
  deser_error_e err;

  tier_desc_t *const desc = tiers + tier;

  FILE *file = renderer_spec_file(tier);

  err = deser_renderer_spec(&desc->r_spec, file);
  if (err)
    return err;

  fclose(file);

  file = simulator_spec_file(tier);

  err = deser_simulator_spec(&desc->s_spec, file);
  if (err)
    return err;

  fclose(file);

  initialized[tier] = true;

  return D_NO_ERROR;
}

static tier_spec_t make_tier_spec(const tier_desc_t *const desc) {
  const tier_spec_t spec = {
      .height = desc->r_spec.resolution,
      .width = desc->r_spec.resolution,
      .n_spheres = desc->s_spec.n_spheres,
  };

  return spec;
}

/**
 * @brief Initiaize tiers[start_tier : end_tier].
 * This function may initialize tiers other than those as well.
 *
 * @param start_tier first tier to initialize
 * @param end_tier last tier to initialize
 */
static void initialize_tiers(const tier_t start_tier, const tier_t end_tier) {
  assert(start_tier >= MIN_TIER);
  assert(end_tier <= MAX_TIER);

  // First, we have to make sure we're initialized up to start_tier.
  if (start_tier == MIN_TIER && !initialized[MIN_TIER]) {
    if (0 != init_tier(MIN_TIER)) {
      fprintf(stderr, "failed to make tier descriptor for tier %d\n", MIN_TIER);
      exit(1);
    }
  }

  for (tier_t tier = start_tier; tier <= end_tier; tier++) {
    if (initialized[tier])
      continue;

    if (0 != init_tier(tier)) {
      fprintf(stderr, "failed to make tier descriptor for tier %hhu\n", tier);
      exit(1);
    }
  }
}

static tdiff_t run_tier_desc(const tier_desc_t *const desc, tier_timing_t* tier_timing) {
  fasttime_t start = gettime();
  struct renderer_state *state_r = init_renderer(&desc->r_spec);
  fasttime_t after_init_renderer = gettime();
  struct simulator_state *state_s = init_simulator(&desc->s_spec);
  fasttime_t after_init_simulator = gettime();

  tier_timing->init_renderer_timing = tdiff_msec(start, after_init_renderer);
  tier_timing->init_simulator_timing = tdiff_msec(after_init_renderer, after_init_simulator);

  fasttime_t split_render_sim = gettime();
  fasttime_t split_sim_render;

  for (uint8_t _frame = 0; _frame < 3; _frame++) {
    // Try getting fine-grained parallelism measurements for simulate() and
    // render() here. May need #ifdef CILKSCALE, may not.
    sphere_t* spheres = simulate(state_s);
    split_sim_render = gettime();
    tier_timing->simulate_timing += tdiff_msec(split_render_sim, split_sim_render);
    const float_t *img = render(state_r, spheres, desc->s_spec.n_spheres);
    split_render_sim = gettime();
    tier_timing->render_timing += tdiff_msec(split_sim_render, split_render_sim);
    UNUSED(img);
  }

  fasttime_t stop = gettime();
  destroy_renderer(state_r);
  destroy_simulator(state_s);

  return tdiff_msec(start, stop);
}

static tdiff_t run_tier(const tier_t tier, tier_timing_t* tier_timing) {
  initialize_tiers(tier, tier);
  const tier_desc_t desc = tiers[tier];
  return run_tier_desc(&desc, tier_timing);
}

bench_result_t run_benchmark(const bench_spec_t *const spec,
                             pass_cb *const on_pass, fail_cb *const on_fail) {
  initialize_tiers(spec->min_tier, spec->max_tier);
  // Parse the file in the spec, which is of the old text format.

  tier_t blowthroughs = 0;
  tier_t max_tier = 0;

  tdiff_t user_ms;
  tier_timing_t user_timing;
  tier_t tier;

  // Run all the tiers in linear search.
  for (tier = spec->min_tier; tier <= spec->max_tier; tier++) {
    user_timing.render_timing = user_timing.simulate_timing = 0;
    user_ms = run_tier(tier, &user_timing);
    const tier_spec_t tier_spec = make_tier_spec(tiers + tier);

    if (user_ms > spec->tier_cutoff) {
      blowthroughs += 1;
      if (on_fail != NULL) {
        on_fail(tier, &tier_spec, user_ms, &user_timing, blowthroughs);
      }
      if (blowthroughs > spec->blowthroughs) {
        break;
      }
    } else {
      if (on_pass != NULL) {
        on_pass(tier, &tier_spec, user_ms, &user_timing);
      }
      max_tier = tier;
    }
  }

  const bench_result_t result = {.blowthroughs_used = blowthroughs,
                                 .last_tier = tier > MAX_TIER ? MAX_TIER : tier,
                                 .last_tier_time = user_ms,
                                 .max_tier = max_tier};
  return result;
}

static void destroy_tier_desc(tier_desc_t *desc) {
  destroy_renderer_spec(&desc->r_spec);
  destroy_simulator_spec(&desc->s_spec);
}

void teardown() {
  for (tier_t t = 0; t <= MAX_TIER; t++) {
    if (initialized[t]) {
      destroy_tier_desc(tiers + t);
    }
  }
}
