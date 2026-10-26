/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/
#include "./dbg_types.h"
#include "./asprintf.h"
#include "./common/types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  OOM_ERROR = 137,
  TAB_WIDTH = 4,
};

typedef enum {
  NO_INDENT = 0,
  DO_INDENT = 1,
} indent_first_e;

char *dbg_vector(const vector_t *v) {
  char *p;
  int nwritten =
      asprintf(&p, "Vector { .x = %e, .y = %e, .z = %e }", v->x, v->y, v->z);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_color(const color_t *c) {
  char *p;
  int nwritten = asprintf(&p, "Color { .red = %f, .green = %f, .blue = %f }",
                          c->red, c->green, c->blue);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

static char *dbg_material_indent(const material_t *m, int indent,
                                 indent_first_e indent_first) {
  char *p;
  char *diffuse = dbg_color(&m->diffuse);
  int nwritten = asprintf(
      &p, "%*sMaterial {\n%*s\t.diffuse = %s\n%*s\t.reflection = %f\n%*s}",
      indent_first * indent, "", indent, "", diffuse, indent, "", m->reflection,
      indent, "");
  free(diffuse);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_material(const material_t *m) {
  return dbg_material_indent(m, 0, NO_INDENT);
}

static char *dbg_sphere_indent(const sphere_t *s, int indent,
                               indent_first_e indent_first) {
  char *p;
  char *pos = dbg_vector(&s->pos);
  char *vel = dbg_vector(&s->vel);
  char *accel = dbg_vector(&s->accel);
  char *mat = dbg_material_indent(&s->mat, indent + TAB_WIDTH, NO_INDENT);

  int nwritten =
      asprintf(&p,
               "%*sSphere {\n%*s\t.pos = %s\n%*s\t.vel = %s\n%*s\t.accel = "
               "%s\n%*s\t.r = %e\n%*s\t.mass = %e\n%*s\t.mat = %s\n%*s}",
               indent_first * indent, "", indent, "", pos, indent, "", vel,
               indent, "", accel, indent, "", s->r, indent, "", s->mass, indent,
               "", mat, indent, "");
  free(pos);
  free(vel);
  free(accel);
  free(mat);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_sphere(const sphere_t *s) {
  return dbg_sphere_indent(s, 0, NO_INDENT);
}

static char *dbg_ray_indent(const ray_t *r, int indent,
                            indent_first_e indent_first) {
  char *p;
  char *origin = dbg_vector(&r->origin);
  char *dir = dbg_vector(&r->dir);
  int nwritten =
      asprintf(&p, "%*sRay {\n%*s\t.origin = %s,\n%*s\t.dir = %s\n%*s}",
               indent_first * indent, "", indent, "", origin, indent, "", dir,
               indent, "");
  free(origin);
  free(dir);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_ray(const ray_t *r) { return dbg_ray_indent(r, 0, NO_INDENT); }

static char *dbg_light_indent(const light_t *l, int indent,
                              indent_first_e indent_first) {
  char *p;
  char *pos = dbg_vector(&l->pos);
  char *intensity = dbg_color(&l->intensity);
  int nwritten =
      asprintf(&p, "%*sLight {\n%*s\t.pos = %s,\n%*s\t.intensity = %s\n%*s}",
               indent_first * indent, "", indent, "", pos, indent, "",
               intensity, indent, "");
  free(pos);
  free(intensity);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_light(const light_t *l) { return dbg_light_indent(l, 0, NO_INDENT); }

// Functions which accept const void * rather than specific pointer types to
// avoid duplicating code for debug-displaying arrays.

static char *dbg_sphere_void(const void *sphere, int indent,
                             indent_first_e indent_first) {
  return dbg_sphere_indent((const sphere_t *)sphere, indent, indent_first);
}
static char *dbg_light_void(const void *light, int indent,
                            indent_first_e indent_first) {
  return dbg_light_indent((const light_t *)light, indent, indent_first);
}

/**
 * @brief Provide the debug representation of an array.
 *
 * Given that arr is a T*, dbg_f must accept T*.
 *
 * @param arr pointer to first element of array to provide debug rep of
 * @param n_elts number of elements in arr
 * @param elt_sz size of each element
 * @param dbg_f function that provides debug representation of a single element
 * in the array
 * @param indent the width to indent by
 * @return the string representing the provided array
 */
static char *dbg_arr(const void *arr, size_t n_elts, size_t elt_sz,
                     char *(dbg_f)(const void *, int, indent_first_e),
                     int indent) {
  if (n_elts == 0) {
    // We can't just return "[]" here because that's actually a const char *.
    char *p;
    int nwritten = asprintf(&p, "[]");
    if (nwritten < 0) {
      exit(OOM_ERROR);
    }
    return p;
  }
  // Our approach here is to first get the string rep of each element, then
  // allocate enough space for all of them, plus whatever extra formatting we
  // need. Then we can use sprintf into the pre-allocated string to avoid
  // quadratic strcat / asprintf.

  const char *as_chars = (const char *)arr;
  char **strs = malloc(n_elts * sizeof(char *));
  int n_chars = 0;
  for (size_t i = 0; i < n_elts; i++) {
    const void *elt = (const void *)(as_chars + (elt_sz * i));
    strs[i] = dbg_f(elt, indent + TAB_WIDTH, DO_INDENT);
    n_chars += strlen(strs[i]);
  }

  const size_t n_commas = n_elts; // We include a trailing comma for simplicity.
  const size_t n_lines = n_elts + 2;
  const size_t n_newlines = n_lines - 1;
  const size_t nbytes = 2 + n_chars + indent + n_commas + n_newlines +
                        1; // 2 for brackets and 1 for the null terminator.
  char *const s = malloc(nbytes);
  if (s == NULL) {
    for (size_t i = 0; i < n_elts; i++) {
      free(strs[i]);
    }
    free(strs);
    exit(OOM_ERROR);
  }

  // We advance and snprintf into p so that we only add to the end of the
  // string.
  char *p = s;
  int length = 0;
  int nwritten = snprintf(p + length, nbytes - length, "[");
  assert(nwritten >= 0 && (size_t)nwritten < nbytes - length);
  length += nwritten;

  for (size_t i = 0; i < n_elts; i++) {
    nwritten = snprintf(p + length, nbytes - length, "\n%s,", strs[i]);
    assert(nwritten >= 0 && (size_t)nwritten < nbytes - length);
    length += nwritten;
  }

  nwritten = snprintf(p + length, nbytes - length, "\n%*s]", indent, "");
  assert(nwritten >= 0 && (size_t)nwritten < nbytes - length);

  length += nwritten;
  // We add 1 because snprintf's return value doesn't include the null
  // terminator.
  assert(nbytes == (size_t)length + 1);

  for (size_t i = 0; i < n_elts; i++) {
    free(strs[i]);
  }
  free(strs);

  return s;
}

char *dbg_simulator_spec(const simulator_spec_t *spec) {
  char *p;
  char *spheres = dbg_arr(spec->spheres, spec->n_spheres, sizeof(sphere_t),
                          dbg_sphere_void, TAB_WIDTH);
  int nwritten = asprintf(&p,
                          "SimulateState {\n\t.spheres = %s\n\t.n_spheres = "
                          "%d\n\t.g = %f\n}",
                          spheres, spec->n_spheres, spec->g);
  free(spheres);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}

char *dbg_renderer_spec(const renderer_spec_t *spec) {
  char *p;
  char *eye = dbg_vector(&spec->eye);
  char *proj_plane_u = dbg_vector(&spec->proj_plane_u);
  char *proj_plane_v = dbg_vector(&spec->proj_plane_v);
  char *lights = dbg_arr(spec->lights, spec->n_lights, sizeof(light_t),
                         dbg_light_void, TAB_WIDTH);
  int nwritten =
      asprintf(&p,
               "RenderState {\n\t.resolution = %d\n\t.eye = %s\n\t.proj_plane_u "
               "= %s\n\t.proj_plane_v = %s\n\t.viewport_size = %f\n\t.lights = %s\n\t"
               ".n_lights = %d\n}",
               spec->resolution, eye, proj_plane_u, proj_plane_v, spec->viewport_size,
               lights, spec->n_lights);
  free(eye);
  free(proj_plane_u);
  free(proj_plane_v);
  free(lights);
  if (nwritten < 0) {
    exit(OOM_ERROR);
  }
  return p;
}