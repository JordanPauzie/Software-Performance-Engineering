#ifndef REF_TEST_TYPES_H
#define REF_TEST_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#include "../common/types.h"
#include "../vtable.h"

typedef enum error_e {
  NO_ERROR = 0,
  BAD_ARGS = 1,
  FILEIO_ERR = 2,
  DESER_ERR = 3,
  SER_ERR = 4,
  OOM_ERROR = 137,
} error_e;

/**
 * @brief An input for end-to-end testing.
 */
typedef struct e2e_spec_t {
  simulator_spec_t s_spec;
  renderer_spec_t r_spec;
  vtable_t impl;
} e2e_spec_t;

/**
 * @brief De-allocate any memory associated with `spec`. Does not free `spec`
 * itself.
 *
 * @param out the e2e spec to destroy
 */
void destroy_e2e_spec(e2e_spec_t *spec);

/**
 * @brief The inputs to a reference test for one frame.
 */
typedef struct ref_spec_t {
  e2e_spec_t ref;
  e2e_spec_t test;
  bool reset_sim;
} ref_spec_t;

/**
 * @brief De-allocate any memory associated with `spec`. Does not free `spec`
 * itself.
 *
 * @param out the ref spec to destroy
 */
void destroy_ref_spec(ref_spec_t *spec);

/**
 * @brief An image representing the pixel-by-pixel difference between two
 * images.
 */
typedef struct image_diff_t {
  float *buf; // has sizeof(float) * height * width bytes allocated
  size_t height;
  size_t width;
} image_diff_t;

/**
 * @brief De-allocate any memory associated with `diff`. Does not free `diff`
 * itself.
 *
 * @param diff the difference to destroy
 */
void destroy_image_diff(image_diff_t *diff);

/**
 * @brief The statistiscs outputted from a reference test.
 */
typedef struct ref_stats_t {
  bool correct;
  float avg;
  float std_dev;
  float min;
  float max;

  image_diff_t diff;
} ref_stats_t;

/**
 * @brief The statistics outputted from a reference test over multiple frames.
 */
typedef struct ref_out_t {
  size_t n_elts;
  ref_stats_t *stats;
} ref_out_t;

/**
 * @brief De-allocate any memory associated with `stats`. Does not free `stats`
 * itself.
 *
 * @param out the ref tester stats to destroy
 */
void destroy_ref_stats(ref_stats_t *stats);

/**
 * @brief De-allocate any memory associated with `out`. Does not free `out`
 * itself.
 *
 * @param out the ref tester output to destroy
 */
void destroy_ref_out(ref_out_t *out);

typedef enum {
  AVG_DIFF,
  MAX_DIFF,
  MIN_DIFF,
} img_agg_e;

/**
 * @brief Aggregate the n frames of output in out into one frame.
 *
 * @param[in] out output to aggregate
 * @param[in] agg_img strategy to use to aggregate the image
 * @return the aggregated output as a single stat
 */
ref_stats_t aggregate(const ref_out_t *out, img_agg_e agg_img);

#endif // REF_TEST_TYPES_H