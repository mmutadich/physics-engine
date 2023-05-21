#include "polygon.h"
#include "body.h"
#include "list.h"
#include "math.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

const vector_t VELOCITY = {.x = 200, .y = 0};
const size_t SIDE_LENGTH = 100;
const double PI = 3.14159;
const vector_t INITIAL_POINT = {.x = 100, .y = 900};
const size_t INITIAL_CAPACITY = 10;

typedef struct polygon {
  list_t *points;
  vector_t *velocity;
  rgb_color_t color;
  size_t n_points;
} polygon_t;

vector_t vec_rotate_point(vector_t v, double angle, vector_t point) {
  v = vec_subtract(v, point);
  v = vec_rotate(v, angle);
  v = vec_add(v, point);
  return v;
}

list_t *make_initial_star(size_t n_points) {
  list_t *result = list_init(INITIAL_CAPACITY, free);
  vector_t center_point = {.x = INITIAL_POINT.x, .y = INITIAL_POINT.y};
  vector_t *first_point = malloc(sizeof(vector_t));
  assert(first_point);
  first_point->x = INITIAL_POINT.x;
  first_point->y = INITIAL_POINT.y + SIDE_LENGTH;
  list_add(result, first_point);

  for (int i = 0; i < n_points * 2 - 1; i++) {
    vector_t *rotated_vec = malloc(sizeof(vector_t));
    assert(rotated_vec);
    rotated_vec[0] = vec_rotate_point(*(vector_t *)list_get(result, i),
                                      (PI / n_points), center_point);
    if (i % 2 == 0) {
      rotated_vec[0].x =
          INITIAL_POINT.x + (rotated_vec[0].x - INITIAL_POINT.x) / 2;
      rotated_vec[0].y =
          INITIAL_POINT.y + (rotated_vec[0].y - INITIAL_POINT.y) / 2;
    }
    if (i % 2 == 1) {
      rotated_vec[0].x =
          INITIAL_POINT.x + (rotated_vec[0].x - INITIAL_POINT.x) * 2;
      rotated_vec[0].y =
          INITIAL_POINT.y + (rotated_vec[0].y - INITIAL_POINT.y) * 2;
    }
    list_add(result, rotated_vec);
  }
  return result;
}

polygon_t *polygon_init(size_t n_points, rgb_color_t color) {
  polygon_t *result = malloc(sizeof(polygon_t));
  assert(result);
  result->points = make_initial_star(n_points);
  result->velocity = malloc(sizeof(vector_t));
  result->velocity->x = VELOCITY.x;
  result->velocity->y = VELOCITY.y;
  result->color = color;
  result->n_points = n_points;
  return result;
}

double polygon_area(list_t *polygon) {
  double sum = 0;
  size_t size = list_size(polygon);
  for (size_t i = 1; i < size + 1; i++) {
    vector_t *first = list_get(polygon, i - 1);
    vector_t *second = list_get(polygon, i % size);
    sum += (second->x + first->x) * (second->y - first->y);
  }
  return fabs(0.5 * sum);
}

vector_t polygon_centroid(list_t *polygon) {
  double x = 0;
  double y = 0;
  size_t size = list_size(polygon);
  for (size_t i = 0; i < size; i++) {
    vector_t *first = list_get(polygon, i);
    vector_t *second = list_get(polygon, (i + 1) % size);
    x += (first->x + second->x) * (first->x * second->y - first->y * second->x);
    y += (first->y + second->y) * (first->x * second->y - first->y * second->x);
  }
  x *= 1.0 / (6.0 * polygon_area(polygon));
  y *= 1.0 / (6.0 * polygon_area(polygon));
  vector_t answer = {.x = x, .y = y};
  return answer;
}

void polygon_translate(list_t *polygon, vector_t translation) {
  ssize_t size = list_size(polygon);
  for (ssize_t i = size - 1; i >= 0; i--) {
    vector_t *vector = list_get(polygon, i);
    vector->x = vector->x + translation.x;
    vector->y = vector->y + translation.y;
  }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
  ssize_t size = list_size(polygon);
  for (ssize_t i = size - 1; i >= 0; i--) {
    vector_t *vector = list_get(polygon, i);
    vector->x = vector->x - point.x;
    vector->y = vector->y - point.y;
    *vector = vec_rotate(*vector, angle);
    *vector = vec_add(*vector, point);
  }
}

vector_t *get_velocity(polygon_t *polygon) { return polygon->velocity; }

rgb_color_t polygon_get_color(polygon_t *polygon) { return polygon->color; }

list_t *get_points(polygon_t *polygon) { return polygon->points; }

void set_velocity(polygon_t *polygon, vector_t *velocity) {
  polygon->velocity->y = velocity->y;
  polygon->velocity->x = velocity->x;
}

size_t get_n_points(polygon_t *polygon) { return polygon->n_points; }
