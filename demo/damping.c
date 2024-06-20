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
const size_t NUM_BALLS = 50;
const double BALL_RADIUS = 20;
const double SPRING_CONSTANT = 100;
const double GAMMA = 4;
const size_t NUM_BALL_POINTS = 7;
const vector_t INITIAL_VELOCITY = {.x = 0, .y = 0};

typedef struct state {
  scene_t *scene;
} state_t;

list_t *make_ball_points(vector_t centroid) {
  list_t *points = list_init(NUM_BALL_POINTS, free);
  for (size_t i = 0; i < NUM_BALL_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    point->x = centroid.x;
    point->y = centroid.y + BALL_RADIUS;
    vector_t v = vec_subtract(*point, centroid);
    v = vec_rotate(v, (2 * M_PI / NUM_BALL_POINTS) * i);
    v = vec_add(v, centroid);
    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = v.x;
    vec->y = v.y;
    point = vec;
    list_add(points, point);
  }
  return points;
}

body_t *make_ball(size_t i) {
  size_t x_init = 1;
  size_t y_init = 1;
  if (i % 2 == 1) { // anchors (odd)
    x_init = ((i - 1) * 2 * BALL_RADIUS + BALL_RADIUS) / 2;
    y_init = SDL_MAX.y / 2;

  } else { // balls (even)
    x_init = (i * 2 * BALL_RADIUS + BALL_RADIUS) / 2;
    y_init = SDL_MAX.y / 5;
  }
  vector_t initial_point = {.x = x_init, .y = y_init};
  list_t *points = make_ball_points(initial_point);

  float red = fabs(((int)x_init * (int)RED) % 100 / 100.0);
  float blue = fabs(((int)x_init * (int)BLUE) % 100 / 100.0);
  float green = fabs(((int)x_init * (int)GREEN) % 100 / 100.0);
  rgb_color_t color = {red, green, blue};

  double mass = 0;
  if (i % 2 == 1) // anchors (odd)
    mass = INFINITY;
  if (i % 2 == 0) // balls (even)
    mass = i;

  body_t *result = body_init(points, mass, color);
  // body_set_centroid(result, initial_point);
  body_set_velocity(result, INITIAL_VELOCITY);
  return result;
}

scene_t *make_initial_scene() {
  scene_t *scene = scene_init();
  assert(scene);
  for (size_t i = 0; i < NUM_BALLS * 2; i = i + 2) {
    body_t *ball = make_ball(i);       // balls (even)
    body_t *anchor = make_ball(i + 1); // anchors (odd)
    scene_add_body(scene, ball);
    scene_add_body(scene, anchor);
  }
  for (size_t i = 1; i < scene_bodies(scene); i = i + 2) {
    body_t *body = scene_get_body(scene, i - 1);
    body_t *anchor = scene_get_body(scene, i);
    create_drag(scene, GAMMA, body);
    create_spring(scene, SPRING_CONSTANT, body, anchor);
  }
  return scene;
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  state_t *state = malloc(sizeof(state));
  assert(state);
  state->scene = make_initial_scene();
  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();
  scene_t *scene = state->scene;
  scene_tick(scene, dt);
  for (size_t i = 0; i < scene_bodies(scene); i = i + 2) {
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
