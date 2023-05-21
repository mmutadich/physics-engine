#ifndef __POLYGON_H__
#define __POLYGON_H__

#include "color.h"
#include "list.h"
#include "vector.h"

typedef struct polygon polygon_t;

polygon_t *polygon_init(size_t n_points, rgb_color_t color);

/**
 * Computes the area of a polygon.
 * See https://en.wikipedia.org/wiki/Shoelace_formula#Statement.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the area of the polygon
 */
double polygon_area(list_t *polygon);

/**
 * Computes the center of mass of a polygon.
 * See https://en.wikipedia.org/wiki/Centroid#Of_a_polygon.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the centroid of the polygon
 */
vector_t polygon_centroid(list_t *polygon);

vector_t vec_rotate_point(vector_t v, double angle, vector_t point);

/**
 * Translates all vertices in a polygon by a given vector.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param translation the vector to add to each vertex's position
 */
void polygon_translate(list_t *polygon, vector_t translation);

/**
 * Rotates vertices in a polygon by a given angle about a given point.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param angle the angle to rotate the polygon, in radians.
 * A positive angle means counterclockwise.
 * @param point the point to rotate around
 */
void polygon_rotate(list_t *polygon, double angle, vector_t point);

list_t *make_initial_star(size_t n_points);

vector_t *get_velocity(polygon_t *polygon);

rgb_color_t polygon_get_color(polygon_t *polygon);

list_t *get_points(polygon_t *polygon);

void set_velocity(polygon_t *polygon, vector_t *velocity);

size_t get_n_points(polygon_t *polygon);

#endif // #ifndef __POLYGON_H__
