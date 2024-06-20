#include "body.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
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
const double RED = 100;
const double GREEN = 75.3;
const double BLUE = 79.6;
const size_t N_POINTS = 4;
const size_t NUM_STARS = 40;
const double GRAVITY_CONSTANT = 1000;
const size_t CAPACITY_INIT = 10;
const vector_t INITIAL_VELOCITY = {.x = 1, .y = 1};

typedef struct state {
  scene_t *scene;
} state_t;

list_t *get_star_points(double side_length, vector_t initial_point) {
  list_t *result = list_init(CAPACITY_INIT, free);
  vector_t center_point = {.x = initial_point.x, .y = initial_point.y};
  vector_t *first_point = malloc(sizeof(vector_t));
  assert(first_point);
  first_point->x = initial_point.x;
  first_point->y = initial_point.y + side_length;
  list_add(result, first_point);
  for (int i = 0; i < N_POINTS * 2 - 1; i++) {
    vector_t *rotated_vec = malloc(sizeof(vector_t));
    assert(rotated_vec);
    vector_t v = vec_rotate_point(*(vector_t *)list_get(result, i),
                                  (M_PI / N_POINTS), center_point);
    rotated_vec[0] = v;
    if (i % 2 == 0) {
      rotated_vec[0].x =
          initial_point.x + (rotated_vec[0].x - initial_point.x) / 2;
      rotated_vec[0].y =
          initial_point.y + (rotated_vec[0].y - initial_point.y) / 2;
    }
    if (i % 2 == 1) {
      rotated_vec[0].x =
          initial_point.x + (rotated_vec[0].x - initial_point.x) * 2;
      rotated_vec[0].y =
          initial_point.y + (rotated_vec[0].y - initial_point.y) * 2;
    }
    list_add(result, rotated_vec);
  }
  return result;
}

body_t *make_gravity_star() {
  double side_length =
      (rand() % 50) + 15; // make a random number between 20 and 80
  size_t x_init = (rand() % (int)SDL_MAX.x / 2) +
                  SDL_MAX.x / 4; // makes a random number between 500 and 1500
  size_t y_init = (rand() % (int)SDL_MAX.y / 2) +
                  SDL_MAX.y / 4; // makes a random number between 500 and 1500
  assert(x_init < SDL_MAX.x && x_init > 0);
  assert(y_init < SDL_MAX.y && y_init > 0);
  vector_t initial_point = {.x = x_init, .y = y_init};
  list_t *points = get_star_points(side_length, initial_point);
  float red = fabs(((int)x_init * (int)RED) % 100 / 100.0);
  float blue = fabs(((int)x_init * (int)BLUE) % 100 / 100.0);
  float green = fabs(((int)x_init * (int)GREEN) % 100 / 100.0);
  rgb_color_t color = {red, green, blue};
  double mass = side_length * 10;
  body_t *result = body_init(points, mass, color);
  body_set_velocity(result, INITIAL_VELOCITY);
  return result;
}

scene_t *make_initial_scene() {
  scene_t *scene = scene_init();
  assert(scene);
  for (size_t i = 0; i < NUM_STARS; i++) {
    body_t *star = make_gravity_star();
    scene_add_body(scene, star);
  }
  for (size_t j = 0; j < scene_bodies(scene); j++) {
    body_t *body1 = scene_get_body(scene, j);
    for (size_t k = j + 1; k < scene_bodies(scene); k++) {
      body_t *body2 = scene_get_body(scene, k);
      create_newtonian_gravity(scene, GRAVITY_CONSTANT, body1, body2);
    }
  }
  return scene;
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  state_t *state = malloc(sizeof(state));
  assert(state);
  state->scene = make_initial_scene();
  srand(time(NULL));
  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  scene_t *scene = state->scene;
  scene_tick(scene, dt);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    sdl_draw_polygon(shape, body_get_color(body));
  }
  sdl_show();
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}