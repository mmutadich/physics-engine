#include "body.h"
#include "color.h"
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
const vector_t ZERO_VECTOR = {.x = 0, .y = 0};

const vector_t INITIAL_PLANTBOY_POSITION = {.x = 100, .y = 100};
const vector_t INITIAL_DIRTGIRL_POSITION = {.x = 300, .y = 100};

const double CHARACTER_MASS = 10;
const rgb_color_t CHARACTER_COLOR = {1, 0.75, 0.75};

typedef enum {
  PLANTBOY = 1,
  DIRTGIRL = 2,
  PLANT = 3,
  DIRT = 4,
  SLUDGE = 5,
  WALL = 6, // includes edges of screen + ledges
} info_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

/*
list_t *make_body_shape(vector_t centroid) {
  list_t *shape = list_init(4, free);

  vector_t *point_1 = malloc(sizeof(vector_t));
  assert(point_1);
  point_1->x = 1;
  point_1->y = 1;
  list_add(shape, point_1);

  vector_t *point_2 = malloc(sizeof(vector_t));
  assert(point_2);
  point_2->x = 100;
  point_2->y = 1;
  list_add(shape, point_2);

  vector_t *point_3 = malloc(sizeof(vector_t));
  assert(point_3);
  point_3->x = 1;
  point_3->y = 100;
  list_add(shape, point_3);

  return shape;
}*/

void add_characters(scene_t *scene) {
  list_t *shape = make_character_shape(INITIAL_PLANTBOY_POSITION);
  body_t *plantboy = body_init_with_info(shape, CHARACTER_MASS, CHARACTER_COLOR, PLANTBOY, NULL);
  scene_add_body(scene, plantboy);

  list_t *shape = make_character_shape(INITIAL_DIRTGIRL_POSITION);
  body_t *dirtgirl = body_init_with_info(shape, CHARACTER_MASS, CHARACTER_COLOR, DIRTGIRL, NULL);
  scene_add_body(scene, dirtgirl);
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  add_walls(result);
  add_characters(result); // add plant boy and dirt girl last since all the forces will be added with those two
  add_dirtgirl(result);
  return result;
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  //sdl_on_key((key_handler_t)keyer);
  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = make_initial_scene();
  state->time_elapsed = 0;
  return state;
}

void emscripten_main(state_t *state) {
  sdl_clear();
  scene_t *scene = state->scene;
  //if (is_game_over(scene)) {
    //reset_game(scene);
  //}
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    sdl_draw_polygon(shape, body_get_color(body));
  }
  scene_tick(scene, dt);
  sdl_show();
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}