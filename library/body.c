#include "body.h"
#include "color.h"
#include "list.h"
#include "polygon.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const vector_t VELOCITY_0 = {.x = 0, .y = 0};
const vector_t POSITION_0 = {.x = 0, .y = 0};
const vector_t FORCE_0 = {.x = 0, .y = 0};
const vector_t IMPULSE_0 = {.x = 0, .y = 0};

typedef struct body {
  list_t *points;
  double mass;
  vector_t velocity;
  rgb_color_t color;
  vector_t force;
  vector_t impulse;
  double angle;
  bool is_removed;
  void *info;
  free_func_t info_freer;
} body_t;

body_t *body_init(list_t *shape, double mass, rgb_color_t color) {
  body_t *result = malloc(sizeof(body_t));
  assert(result);
  result->points = shape;
  result->mass = mass;
  result->color = color;
  result->velocity = VELOCITY_0;
  result->force = FORCE_0;
  result->impulse = IMPULSE_0;
  result->angle = M_PI;
  result->is_removed = false;
  result->info = NULL;
  result->info_freer = NULL;
  return result;
}

body_t *body_init_with_info(list_t *shape, double mass, rgb_color_t color,
                            void *info, free_func_t info_freer) {
  body_t *result = malloc(sizeof(body_t));
  assert(result);
  result->points = shape;
  result->mass = mass;
  result->color = color;
  result->velocity = VELOCITY_0;
  result->force = FORCE_0;
  result->impulse = IMPULSE_0;
  result->angle = M_PI;
  result->is_removed = false;
  result->info = info;
  result->info_freer = info_freer;
  return result;
}

void body_free(body_t *body) {
  if (body->info != NULL && body->info_freer != NULL)
    body->info_freer(body->info);
  list_free(body->points);
  free(body);
}

list_t *body_get_shape(body_t *body) {
  list_t *result = list_init(list_size(body->points), free);
  for (size_t i = 0; i < list_size(body->points); i++) {
    vector_t *v = list_get(body->points, i);
    vector_t *to_add = malloc(sizeof(vector_t));
    to_add->x = v->x;
    to_add->y = v->y;
    list_add(result, to_add);
  }
  assert(result);
  return result;
}

vector_t body_get_centroid(body_t *body) {
  return polygon_centroid(body->points);
}

vector_t body_get_velocity(body_t *body) { return body->velocity; }

rgb_color_t body_get_color(body_t *body) { return body->color; }

void body_set_color(body_t *body, rgb_color_t color) { body->color = color; }

void body_set_centroid(body_t *body, vector_t x) {
  vector_t current_centroid = body_get_centroid(body);
  polygon_translate(body->points, vec_subtract(x, current_centroid));
}

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_rotation(body_t *body, double angle) {
  list_t *temp_list = list_init(list_size(body->points), free);
  for (size_t i = 0; i < list_size(body->points); i++) {
    vector_t vec = vec_subtract(*(vector_t *)list_get(body->points, i),
                                body_get_centroid(body));
    vec = vec_rotate(vec, angle);
    vector_t final_vec = vec_add(vec, body_get_centroid(body));
    vector_t *final_vec_pointer = malloc(sizeof(vector_t));
    assert(final_vec_pointer);
    final_vec_pointer->x = final_vec.x;
    final_vec_pointer->y = final_vec.y;
    list_add(temp_list, final_vec_pointer);
  }
  list_free(body->points);
  body->points = temp_list;
}

double calculate_net_force(body_t *body) {
  return sqrt((body->force.x) * (body->force.x) +
              (body->force.y) * (body->force.y));
}

void body_tick(body_t *body, double dt) {
  vector_t old_velocity = body->velocity;
  vector_t acc = {.x = body->force.x / body->mass,
                  .y = body->force.y / body->mass};
  body->velocity = vec_add(
      body->velocity, vec_multiply(1 / body_get_mass(body), body->impulse));
  vector_t new_velocity = {.x = body->velocity.x + acc.x * dt,
                           .y = body->velocity.y + acc.y * dt};
  body->velocity = new_velocity;
  vector_t difference =
      vec_multiply(dt / 2, vec_add(old_velocity, new_velocity));
  vector_t centroid = vec_add(body_get_centroid(body), difference);
  body_set_centroid(body, centroid);
  body->force = VEC_ZERO;
  body->impulse = VEC_ZERO;
}

size_t body_get_n_points(body_t *body) { return list_size(body->points) / 2; }

void body_set_points(body_t *body, list_t *points) {
  list_free(body->points);
  body->points = points;
}

double body_get_angle(body_t *body) { return body->angle; }

void body_set_angle(body_t *body, double angle) { body->angle = angle; }

double body_get_mass(body_t *body) { return body->mass; }

void body_set_force(body_t *body, vector_t force) { body->force = force; }

void body_add_force(body_t *body, vector_t force) {
  body->force = vec_add(body->force, force);
}

vector_t body_get_force(body_t *body) { return body->force; }

void body_add_impulse(body_t *body, vector_t impulse) {
  body->impulse = vec_add(body->impulse, impulse);
}

void *body_get_info(body_t *body) { return body->info; }

void body_remove(body_t *body) { body->is_removed = true; }

bool body_is_removed(body_t *body) { return body->is_removed; }

bool body_is_player(body_t *body) { return body->info == 0; }

void body_reset(body_t *body) {
  body->velocity = VELOCITY_0;
  body->impulse = IMPULSE_0;
  body->force = FORCE_0;
}

void body_decrease_velocity(body_t *body, double factor) {
  vector_t new_velocity = vec_multiply(factor, body->velocity); 
  body->velocity = new_velocity;
}