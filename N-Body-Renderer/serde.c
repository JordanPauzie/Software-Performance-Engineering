#include "./serde.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

const uint64_t FRAMES_HEADER = 0xdd62fd16ef82c162;

/**
 * @brief A macro that reads into dst and returns early if too few or too many
 * items are read.
 *
 * dst must be the value to read into. n is the number of items to read.
 *
 * nread must be a size_t.
 * file must be a FILE*.
 *
 */
#define READ(dst, n)                                                           \
  do {                                                                         \
    nread = fread(&(dst), sizeof((dst)), (n), file);                           \
    if (nread < (size_t)(n)) {                                                 \
      return D_READ_TOO_FEW;                                                   \
    } else if (nread > (size_t)(n)) {                                          \
      return D_READ_TOO_MANY;                                                  \
    }                                                                          \
  } while (0)

/**
 * @brief A macro that writes to file from src and returns early if too few or
 * too many items are read.
 *
 * dst must be the value to read into. n is the number of items to read.
 *
 * nwritten must be a size_t.
 * file must be a FILE*.
 *
 */
#define WRITE(src, n)                                                          \
  do {                                                                         \
    nwritten = fwrite((src), sizeof((*src)), (n), file);                       \
    if (nwritten < (size_t)(n)) {                                              \
      return S_WROTE_TOO_FEW;                                                  \
    } else if (nwritten > (size_t)(n)) {                                       \
      return S_WROTE_TOO_MANY;                                                 \
    }                                                                          \
  } while (0)

/*
The format of serialized frames is as follows:
    is_diff
    height
    width
    n_frames
    buf
*/

void destroy_frames(frames_t *frames) {
  free(frames->buf);
  frames->buf = NULL;
}

deser_error_e deser_frames(frames_t *frames, FILE *file) {
  if (file == NULL || frames == NULL) {
    return D_NULL_PTR;
  }

  size_t nread;
  uint64_t header;
  READ(header, 1);
  if (header != FRAMES_HEADER) {
    return D_HEADER_MISMATCH;
  }

  READ(frames->is_diff, 1);
  READ(frames->height, 1);
  READ(frames->width, 1);
  READ(frames->n_frames, 1);

  const size_t floats_per_px = frames->is_diff ? 1 : 3;
  const size_t n_floats =
      floats_per_px * frames->height * frames->width * frames->n_frames;
  float *buf = malloc(n_floats * sizeof(float));
  assert(buf != NULL || n_floats == 0);

  nread = fread(buf, sizeof(float), n_floats, file);
  if (nread < n_floats) {
    free(buf);
    return D_READ_TOO_FEW;
  } else if (nread > n_floats) {
    free(buf);
    return D_READ_TOO_MANY;
  }
  frames->buf = buf;

  char smallbuf[1];
  if (fread(smallbuf, sizeof(char), 1, file) != 0)
   return D_TOO_MUCH_DATA;

  return D_NO_ERROR;
}

ser_error_e ser_frames(FILE *file, const frames_t *frames) {
  if (file == NULL || frames == NULL) {
    return S_NULL_PTR;
  }

  size_t nwritten;

  WRITE(&FRAMES_HEADER, 1);
  WRITE(&frames->is_diff, 1);
  WRITE(&frames->height, 1);
  WRITE(&frames->width, 1);
  WRITE(&frames->n_frames, 1);

  const size_t floats_per_px = frames->is_diff ? 1 : 3;
  const size_t floats_per_frame =
      frames->width * frames->height * floats_per_px;
  WRITE(frames->buf, floats_per_frame * frames->n_frames);

  return S_NO_ERROR;
}

/*
The format of a simulation spec is as follows:
    g n_spheres
    each sphere (r, mass, position vector, velocity vector, material diffusion/reflection)
*/

deser_error_e deser_simulator_spec(simulator_spec_t *s_spec, FILE *file) {
  // Parses simulation spec text file
  fscanf(file, "%lf%d", &s_spec->g, &s_spec->n_spheres);
  // We use calloc here to make sure that all fields of spheres are initialized to a valid value.
  // Specifically, this makes sure that our accelerations are zeroed: we'll set all other spheres
  // later.
  sphere_t *const spheres = calloc(s_spec->n_spheres, sizeof(sphere_t));

  for (int i = 0; i < s_spec->n_spheres; i++) {
    fscanf(file, "%f%f%f%f%f%f%f%f%f%f%f%f", (float *)&spheres[i].r,
           (float *)&spheres[i].mass, (float *)&spheres[i].pos.x,
           (float *)&spheres[i].pos.y, (float *)&spheres[i].pos.z,
           (float *)&spheres[i].vel.x, (float *)&spheres[i].vel.y,
           (float *)&spheres[i].vel.z, &spheres[i].mat.diffuse.red,
           &spheres[i].mat.diffuse.green, &spheres[i].mat.diffuse.blue,
           &spheres[i].mat.reflection);
  }

  s_spec->spheres = spheres;
  return 0;
}

/*
The format of a render spec is as follows:
    resolution viewport_size
    eye position vector
    projection plane u-vector
    projection plane v-vector
    n_lights
    each light (position vector, color)
*/

deser_error_e deser_renderer_spec(renderer_spec_t *r_spec, FILE *file) {
  // Parses rendering spec text file
  fscanf(file, "%d%f", &r_spec->resolution, &r_spec->viewport_size);
  fscanf(file, "%f%f%f", (float *)&r_spec->eye.x, (float *)&r_spec->eye.y, 
                           (float *)&r_spec->eye.z);
  fscanf(file, "%f%f%f", (float *)&r_spec->proj_plane_u.x, (float *)&r_spec->proj_plane_u.y,
                           (float *)&r_spec->proj_plane_u.z);
  fscanf(file, "%f%f%f", (float *)&r_spec->proj_plane_v.x, (float *)&r_spec->proj_plane_v.y,
                           (float *)&r_spec->proj_plane_v.z);
  fscanf(file, "%d", &r_spec->n_lights);

  light_t *const lights = malloc(r_spec->n_lights * sizeof(light_t));

  for (int i = 0; i < r_spec->n_lights; i++) {
    fscanf(file, "%f%f%f%f%f%f", (float *)&lights[i].pos.x, (float *)&lights[i].pos.y, 
                                   (float *)&lights[i].pos.z,
                                   (float *)&lights[i].intensity.red,  
                                   (float *)&lights[i].intensity.green,  
                                   (float *)&lights[i].intensity.blue); 
  }

  r_spec->lights = lights;
  return 0;
}