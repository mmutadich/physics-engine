#include "list.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// TODO: NEED TO CHANGE THE STARTING POSITION OF THE BALLS TO 100,900
const vector_t SDL_MIN = {.x = 0, .y = 0};
const vector_t SDL_MAX = {.x = 2000, .y = 1000};
const double ROTATION_ANGLE = 0.01;
const double DAMPING_CONSTANT = 0.9;
const size_t MAX_POLYGONS = 10;
const double TIME_BETWEEN_POLYGONS = 1.5;
const double RED = 30;
const double GREEN = 30;
const double BLUE = 40;
const double ACCELERATION = -100;
size_t INITIAL_N_POINTS = 2;

typedef struct state {
  list_t *polygons;
  double time_since_polygon_added;
} state_t;

void add_polygon(state_t *state, size_t n_points) {
  float red = fabs(((int)n_points * (int)RED) % 100 / 100.0);
  float blue = fabs(((int)n_points * (int)BLUE) % 100 / 100.0);
  float green = fabs(((int)n_points * (int)GREEN) % 100 / 100.0);
  rgb_color_t color = {red, green, blue};
  polygon_t *current_polygon = polygon_init(n_points, color);
  list_add(state->polygons, current_polygon);
}

void polygon_free(polygon_t *polygon) {
  list_free(get_points(polygon));
  free(get_velocity(polygon));
}

void remove_polygon(state_t *state) {
  if (list_size(state->polygons) == MAX_POLYGONS) {
    list_remove(state->polygons, 0);
  }
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  state_t *result = malloc(sizeof(state_t));
  result->polygons = list_init(MAX_POLYGONS, free);
  result->time_since_polygon_added = 0;
  add_polygon(result, INITIAL_N_POINTS);
  return result;
}

void move_polygon(list_t *polygon, vector_t velocity, double dt) {
  polygon_translate(polygon, (vector_t){velocity.x * dt, velocity.y * dt});
  polygon_rotate(polygon, ROTATION_ANGLE, polygon_centroid(polygon));
}

void update_velocity(polygon_t *polygon, double dt) {
  for (size_t i = 0; i < list_size(get_points(polygon)); i++) {
    vector_t *point = (vector_t *)list_get(get_points(polygon), i);
    double new_y_velocity = get_velocity(polygon)->y + ACCELERATION * dt;
    vector_t *new_velocity = get_velocity(polygon);
    new_velocity->y = new_y_velocity;
    set_velocity(polygon, new_velocity);
    if (point->y <= SDL_MIN.y && get_velocity(polygon)->y < 0) {
      vector_t *new_velocity = get_velocity(polygon);
      new_velocity->y = -1 * new_velocity->y * DAMPING_CONSTANT;
      set_velocity(polygon, new_velocity);
    }
  }
}

size_t get_next_n_points(state_t *state) {
  polygon_t *polygon =
      list_get(state->polygons, list_size(state->polygons) - 1);
  size_t result = get_n_points(polygon);
  return result + 1;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  state->time_since_polygon_added += dt;
  if (state->time_since_polygon_added > TIME_BETWEEN_POLYGONS) {
    size_t n_points = get_next_n_points(state);
    remove_polygon(state);
    add_polygon(state, n_points);
    state->time_since_polygon_added = 0;
  }
  for (size_t i = 0; i < list_size(state->polygons); i++) {
    polygon_t *curr_poly = list_get(state->polygons, i);
    move_polygon(get_points(curr_poly), *get_velocity(curr_poly), dt);
    update_velocity(curr_poly, dt);
    sdl_draw_polygon(get_points(curr_poly),
                     polygon_get_color(curr_poly)); // draws
  }
  sdl_show();
}

void emscripten_free(state_t *state) {
  list_t *polygons = state->polygons;
  list_free(polygons);
  free(state);
}