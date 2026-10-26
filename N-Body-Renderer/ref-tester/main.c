#include <assert.h>
#include <getopt.h> // optarg, optind
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../serde.h"
#include "../vtable.h"
#include "./ref-tester.h"
#include "./types.h"

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_DEFAULT "\033[0m"

#define PASS_STR COLOR_GREEN "PASS" COLOR_DEFAULT
#define FAIL_STR COLOR_RED "FAIL" COLOR_DEFAULT

enum impl_opts {
  STAFF = 0,
  STUDENT = 1,
};

struct opts {
  const char *s_spec;
  const char *r_spec;
  const char *expected_frames;
  const char *diff_output;
  const char *test_name;
  size_t n_frames;
  enum impl_opts renderer;
  enum impl_opts simulator;
  bool concise_output;
};

static void open_spec_files(FILE **s_file, FILE **r_file, const char *sim_spec,
                            const char *renderer_spec) {
  *s_file = fopen(sim_spec, "rb");
  if (*s_file == NULL) {
    fprintf(stderr, "Failed to open simulator spec file %s\n", sim_spec);
    exit(FILEIO_ERR);
  }
  *r_file = fopen(renderer_spec, "rb");
  if (*r_file == NULL) {
    fprintf(stderr, "Failed to open renderer spec file %s\n", renderer_spec);
    exit(FILEIO_ERR);
  }
  return;
}

/**
 * @brief Deserialize the specs in the provided files into the provided
 * structures.
 *
 * @param[out] s_spec simulator spec to deserialize into
 * @param[out] r_spec renderer spec to deserialize into
 * @param[in] sim_spec file storing initial simulator spec
 * @param[in] renderer_spec file storing initial renderer spec
 * @return nonzero if error, otherwise 0
 */
static int deser_specs(simulator_spec_t *const s_spec,
                       renderer_spec_t *const r_spec, FILE *const s_file,
                       FILE *const r_file) {
  deser_error_e deser_sim_err = deser_simulator_spec(s_spec, s_file);
  if (deser_sim_err != D_NO_ERROR) {
    fprintf(stderr, "Failed to deserialize simulator spec\n");
    return 1;
  }
  deser_error_e deser_render_err = deser_renderer_spec(r_spec, r_file);
  if (deser_render_err != D_NO_ERROR) {
    fprintf(stderr, "Failed to deserialize renderer spec\n");
    return 1;
  }
  return 0;
}

/**
 * @brief Compare an implementation against the expected frames, without
 * resetting between frames.
 *
 * @param[in] start_spec e2e_spec to test
 * @param[in] e_frames expected frames
 * @param[in] n_frames number of frames to test
 * @return output from comparing impl to e_frames
 */
static ref_out_t ref_test_fast(e2e_spec_t *const start_spec,
                               const frames_t *e_frames, size_t n_frames) {
  vtable_t *impl = &start_spec->impl;
  init_impl(impl, &start_spec->r_spec, &start_spec->s_spec);

  const size_t floats_per_frame = 3 * (size_t)e_frames->height * (size_t)e_frames->width;

  ref_stats_t *const stats = malloc(n_frames * sizeof(ref_stats_t));
  for (size_t f = 0; f < n_frames; f++) {
    sphere_t* spheres = impl->simulate(impl->simulate_this);
    const float *img = impl->render(impl->renderer_this, spheres, start_spec->s_spec.n_spheres);
    const float *e_frame = e_frames->buf + (f * floats_per_frame);
    stats[f] = compare_images(e_frame, img, start_spec->r_spec.resolution,
                              start_spec->r_spec.resolution);
  }

  destroy_impl(impl);

  const ref_out_t out = {
      .n_elts = n_frames,
      .stats = stats,
  };
  return out;
}

/**
 * @brief Compare an implementation against the staff implementation,
 * initiaizing the provided implementation to start with the same spec as the
 * staff implementation.
 *
 * @param[in] start_spec e2e_spec to test
 * @param[in] n_frames number of frames to test with
 * @return statistics and input for next test, which derives sphere positions
 * from each respective implementation's output spheres
 */
