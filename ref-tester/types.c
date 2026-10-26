#include "./types.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void destroy_e2e_spec(e2e_spec_t *spec) {
  destroy_renderer_spec(&spec->r_spec);
  destroy_simulator_spec(&spec->s_spec);
}

void destroy_ref_spec(ref_spec_t *spec) {
  destroy_e2e_spec(&spec->ref);
  destroy_e2e_spec(&spec->test);
}

void destroy_image_diff(image_diff_t *diff) {
  free(diff->buf);
  diff->buf = NULL;
}

void destroy_ref_stats(ref_stats_t *stats) {
  free(stats->diff.buf);
  stats->diff.buf = NULL;
}

void destroy_ref_out(ref_out_t *out) {
  for (ref_stats_t *stats = out->stats; stats < out->stats + out->n_elts;
       stats++) {
    destroy_ref_stats(stats);
  }
  free(out->stats);
  out->stats = NULL;
}

/**
 * @brief Aggregate the provided image difference.
 *
 * @param out output to aggregate
 * @param agg aggregation strategy
 * @return the image difference, aggregated via `agg`
 */
static image_diff_t aggregate_image_diff(const ref_out_t *const out,
                                         img_agg_e agg) {
  assert(out->n_elts > 0);

  image_diff_t diff;
  diff.height = out->stats->diff.height;
  diff.width = out->stats->diff.width;
  diff.buf = malloc(3 * (size_t)diff.width * (size_t)diff.height * sizeof(float));
  assert(diff.buf != NULL && "allocation failure");

  for (const ref_stats_t *stat = out->stats; stat < out->stats + out->n_elts;
       stat++) {
    for (size_t y = 0; y < diff.height; y++) {
      for (size_t x = 0; x < diff.width; x++) {
        const size_t px_i =
            x +
            (y *
             diff.width); // the index of the current pixel in row-major order

        float *px = diff.buf + (3 * px_i);
        const float r = px[0];
        const float g = px[1];
        const float b = px[2];

        switch (agg) {
        case AVG_DIFF:
          px[0] += r / out->n_elts;
          px[1] += g / out->n_elts;
          px[2] += b / out->n_elts;
          break;
        case MAX_DIFF:
          px[0] = fmaxf(px[0], r);
          px[1] = fmaxf(px[1], g);
          px[2] = fmaxf(px[2], b);
          break;
        case MIN_DIFF:
          px[0] = fminf(px[0], r);
          px[1] = fminf(px[1], g);
          px[2] = fminf(px[2], b);
          break;

        default:
          assert(false && "shouldn't get here");
          break;
        }
      }
    }
  }

  return diff;
}

ref_stats_t aggregate(const ref_out_t *out, img_agg_e agg) {
  bool correct = true;
  float max_diff = 0;
  float min_diff = 1;
  float var_sum = 0;
  float avg_sum = 0;
  for (const ref_stats_t *stat = out->stats; stat < out->stats + out->n_elts;
       stat++) {
    correct &= stat->correct;
    
    max_diff = fmaxf(max_diff, stat->max);
    min_diff = fminf(min_diff, stat->min);

    var_sum += stat->std_dev * stat->std_dev;
    avg_sum += stat->avg;
  }

  const ref_stats_t result = {
      .correct = correct,
      .avg = avg_sum / out->n_elts,
      .std_dev = sqrt(var_sum / out->n_elts),
      .min = min_diff,
      .max = max_diff,
      .diff = aggregate_image_diff(out, agg),
  };

  return result;
}