/**
 * Author: Jay Hilton, jhilton@mit.edu
 **/
#ifndef DBG_TYPES_H
#define DBG_TYPES_H
#include "./common/types.h"

/**
 * @brief Provide a string representing the provided vector.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param v the provided vector
 * @return the string representing the provided vector
 */
char *dbg_vector(const vector_t *v);

/**
 * @brief Provide a string representing the provided color.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param c the provided color
 * @return the string representing the provided color
 */
char *dbg_color(const color_t *c);

/**
 * @brief Provide a string representing the provided material.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param m the provided material
 * @return the string representing the provided material
 */
char *dbg_material(const material_t *m);

/**
 * @brief Provide a string representing the provided sphere.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param m the provided sphere
 * @return the string representing the provided sphere
 */
char *dbg_sphere(const sphere_t *s);

/**
 * @brief Provide a string representing the provided ray.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param m the provided ray
 * @return the string representing the provided ray
 */
char *dbg_ray(const ray_t *r);

/**
 * @brief Provide a string representing the provided light.
 *
 * It is the caller's responsibility to free the returned string, which is
 * heap-allocated.
 *
 * @param m the provided light
 * @return the string representing the provided light
 */
char *dbg_light(const light_t *l);

char *dbg_simulator_spec(const simulator_spec_t *spec);

char *dbg_renderer_spec(const renderer_spec_t *spec);

#endif // DBG_TYPES_H