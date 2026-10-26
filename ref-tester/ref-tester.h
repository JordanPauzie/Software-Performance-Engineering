#ifndef REF_TEST_H
#define REF_TEST_H

#include "./types.h"

// The below functions are for frame-by-frame reference testing.

/**
 * @brief Compare the two provided images.
 *
 * @param ref reference image
 * @param test test image
 * @param height height of both images
 * @param width width of both images
 * @return statistics from comparing the images
 */
ref_stats_t compare_images(const float *ref, const float *test, int height,
                           int width);

#endif // REF_TEST_H