static ref_out_t ref_test(e2e_spec_t *const start_spec, size_t n_frames) {
  vtable_t *test_impl = &start_spec->impl;
  init_impl(test_impl, &start_spec->r_spec, &start_spec->s_spec);
  
  vtable_t ref_impl = staff_all();
  init_impl(&ref_impl, &start_spec->r_spec, &start_spec->s_spec);

  int resolution = start_spec->r_spec.resolution;
  int n_spheres = start_spec->s_spec.n_spheres;

  ref_stats_t *const stats = malloc(n_frames * sizeof(ref_stats_t));
  for (size_t f = 0; f < n_frames; f++) {
    sphere_t* test_spheres = test_impl->simulate(test_impl->simulate_this);
    const float *test_img = test_impl->render(test_impl->renderer_this, test_spheres, n_spheres);

    sphere_t* ref_spheres = ref_impl.simulate(ref_impl.simulate_this);
    const float *ref_img = ref_impl.render(ref_impl.renderer_this, ref_spheres, n_spheres);

    stats[f] = compare_images(ref_img, test_img, resolution, resolution);
  }

  destroy_impl(test_impl);
  destroy_impl(&ref_impl);

  const ref_out_t out = {
      .n_elts = n_frames,
      .stats = stats,
  };
  return out;
}

/**
 * @brief Aggregate the provided output and print summary statistics
 *
 * @param[in] out output to aggregate and print
 */
static void print_agg_stats(const ref_out_t *const out, const struct opts *const o) {
  ref_stats_t agg_stats = aggregate(out, AVG_DIFF);

  if (o->concise_output) {
    printf("%s: %s\n", o->test_name, agg_stats.correct ? PASS_STR : FAIL_STR);
  } else {
    printf("Average difference: %1.16f\nStandard deviation: %1.16f\nMinimum difference: "
            "%1.16f\nMaximum difference: %1.16f\nTest result: %s\n",
            agg_stats.avg, agg_stats.std_dev, agg_stats.min, agg_stats.max,
            agg_stats.correct ? PASS_STR : FAIL_STR);
  }

  destroy_ref_stats(&agg_stats);
}

static void display_opts(const struct opts *const o) {
  printf("ref-test options:\n");
  printf("\ts_spec = %s\n", o->s_spec);
  printf("\tr_spec = %s\n", o->r_spec);
  printf("\trenderer = %s\n", o->renderer == STAFF ? "staff" : "student");
  printf("\tsimulator = %s\n", o->simulator == STAFF ? "staff" : "student");
  printf("\tn_frames = %zu\n", o->n_frames);
  if (o->diff_output) {
    printf("\tdiff_output = %s\n", o->diff_output);
  }
  if (o->expected_frames) {
    printf("\texpected_frames = %s\n", o->expected_frames);
  }
}

static void apply_defaults(struct opts *const o) {
  o->renderer = STUDENT;
  o->simulator = STUDENT;
  o->expected_frames = NULL;
  o->diff_output = NULL;
  o->n_frames = 12;
  o->concise_output = false;
}

static void usage(void) {
  fprintf(stderr, "./ref-tester [-n num_frames] [-r | -s] [-x "
                  "expected_frames] [-o diff_output] sim_spec renderer_spec\n");
}

/*
 * parse arguments according to the above usage statement
 * returns -1 on error, in which case the above should be
 * outputted and execution should be terminated.
 * returns 0 on success, populating o accordingly
 */
