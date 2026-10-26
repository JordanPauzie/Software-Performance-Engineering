#include <getopt.h>   // optarg, optind
#include <inttypes.h> // PRIu64
#include <stdio.h>
#include <stdlib.h>

#include "../vtable.h"
#include "./benchmark.h"

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_DEFAULT "\033[0m"

#define PASS_STR COLOR_GREEN "PASS" COLOR_DEFAULT
#define FAIL_STR COLOR_RED "FAIL" COLOR_DEFAULT

#define ERR_PREFIX "find-tier: "

struct opts {
  tier_t min_tier;
  tier_t max_tier;
  tier_t blowthroughs;
};

static void usage() {
  fprintf(stderr, "./find-tier [-m min_tier] [-M max_tier] [-b blowthroughs]\n");
}

static int argparse(int argc, char *const argv[], struct opts *const o) {
  int ch;

  while ((ch = getopt(argc, argv, "m:M:b:")) != -1) {
    switch (ch) {
    case 'm':
      if (1 != sscanf(optarg, "%hhu", &o->min_tier)) {
        goto error;
      }
      break;
    case 'M':
      if (1 != sscanf(optarg, "%hhu", &o->max_tier)) {
        goto error;
      }
      break;
    case 'b':
      if (1 != sscanf(optarg, "%hhu", &o->blowthroughs)) {
        goto error;
      }
      break;
    default:
      goto error;
    }
  }

  argc -= optind;
  if (argc != 0) {
    goto error;
  }
  return 0;

error:
  usage();
  return -1;
}

static void apply_defaults(struct opts *o) {
  o->blowthroughs = 2;
  o->max_tier = MAX_TIER;
  o->min_tier = MIN_TIER;
}

const tdiff_t DEFAULT_CUTOFF = 1000;

static void print_tier_pass_message(tier_t tier_passed, const tier_spec_t *spec,
                                    tdiff_t time_elapsed, tier_timing_t* tier_timing) {
  // For some fun!
  // Celebrations must be under 5 chars
  const char *celebrations[] = {"yay", "woot", "boyah", "skrrt",
                                "ayy", "yeee", "eoo"};
  const uint32_t ncelebrations = sizeof(celebrations) / sizeof(celebrations[0]);
  const char *const random_celebration = celebrations[rand() % ncelebrations];

  printf(PASS_STR
         " (%s!):\tTier %hhu :\tRan %dx%d\timage with %zu bodies in %" PRIu64
         "ms (s: %" PRIu64 ", r: %" PRIu64 ", is: %" PRIu64 ", ir: %" PRIu64 ")\n",
         random_celebration, tier_passed, spec->width, spec->height,
         spec->n_spheres, time_elapsed, tier_timing->simulate_timing, tier_timing->render_timing,
         tier_timing->init_simulator_timing, tier_timing->init_renderer_timing);
}

static void print_tier_fail_message(tier_t tier_failed, const tier_spec_t *spec,
                                    tdiff_t time_elapsed, tier_timing_t* tier_timing,
                                    tier_t blowthroughs_used) {
  printf(
      FAIL_STR
      " (timeout):\tTier %hhu :\tRan %dx%d\timage with %zu bodies in %" PRIu64
      "ms (s: %" PRIu64 ", r: %" PRIu64 ", is: %" PRIu64 ", ir: %" PRIu64 ") but the cutoff is %" PRIu64 
      "ms; used blowthrough %hhu\n",
      tier_failed, spec->width, spec->height, spec->n_spheres, time_elapsed,
      tier_timing->simulate_timing, tier_timing->render_timing,
      tier_timing->init_simulator_timing, tier_timing->init_renderer_timing,
      DEFAULT_CUTOFF, blowthroughs_used);
}

static void display_result(const bench_result_t *result,
                           const bench_spec_t *spec) {
  printf(COLOR_GREEN "Reached tier %hhu of %hhu\n", result->max_tier,
         spec->max_tier);
  printf("Final tier attempted: %hhu\n", result->last_tier);
  printf("Time elapsed on final tier: %" PRIu64 " ms\n",
         result->last_tier_time);
  printf(COLOR_RED "REMINDER: This is only timing, NOT CORRECTNESS. Ensure you also run correctness tests" COLOR_DEFAULT "\n");
  printf(COLOR_GREEN "Blowthroughs used: %hhu / %hhu\n" COLOR_DEFAULT,
         result->blowthroughs_used, spec->blowthroughs);
}

static void validate_opts(const struct opts *o) {
  if (o->min_tier > o->max_tier) {
    fprintf(stderr, ERR_PREFIX "min tier must be less than max_tier\n");
    exit(1);
  } else if (o->min_tier < MIN_TIER) {
    fprintf(stderr, ERR_PREFIX "min tier must be at least %d\n", MIN_TIER);
    exit(1);
  } else if (o->max_tier > MAX_TIER) {
    fprintf(stderr, ERR_PREFIX "max tier must be at most %d\n", MAX_TIER);
    exit(1);
  } else if (o->blowthroughs < 0) {
    fprintf(stderr, ERR_PREFIX "blowthrough count must be non-negative\n");
    exit(1);
  }
}

int main(int argc, char *const argv[]) {
  struct opts o;
  apply_defaults(&o);

  if (argparse(argc, argv, &o) != 0) {
    return 1;
  }

  validate_opts(&o);

  bench_spec_t spec = {
      .blowthroughs = o.blowthroughs,
      .max_tier = o.max_tier,
      .min_tier = o.min_tier,
      .tier_cutoff = DEFAULT_CUTOFF,
  };

  // We open libstaff in the same manner we normally would during a reference
  // tester run.
  vtable_t vtable = staff_all();
  (void)vtable;

  printf(COLOR_RED "REMINDER: This is only timing, NOT CORRECTNESS. Ensure you also run correctness tests" COLOR_DEFAULT "\n");
  bench_result_t result =
      run_benchmark(&spec, print_tier_pass_message, print_tier_fail_message);
  display_result(&result, &spec);

  teardown();
}