/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/

#ifndef RENDER_H
#define RENDER_H

#define EXPORT __attribute__((visibility("default")))

#include "./types.h"

EXPORT
/**
 * @brief Initialize the renderer with the provided starting spec. May be
 * called more than once.
 *
 * @param spec[in] initial spec
 */
struct renderer_state* init_renderer(const renderer_spec_t *spec);

EXPORT
/**
 * @brief Reset the renderer, de-allocating any allocated memory.
 */
void destroy_renderer(struct renderer_state *state);

EXPORT
/**
 * @brief Render the spheres provided, using the renderer's saved spec.
 *
 * The image is interpreted as a 3 x height x width array, where height and
 * width were the height and width provided in the most recent invocation of
 * init_renderer. For an image im, im[3 * i : 3 * i + 3] represents the red,
 * green, and blue color values of im at pixel i.
 *
 * @param[in] spheres spheres to render
 *
 * @return a pointer to the resulting image
 */
const float *render(struct renderer_state* state, const sphere_t *spheres, int n_spheres);


#endif // RENDER_H
