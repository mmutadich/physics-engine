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

// WINDOW CONSTANTS
const vector_t SDL_MIN = {.x = 0, .y = 0};
const vector_t SDL_MAX = {.x = 2000, .y = 1000};
const vector_t ZERO_VECTOR = {.x = 0, .y = 0};

// GENERAL BODY CONSTANTS
const NUM_RECT_POINTS = 4;
const double INFINITY_MASS = INFINITY; 

// CHARACTER CONSTANTS
const vector_t INITIAL_PLANT_BOY_POSITION = {.x = 100, .y = 100};
const vector_t INITIAL_DIRT_GIRL_POSITION = {.x = 300, .y = 100};
const double CHARACTER_SIDE_LENGTH = 80;
const double CHARACTER_MASS = 10;
const rgb_color_t CHARACTER_COLOR = {1, 0.75, 0.75};

// WALL CONSTANTS
const rgb_color_t WALL_COLOR = {0, 0, 0}; // doesn't matter b/c wall not displayed
const double WALL_WIDTH = 10;

// LEDGE CONSTANTS
const vector_t LEDGE_1_CENTROID = {.x = 800, 350};
const vector_t LEDGE_2_CENTROID = {.x = 1200, 700};
const rgb_color_t LEDGE_COLOR = {0.98, 0.76, 0.01};
const double LEDGE_LENGTH = 1600;
const double LEDGE_WIDTH = 40;

typedef enum {
  PLANT_BOY = 1,
  DIRT_GIRL = 2,
  PLANT = 3, // obstacle for dirtgirl
  DIRT = 4, // obstacle for plantboy
  SLUDGE = 5, // obstacle for both players
  PLANT_BOY_DOOR = 6, // finish line for plantboy
  DIRT_GIRL_DOOR = 7, // finish line for dirtgirl
  LEDGE = 8, // for characters to walk on
  WALL = 9, // edges of screen
} info_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

list_t *make_wall_shape(char wall) {
  list_t *wall_points = list_init(NUM_RECT_POINTS, free);
  // top left, bottom left, bottom right, top right
  for (size_t i = 0; i < NUM_RECT_POINTS;i++) {        
    if (wall == 0) { // left
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MIN.x;
      else
        point->x = SDL_MIN.x + WALL_WIDTH;
      if (i == 0 || i == 3)
        point->y = SDL_MAX.y;
      else
        point->y = SDL_MIN.y;
      list_add(wall_points, point);
    }
    if (wall == 1) { // top
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MIN.x;
      else
        point->x = SDL_MAX.x;
      if (i == 0 || i == 3)
        point->y = SDL_MAX.y;
      else
        point->y = SDL_MAX.y - WALL_WIDTH;
      list_add(wall_points, point);
    }
    if (wall == 2) { // right
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MAX.x - WALL_WIDTH;
      else
        point->x = SDL_MAX.x;
      if (i == 0 || i == 3)
        point->y = SDL_MAX.y;
      else
        point->y = SDL_MIN.y;
      list_add(wall_points, point);
    }
    if (wall == 3) { // bottom
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MIN.x;
      else
        point->x = SDL_MAX.x;
      if (i == 0 || i == 3)
        point->y = SDL_MIN.y + WALL_WIDTH;
      else
        point->y = SDL_MIN.y;
      list_add(wall_points, point);
    }
  }
  return wall_points;
}

void add_walls(scene_t *scene) {
  body_t *left_wall = body_init_with_info(make_wall_shape(0), INFINITY_MASS,
                                          WALL_COLOR, WALL, NULL);
  body_t *top_wall = body_init_with_info(make_wall_shape(1), INFINITY_MASS,
                                         WALL_COLOR, WALL, NULL);
  body_t *right_wall = body_init_with_info(make_wall_shape(2), INFINITY_MASS,
                                           WALL_COLOR, WALL, NULL);
  body_t *bottom_wall = body_init_with_info(make_wall_shape(3), INFINITY_MASS,
                                           WALL_COLOR, WALL, NULL);                                       
  scene_add_body(scene, left_wall);
  scene_add_body(scene, top_wall);
  scene_add_body(scene, right_wall);
  scene_add_body(scene, bottom_wall);
}

list_t *make_ledge_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  // 0: top left, counter-clockwise
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - LEDGE_LENGTH / 2;
    else
      point->x = centroid.x + LEDGE_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + LEDGE_WIDTH / 2;
    else
      point->y = centroid.y - LEDGE_WIDTH / 2;
    list_add(points, point);
  }
  return points;
}

void add_ledges(scene_t *scene) {
  body_t *ledge_1 = body_init_with_info(make_ledge_shape(LEDGE_1_CENTROID), INFINITY_MASS, LEDGE_COLOR, LEDGE, NULL);
  body_t *ledge_2 = body_init_with_info(make_ledge_shape(LEDGE_2_CENTROID), INFINITY_MASS, LEDGE_COLOR, LEDGE, NULL);                                     
  scene_add_body(scene, ledge_1);
  scene_add_body(scene, ledge_2);
}

list_t *make_character_shape(vector_t centroid) { // squares as of now
  list_t *points = list_init(NUM_RECT_POINTS, free);
  // 0: top left, counter-clockwise
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - CHARACTER_SIDE_LENGTH / 2;
    else
      point->x = centroid.x + CHARACTER_SIDE_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + CHARACTER_SIDE_LENGTH / 2;
    else
      point->y = centroid.y - CHARACTER_SIDE_LENGTH / 2;
    list_add(points, point);
  }
  return points;
}

void add_characters(scene_t *scene) {
  list_t *plant_boy_shape = make_character_shape(INITIAL_PLANT_BOY_POSITION);
  body_t *plant_boy = body_init_with_info(plant_boy_shape, CHARACTER_MASS, CHARACTER_COLOR, PLANT_BOY, NULL);
  scene_add_body(scene, plant_boy);
  list_t *dirt_girl_shape = make_character_shape(INITIAL_DIRT_GIRL_POSITION);
  body_t *dirt_girl = body_init_with_info(dirt_girl_shape, CHARACTER_MASS, CHARACTER_COLOR, DIRT_GIRL, NULL);
  scene_add_body(scene, dirt_girl);
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  add_walls(result);
  add_ledges(result);
  //add_doors(result);
  //add_obstacles(result);
  //add_fertilizer(result);
  add_characters(result); // add plant boy and dirt girl last since all the forces will be added with those two
  return result;
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  // TODO: Uncomment line below and implement keyer
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
  // TODO: Uncomment folllowing lines and implement isgameover and resetgame
  //if (is_game_over(scene)) {
    //reset_game(scene);
  //}
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    sdl_draw_polygon(shape, body_get_color(body));
    // TODO: don't draw walls
  }
  scene_tick(scene, dt);
  sdl_show();
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}