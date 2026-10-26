/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/

#ifndef SERDE_H
#define SERDE_H

#include "./common/types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// The first 8 bytes of serialized frames..
extern const uint64_t FRAMES_HEADER;

typedef enum {
  D_NO_ERROR = 0,
  D_NULL_PTR = 1,
  D_READ_TOO_FEW = 2,
  D_READ_TOO_MANY = 3,
  D_HEADER_MISMATCH = 4,
  D_TOO_MUCH_DATA = 5,
} deser_error_e;

typedef enum {
  S_NO_ERROR = 0,
  S_NULL_PTR = 1,
  S_WROTE_TOO_FEW = 2,
  S_WROTE_TOO_MANY = 3,
  S_HEADER_MISMATCH = 4,
} ser_error_e;

/**
 * @brief A representation of the frames in an image.
 *
 */
typedef struct {
  bool is_diff;
  size_t height;
  size_t width;
  size_t n_frames;
  float *buf;
} frames_t;

void destroy_frames(frames_t *frames);

/**
 * @brief Deserialize the frames_t contained within `file` into `frames`.
 * Returns a non-zero value in the case of an error.
 *
 * @param frames item to deserialize into
 * @param file file containing exactly one frames_t
 * @return non-zero in case of error
 */
deser_error_e deser_frames(frames_t *frames, FILE *file);

/**
 * @brief Serialize `frames` and write it to `file`. Returns a non-zero value in
 * the case of an error.
 *
 * @param file file to write to
 * @param frames item to serialize
 * @return non-zero in case of error
 */
ser_error_e ser_frames(FILE *file, const frames_t *frames);

/**
 * @brief Deserialize the simulator_spec_t contained within `file` into `spec`.
 * Returns a non-zero value in the case of an error.
 *
 * This function may allocate memory, which it is the caller's responsibility to
 * free via destroy_simulator_spec.
 *
 * @param[out] spec item to deserialize into
 * @param[in] file file containing exactly one simulator_spec_t
 * @return non-zero in case of error
 */
deser_error_e deser_simulator_spec(simulator_spec_t *spec, FILE *file);

/**
 * @brief Deserialize the renderer_spec_t contained within `file` into `spec`.
 * Returns a non-zero value in the case of an error.
 *
 * This function may allocate memory, which it is the caller's responsibility to
 * free via destroy_render_spec.
 *
 * @param[out] spec item to deserialize into
 * @param[in] file file containing exactly one renderer_spec_t
 * @return non-zero in case of error
 */
deser_error_e deser_renderer_spec(renderer_spec_t *spec, FILE *file);

#endif // SERDE_H
