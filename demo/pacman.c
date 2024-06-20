#include "body.h"
#include "list.h"
#include "scene.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const vector_t SDL_MIN = {.x = 0, .y = 0};
const vector_t SDL_MAX = {.x = 2000, .y = 1000};
rgb_color_t COLOR = {1, 0.75, 0.75};
const double MASS = 0.001;
const size_t NUM_POINTS_OF_PELLET = 8;
const size_t NUM_PACMAN_POINTS = 16;
const double PELLET_RADIUS = 10;
const double PACMAN_RADIUS = 60;
const size_t INITIAL_NUM_PELLETS = 20;
const size_t TIME_BETWEEN_PELLETS = 3;
const double VELOCITY_MAGNITUDE = 30;
const vector_t INITIAL_POS = {.x = 500, .y = 500};
const size_t PACMAN_INDEX = 0;
const vector_t ZERO_VECTOR = {.x = 0, .y = 0};

typedef enum {
  CENTROID_UP = 0,
  CENTROID_LEFT = 4,
  CENTROID_DOWN = 8,
  CENTROID_RIGHT = 12
} centroid_direction_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

void pacman_rotate(body_t *pacman, double new_angle) {
  if (new_angle != body_get_angle(pacman)) {
    body_set_rotation(pacman, body_get_angle(pacman) - new_angle);
    body_set_angle(pacman, new_angle);
  }
}

list_t *make_pellet_points(vector_t *centroid) {
  list_t *points = list_init(NUM_POINTS_OF_PELLET, free);
  for (size_t i = 0; i < NUM_POINTS_OF_PELLET; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    point->x = centroid->x;
    point->y = centroid->y + PELLET_RADIUS;
    vector_t v = vec_subtract(*point, *centroid);
    v = vec_rotate(v, (2 * M_PI / NUM_POINTS_OF_PELLET) * i);
    v = vec_add(v, *centroid);
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = v.x;
    vec->y = v.y;
    point = vec;
    list_add(points, point);
  }
  return points;
}

list_t *points_around_centroid(vector_t *centroid, size_t point_of_centroid) {
  list_t *points = list_init(NUM_PACMAN_POINTS, free);
  for (size_t i = 0; i < NUM_PACMAN_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == point_of_centroid) {
      point = centroid;
    } else {
      point->x = centroid->x;
      point->y = centroid->y + PACMAN_RADIUS;
      vector_t v = vec_subtract(*point, *centroid);
      v = vec_rotate(v, (M_PI / NUM_POINTS_OF_PELLET) * i);
      v = vec_add(v, *centroid);
      point->x = v.x;
      point->y = v.y;
    }
    list_add(points, point);
  }
  return points;
}

body_t *spawn_pellet() {
  size_t rand_x = rand() % (size_t)SDL_MAX.x; // makes number between
                                              // 0-SDL_MAX.x
  size_t rand_y = rand() % (size_t)SDL_MAX.y; // makes number between
                                              // 0-SDL_MAX.y
  vector_t *centroid = malloc(sizeof(vector_t));
  assert(centroid);
  centroid->x = rand_x;
  centroid->y = rand_y;
  list_t *points = make_pellet_points(centroid);
  body_t *result = body_init(points, MASS, COLOR);
  body_set_velocity(result, ZERO_VECTOR);
  return result;
}

double vec_distance(vector_t vec1, vector_t vec2) {
  return sqrt((vec1.x - vec2.x) * (vec1.x - vec2.x) +
              (vec1.y - vec2.y) * (vec1.y - vec2.y));
}

void eat_pellet(state_t *state) {
  for (size_t i = 1; i < scene_bodies(state->scene); i++) {
    body_t *pacman = scene_get_body(state->scene, PACMAN_INDEX);
    body_t *pellet = scene_get_body(state->scene, i);
    if (vec_distance(body_get_centroid(pellet), body_get_centroid(pacman)) <=
        PACMAN_RADIUS) {
      scene_remove_body(state->scene, i);
    }
  }
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  vector_t *initial_pos = malloc(sizeof(vector_t));
  assert(initial_pos);
  initial_pos->x = INITIAL_POS.x;
  initial_pos->y = INITIAL_POS.y;
  body_t *pacman = body_init(points_around_centroid(initial_pos, CENTROID_LEFT),
                             MASS, COLOR);
  vector_t velocity = {.x = 0, .y = 0};
  body_set_velocity(pacman, velocity);
  scene_add_body(result, pacman);
  for (size_t i = 0; i < INITIAL_NUM_PELLETS; i++) {
    scene_add_body(result, spawn_pellet());
  }
  return result;
}

