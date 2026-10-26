#include "./serde.h"

#include <assert.h>
#include <stdlib.h>

const uint64_t REF_OUT_HEADER = 0x782d928c37a220de;

/*
The format of a serialized ref_out_t is as follows:
    header
    n_elts

    stats {
        avg
        std_dev
        min
        max
        stats.diff {
            height
            width
            buf
        }
    }
*/

deser_error_e deser_ref_out(ref_out_t *out, FILE *file) {
  if (file == NULL || out == NULL) {
    return D_NULL_PTR;
  }

  size_t nread;
// See the larger serde module for documentation of this macro.
#define READ(dst, n)                                                           \
  do {                                                                         \
    nread = fread(&(dst), sizeof((dst)), (n), file);                           \
    if (nread < (size_t)(n)) {                                                 \
      return D_READ_TOO_FEW;                                                   \
    } else if (nread > (size_t)(n)) {                                          \
      return D_READ_TOO_MANY;                                                  \
    }                                                                          \
  } while (0)

  uint64_t header;
  READ(header, 1);
  if (header != REF_OUT_HEADER) {
    return D_HEADER_MISMATCH;
  }

  READ(out->n_elts, 1);

  ref_stats_t stats;
  READ(stats.avg, 1);
  READ(stats.std_dev, 1);
  READ(stats.min, 1);
  READ(stats.max, 1);

  image_diff_t diff;
  READ(diff.height, 1);
  READ(diff.width, 1);
  const size_t exp_px = (size_t)diff.height * (size_t)diff.width;
  const size_t exp_floats = 3 * exp_px;

  float *buf = malloc(sizeof(float) * exp_floats);
  assert(buf != NULL || exp_px == 0);

  nread = fread(buf, sizeof(float), exp_floats, file);
  if (nread < exp_floats) {
    free(buf);
    return D_READ_TOO_FEW;
  } else if (nread > exp_floats) {
    free(buf);
    return D_READ_TOO_MANY;
  }
  diff.buf = buf;

  stats.diff = diff;

  *out->stats = stats;
#undef READ

  return D_NO_ERROR;
}

ser_error_e ser_ref_out(FILE *file, const ref_out_t *out) {
  if (file == NULL || out == NULL) {
    return S_NULL_PTR;
  }

  size_t nwritten;
// A macro that reads into dst and returns early if too few or too many items
// are read. dst must be a pointer to the value which will be written.
#define WRITE(src, n)                                                          \
  do {                                                                         \
    nwritten = fwrite((src), sizeof((*src)), (n), file);                       \
    if (nwritten < (size_t)(n)) {                                              \
      return S_WROTE_TOO_FEW;                                                  \
    } else if (nwritten > (size_t)(n)) {                                       \
      return S_WROTE_TOO_MANY;                                                 \
    }                                                                          \
  } while (0)

  WRITE(&REF_OUT_HEADER, 1);
  WRITE(&out->stats->avg, 1);
  WRITE(&out->stats->std_dev, 1);
  WRITE(&out->stats->min, 1);
  WRITE(&out->stats->max, 1);
  WRITE(&out->stats->diff.height, 1);
  WRITE(&out->stats->diff.width, 1);

  size_t n_floats = 3 * out->stats->diff.height * out->stats->diff.width;
  WRITE(out->stats->diff.buf, n_floats);
#undef WRITE

  return S_NO_ERROR;
}
