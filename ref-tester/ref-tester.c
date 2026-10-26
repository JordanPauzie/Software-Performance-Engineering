#include "./ref-tester.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

ref_stats_t compare_images(const float *ref, const float *test,
                           const int height, const int width) {
  float sum = 0;
  float sumSq = 0;
  float min = 1;
  float max = 0;

  float *diff = malloc(sizeof(float) * (size_t)height * (size_t)width);
  bool correct = true;

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      float red_diff =
          fabs(ref[(x + y * width) * 3] - test[(x + y * width) * 3]);
      float green_diff =
          fabs(ref[(x + y * width) * 3 + 1] - test[(x + y * width) * 3 + 1]);
      float blue_diff =
          fabs(ref[(x + y * width) * 3 + 2] - test[(x + y * width) * 3 + 2]);

      float px_diff = (red_diff + green_diff + blue_diff) / 3;
      if (px_diff != 0) {
        correct = false;
      }

      sum += px_diff;
      sumSq += px_diff * px_diff;

      if (px_diff > max) {
        max = px_diff;
      }
      if (px_diff < min) {
        min = px_diff;
      }
      diff[(x + y * width)] = px_diff;
    }
  }

  const image_diff_t image_diff = {
      .buf = diff, .height = height, .width = width};
  const ref_stats_t stats = {
      .correct = correct,
      .avg = sum / (height * width),
      .std_dev =
          sqrt((sumSq - (sum * sum) / (width * height)) / (width * height)),
      .min = min,
      .max = max,
      .diff = image_diff,
  };
  return stats;
}