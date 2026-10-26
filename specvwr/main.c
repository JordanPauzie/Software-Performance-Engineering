#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../dbg_types.h"
#include "../serde.h"

#define ERR_PREFIX "specvwr: "

enum spec_type { UNSET, RENDER, SIMULATE };

struct opts {
  const char *spec_fn;
  enum spec_type type;
};

static void usage(void) { fprintf(stderr, "./specvwr [-r|-s] spec\n"); }

static int argparse(int argc, char *const argv[], struct opts *const o) {
  o->type = UNSET;

  int ch;

  while ((ch = getopt(argc, argv, "rs")) != -1) {
    switch (ch) {
    case 'r':
      if (o->type == SIMULATE)
        goto error;
      o->type = RENDER;
      break;
    case 's':
      if (o->type == RENDER)
        goto error;
      o->type = SIMULATE;
      break;
    default:
      goto error;
    }
  }

  if (o->type == UNSET) {
    goto error;
  }

  argc -= optind;
  argv += optind;

  if (argc != 1) {
    goto error;
  }
  o->spec_fn = argv[0];
  return 0;

error:
  usage();
  return -1;
}

static int view_sim_spec(FILE *f) {
  simulator_spec_t spec;
  if (deser_simulator_spec(&spec, f) != D_NO_ERROR) {
    return 1;
  }

  char *const s = dbg_simulator_spec(&spec);
  printf("%s\n", s);

  destroy_simulator_spec(&spec);
  free(s);
  return 0;
}

static int view_render_spec(FILE *f) {
  renderer_spec_t spec;
  if (deser_renderer_spec(&spec, f) != D_NO_ERROR) {
    return 1;
  }

  char *const s = dbg_renderer_spec(&spec);
  printf("%s\n", s);

  free(s);
  destroy_renderer_spec(&spec);
  return 0;
}

int main(int argc, char *const argv[]) {
  struct opts o;

  if (argparse(argc, argv, &o) != 0) {
    return 1;
  }

  int (*view_fn)(FILE *);
  switch (o.type) {
  case RENDER:
    view_fn = view_render_spec;
    break;
  case SIMULATE:
    view_fn = view_sim_spec;
    break;
  case UNSET:
    assert(false && "Shouldn't get here!");
    return 1;
  }

  FILE *const file = fopen(o.spec_fn, "rb");
  if (file == NULL) {
    fprintf(stderr, ERR_PREFIX "file %s does not exist\n", o.spec_fn);
    return 1;
  }
  int error = view_fn(file);
  fclose(file);

  if (error != 0) {
    fprintf(stderr, ERR_PREFIX "error parsing %s as %s spec\n", o.spec_fn,
            o.type == RENDER ? "renderer" : "simulator");
  }

  return error;
}