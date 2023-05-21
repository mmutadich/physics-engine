#include "collision.h"
#include "list.h"
#include "math.h"
#include "polygon.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const vector_t ZERO_VEC = {.x = 0, .y = 0};

typedef struct line {
  vector_t point1;
  vector_t point2;
} line_t;

void line_freer(line_t *line) { free(line); }

line_t *line_init(vector_t point1, vector_t point2) {
  line_t *result = malloc(sizeof(line_t));
  result->point1 = point1;
  result->point2 = point2;
  return result;
}

line_t *get_perpendicular_line(line_t *line) {
  vector_t point1 = line->point1;
  vector_t point2 = line->point2;
  vector_t midpoint = {.x = (point1.x + point2.x) / 2,
                       .y = (point1.y + point2.y) / 2};
  vector_t new_point1 = vec_rotate_point(point1, M_PI / 2, midpoint);
  vector_t new_point2 = vec_rotate_point(point2, M_PI / 2, midpoint);
  line_t *result = line_init(new_point1, new_point2);
  return result;
}

list_t *shape_perpendicular_lines(list_t *shape) {
  list_t *lines = list_init(list_size(shape), line_freer);
  for (size_t i = 1; i <= list_size(shape); i++) {
    vector_t point1 = *(vector_t *)list_get(shape, i - 1);
    vector_t point2 = *(vector_t *)list_get(shape, i % list_size(shape));
    line_t *line = line_init(point1, point2);
    assert(line);
    line_t *perp_line = get_perpendicular_line(line);
    assert(perp_line);
    free(line);
    list_add(lines, perp_line);
  }
  return lines;
}

vector_t line_get_unit_vector(line_t *line) {
  vector_t point1 = line->point1;
  vector_t point2 = line->point2;
  vector_t temp = vec_subtract(point2, point1);
  vector_t unit_vector = vec_normalize(temp);
  return unit_vector;
}

double project_vertex_onto_line(line_t *line, vector_t vertex) {
  vector_t unit_vector = line_get_unit_vector(line);
  double length = vec_dot(vertex, unit_vector);
  return length;
}

vector_t project_polygon_onto_line(list_t *shape, line_t *line) {
  vector_t vertex1 = *(vector_t *)list_get(shape, 0);
  double value1 = project_vertex_onto_line(line, vertex1);
  double max = value1;
  double min = value1;
  for (size_t i = 0; i < list_size(shape); i++) {
    vector_t vertex = *(vector_t *)list_get(shape, i);
    double value = project_vertex_onto_line(line, vertex);
    if (value < min)
      min = value;
    if (value > max)
      max = value;
  }
  vector_t result = {.x = min, .y = max};
  return result;
}

double overlaps(vector_t v1, vector_t v2) {
  double min1 = v1.x;
  double min2 = v2.x;
  double max1 = v1.y;
  double max2 = v2.y;
  if (min1 < min2 && max1 > min2 && max2 > max1) {
    return (max1 - min2);
  }
  if (min1 < min2 && max1 > min2 && max2 < max1) {
    return (max2 - min2);
  }
  if (min2 < min1 && max2 > min1 && max1 > max2) {
    return (max2 - min1);
  }
  if (min2 < min1 && max2 > min1 && max1 < max2) {
    return (max1 - min1);
  } else {
    return 0;
  }
}

bool collision_get_collided(collision_info_t collision) {
  return collision.collided;
}

vector_t collision_get_axis(collision_info_t collision) {
  return collision.axis;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
  list_t *perp_lines1 = shape_perpendicular_lines(shape1);
  list_t *perp_lines2 = shape_perpendicular_lines(shape2);
  double min_overlap = INFINITY;
  vector_t min_axis = ZERO_VEC;
  for (size_t i = 0; i < list_size(perp_lines1); i++) {
    line_t *current_line = list_get(perp_lines1, i);
    vector_t mm1 = project_polygon_onto_line(shape1, current_line);
    vector_t mm2 = project_polygon_onto_line(shape2, current_line);
    if (!overlaps(mm1, mm2)) {
      list_free(perp_lines1);
      list_free(perp_lines2);
      collision_info_t result = {.collided = false, .axis = ZERO_VEC};
      return result;
    }
    if (overlaps(mm1, mm2) < min_overlap) {
      min_overlap = overlaps(mm1, mm2);
      min_axis = line_get_unit_vector(current_line);
    }
  }
  for (size_t i = 0; i < list_size(perp_lines2); i++) {
    line_t *current_line = list_get(perp_lines2, i);
    vector_t mm1 = project_polygon_onto_line(shape1, current_line);
    vector_t mm2 = project_polygon_onto_line(shape2, current_line);
    if (!overlaps(mm1, mm2)) {
      list_free(perp_lines1);
      list_free(perp_lines2);
      collision_info_t result = {.collided = false, .axis = ZERO_VEC};
      return result;
    }
    if (overlaps(mm1, mm2) < min_overlap) {
      min_overlap = overlaps(mm1, mm2);
      min_axis = line_get_unit_vector(current_line);
    }
  }
  if (perp_lines2 != NULL)
    list_free(perp_lines2);
  if (perp_lines1 != NULL)
    list_free(perp_lines1);
  collision_info_t result = {.collided = true, .axis = min_axis};
  return result;
}