void wrap_around(body_t *pacman) {
  vector_t centroid = body_get_centroid(pacman);
  size_t direction = 1; // initilize to non-0
  if (centroid.x < SDL_MIN.x + PACMAN_RADIUS) {
    centroid.x = SDL_MAX.x - PACMAN_RADIUS;
    direction = CENTROID_LEFT;
  }

  if (centroid.x > SDL_MAX.x - PACMAN_RADIUS) {
    centroid.x = SDL_MIN.x + PACMAN_RADIUS;
    direction = CENTROID_RIGHT;
  }

  if (centroid.y < SDL_MIN.y + PACMAN_RADIUS) {
    centroid.y = SDL_MAX.y - PACMAN_RADIUS;
    direction = CENTROID_DOWN;
  }

  if (centroid.y > SDL_MAX.y - PACMAN_RADIUS) {
    centroid.y = SDL_MIN.y + PACMAN_RADIUS;
    direction = CENTROID_UP;
  }

  if (direction == CENTROID_UP || direction == CENTROID_DOWN ||
      direction == CENTROID_LEFT || direction == CENTROID_RIGHT) {
    vector_t *centroid_pointer = malloc(sizeof(vector_t));
    assert(centroid_pointer);
    centroid_pointer->x = centroid.x;
    centroid_pointer->y = centroid.y;
    body_set_centroid(pacman, *centroid_pointer);
    body_set_points(pacman,
                    points_around_centroid(centroid_pointer, direction));
  }
}

void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *pacman = scene_get_body(state->scene, PACMAN_INDEX);
  if (type == KEY_PRESSED) {
    eat_pellet(state);
    wrap_around(scene_get_body(state->scene, PACMAN_INDEX));
    vector_t velocity =
        body_get_velocity(scene_get_body(state->scene, PACMAN_INDEX));
    vector_t force = body_get_force(scene_get_body(state->scene, PACMAN_INDEX));
    if (key == LEFT_ARROW) {
      velocity.x = -1 * VELOCITY_MAGNITUDE;
      velocity.y = 0;
      pacman_rotate(pacman, M_PI);
      force.x = -1 * held_time * held_time;
    }
    if (key == UP_ARROW) {
      velocity.x = 0;
      velocity.y = VELOCITY_MAGNITUDE;
      pacman_rotate(pacman, 3 * M_PI / 2.0);
      force.y = held_time * held_time;
    }
    if (key == RIGHT_ARROW) {
      velocity.x = VELOCITY_MAGNITUDE;
      velocity.y = 0;
      pacman_rotate(pacman, 0.0);
      force.x = held_time * held_time;
    }
    if (key == DOWN_ARROW) {
      velocity.x = 0;
      velocity.y = -1 * VELOCITY_MAGNITUDE;
      pacman_rotate(pacman, M_PI / 2);
      force.y = -1 * held_time * held_time;
    }
    body_set_force(pacman, force);
    body_set_velocity(pacman, velocity);
  } else {
    body_set_velocity(pacman, ZERO_VECTOR);
    body_set_force(pacman, ZERO_VECTOR);
  }
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  sdl_on_key((key_handler_t)keyer);
  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = make_initial_scene();
  state->time_elapsed = 0;
  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  if (state->time_elapsed > TIME_BETWEEN_PELLETS) {
    scene_add_body(state->scene, spawn_pellet());
    state->time_elapsed = 0;
  }
  for (size_t i = 0; i < scene_bodies(state->scene); i++) {
    body_t *body = scene_get_body(state->scene, i);
    list_t *shape = body_get_shape(body);
    sdl_draw_polygon(shape, COLOR);
    list_free(shape);
  }
  scene_tick(state->scene, dt);
  sdl_show();
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}