static int argparse(int argc, char *const argv[], struct opts *const o) {
  apply_defaults(o);

  int ch;

  while ((ch = getopt(argc, argv, "n:rhsix:o:c:")) != -1) {
    switch (ch) {
    case 'n':
      if (1 != sscanf(optarg, "%zu", &o->n_frames))
        goto error;
      break;
    case 'x':
      o->expected_frames = optarg;
      break;
    case 'r':
      o->renderer = STAFF;
      break;
    case 's':
      o->simulator = STAFF;
      break;
    case 'o':
      o->diff_output = optarg;
      break;
    case 'c':
      o->concise_output = true;
      o->test_name = optarg;
      break;
    case 'h':
    default:
      goto error;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc != 2) {
    goto error;
  }
  if (o->renderer == STAFF && o->simulator == STAFF) {
    goto error;
  }

  o->s_spec = argv[0];
  o->r_spec = argv[1];
  return 0;

error:
  usage();
  return -1;
}

static frames_t out_frames(const ref_out_t *out) {
  const size_t floats_per_frame =
      out->stats->diff.height * out->stats->diff.width;

  float *buf = malloc(sizeof(float) * floats_per_frame * (size_t)out->n_elts);
  for (size_t f = 0; f < out->n_elts; f++) {
    float *frame = buf + (f * floats_per_frame);
    const ref_stats_t stats = out->stats[f];
    memcpy(frame, stats.diff.buf, sizeof(float) * floats_per_frame);
  }

  const frames_t diff = {
      .buf = buf,
      .height = out->stats->diff.height,
      .width = out->stats->diff.width,
      .n_frames = out->n_elts,
      .is_diff = true,
  };

  return diff;
}

static void apply_log_scale(frames_t *frames) {
  assert(frames->is_diff &&
         "log scale should only be used for image difference");

  float log_eps = log(1e-8);
  float log_max = 1.0 - log_eps;

  const size_t floats_per_px = frames->is_diff ? 1 : 3;
  const size_t floats_per_frame =
      floats_per_px * frames->width * frames->height;

  for (float *frame = frames->buf;
       frame < frames->buf + (floats_per_frame * frames->n_frames);
       frame += floats_per_frame) {
    for (float *px = frame; px < frame + floats_per_frame; px++) {
      if (*px > 1e-8) {
        *px = (log(*px) - log_eps) / log_max;
      }
    }
  }
}

static error_e run(struct opts o) {
  vtable_t impl;
  switch (o.renderer) {
  case STUDENT:
    impl = o.simulator == STUDENT ? student_all() : student_renderer();
    break;
  case STAFF:
    impl = student_simulator();
    break;
  default:
    assert(false && "Shouldn't get here!");
    break;
  }

  FILE *s_f;
  FILE *r_f;
  open_spec_files(&s_f, &r_f, o.s_spec, o.r_spec);

  renderer_spec_t r_spec;
  simulator_spec_t s_spec;

  if (deser_specs(&s_spec, &r_spec, s_f, r_f)) {
    fprintf(stderr, "Failed to deserialize initial specs\n");
    return DESER_ERR;
  };

  fclose(r_f);
  fclose(s_f);

  e2e_spec_t start_spec = {
      .impl = impl, .r_spec = r_spec, .s_spec = s_spec};

  ref_out_t out;
  if (o.expected_frames == NULL) {
    out = ref_test(&start_spec, o.n_frames);
  } else {
    FILE *const file = fopen(o.expected_frames, "r");
    if (file == NULL) {
      fprintf(stderr, "Failed to open expected frames file %s\n",
              o.expected_frames);
      return BAD_ARGS;
    }
    frames_t e_frames;
    if (deser_frames(&e_frames, file)) {
      fprintf(stderr, "could not deserialize frames file %s\n",
              o.expected_frames);
      return DESER_ERR;
    }

    if (e_frames.n_frames < o.n_frames) {
      fprintf(stderr,
              "number of frames in expected frames file %s is %zu, but "
              "requested %zu frames to test\n",
              o.expected_frames, e_frames.n_frames, o.n_frames);
      return BAD_ARGS;
    }

    out = ref_test_fast(&start_spec, &e_frames, o.n_frames);

    destroy_frames(&e_frames);
  }

  print_agg_stats(&out, &o);

  if (o.diff_output != NULL) {
    FILE *diff_out = fopen(o.diff_output, "wb");
    frames_t diff_frames = out_frames(&out);
    apply_log_scale(&diff_frames);

    if (ser_frames(diff_out, &diff_frames)) {
      fprintf(stderr, "failed to serialize frame difference\n");
      return SER_ERR;
    }

    destroy_frames(&diff_frames);
  }

  // Cleanup!
  destroy_renderer_spec(&r_spec);
  destroy_simulator_spec(&s_spec);
  destroy_ref_out(&out);

  return NO_ERROR;
}

int main(int argc, char *const argv[]) {
  struct opts o;

  if (argparse(argc, argv, &o) != 0) {
    return 1;
  }

  if (!o.concise_output) {
    display_opts(&o);
  }

  return run(o);
}
