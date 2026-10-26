#ifndef FIND_TIER_BENCHMARK_H
#define FIND_TIER_BENCHMARK_H
#include <stddef.h>
#include <stdint.h>

#include "./fasttime.h" // tdiff_t

typedef int8_t tier_t;
typedef int dim_t;

enum {
  MIN_TIER = 0,
  MAX_TIER = 100,
};

/**
 * @brief A specificaton of the benchmark to run.
 */
typedef struct bench_spec_t {
  tier_t min_tier;     // tier to start from
  tier_t max_tier;     // tier to stop at
  tier_t blowthroughs; // number of failures to ignore
  tdiff_t tier_cutoff; // time to pass a tier
} bench_spec_t;

/**
 * @brief A description of a tier.
 */
typedef struct tier_spec_t {
  size_t n_spheres;
  dim_t height;
  dim_t width;
} tier_spec_t;

/**
 * @brief A representation of the render and simulate timings of a tier run
*/
typedef struct tier_timing_t {
  tdiff_t simulate_timing;
  tdiff_t render_timing;
  tdiff_t init_simulator_timing;
  tdiff_t init_renderer_timing;
} tier_timing_t;

/**
 * @brief A representation of the results of a benchmark.
 */
typedef struct bench_result_t {
  tier_t max_tier;

  tier_t last_tier;
  tdiff_t last_tier_time;

  tier_t blowthroughs_used;
} bench_result_t;

// Callback for passing a tier.
typedef void(pass_cb)(tier_t tier_passed, const tier_spec_t *spec,
                      tdiff_t time_elapsed, tier_timing_t* tier_timing);
// Callback for failing a tier.
typedef void(fail_cb)(tier_t tier_failed, const tier_spec_t *spec,
                      tdiff_t time_elapsed, tier_timing_t* tier_timing, tier_t blowthroughs_used);

/**
 * @brief Run the specified benchmark, running the corresponding callbacks on
 * tier success and failure.
 *
 * @param spec description of the benchmark to run
 * @param on_pass function to invoke on passing a tier, NULL to call no function
 * @param on_fail function to invoke on failing a tier, NULL to call no function
 * @return summary results of the benchmark
 */
bench_result_t run_benchmark(const bench_spec_t *spec, pass_cb *on_pass,
                             fail_cb *on_fail);

void teardown();

#endif // FIND_TIER_BENCHMARK_H
