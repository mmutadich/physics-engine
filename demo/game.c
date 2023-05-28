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
const rgb_color_t COLOR = {1, 0.75, 0.75}; // general color used for everything since we will use drawings on top

// GENERAL BODY CONSTANTS
const NUM_RECT_POINTS = 4;
const double INFINITY_MASS = INFINITY;
const double ELASTICITY = 0.5;

// CHARACTER CONSTANTS
const vector_t INITIAL_PLANT_BOY_POSITION = {.x = 100, .y = 20};
const vector_t INITIAL_DIRT_GIRL_POSITION = {.x = 300, .y = 20};
const double CHARACTER_VELOCITY = 1000;
const double CHARACTER_SIDE_LENGTH = 80;
const double CHARACTER_MASS = 10;

// WALL CONSTANTS
const double WALL_WIDTH = 10;

// LEDGE CONSTANTS
const vector_t LEDGE_1_CENTROID = {.x = 800, .y = 350};
const vector_t LEDGE_2_CENTROID = {.x = 1200, .y = 700};
const double LEDGE_LENGTH = 1600;
const double LEDGE_WIDTH = 40;

// BLOCK CONSTANTS
const vector_t BLOCK_1_CENTROID = {.x = 1905, .y = 95};
const vector_t BLOCK_2_CENTROID = {.x = 95, .y = 465};
const double BLOCK_LENGTH = 190;

// DOOR CONSTANTS
const vector_t PLANT_BOY_DOOR_CENTROID = {.x = 1600, .y = 780};
const vector_t DIRT_GIRL_DOOR_CENTROID = {.x = 1800, .y = 780};
const double DOOR_HEIGHT = 120;
const double DOOR_WIDTH = 80;

// OBSTACLE CONSTANTS
const vector_t PLANT_BOY_OBSTACLE_CENTROID = {.x = 600, .y = 30};
const vector_t DIRT_GIRL_OBSTACLE_CENTROID = {.x = 1300, .y = 710};
const double OBSTACLE_LENGTH = 120;
const double OBSTACLE_WIDTH = 20;
const rgb_color_t OBSTACLE_COLOR = {0.75, 1, 0.75}; // for visibility on top of ledge


typedef enum {
  PLANT_BOY = 1,
  DIRT_GIRL = 2,
  PLANT_BOY_OBSTACLE = 3, // obstacle for plantboy
  DIRT_GIRL_OBSTACLE = 4, // obstacle for dirtgirl
  SLUDGE = 5, // obstacle for both players
  PLANT_BOY_DOOR = 6, // finish line for plantboy
  DIRT_GIRL_DOOR = 7, // finish line for dirtgirl
  LEDGE = 8, // for characters to walk on
  BLOCK = 9, // to assist the characters in jumping
  WALL = 10, // edges of screen
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
                                          COLOR, WALL, NULL);
  body_t *top_wall = body_init_with_info(make_wall_shape(1), INFINITY_MASS,
                                         COLOR, WALL, NULL);
  body_t *right_wall = body_init_with_info(make_wall_shape(2), INFINITY_MASS,
                                           COLOR, WALL, NULL);
  body_t *bottom_wall = body_init_with_info(make_wall_shape(3), INFINITY_MASS,
                                           COLOR, LEDGE, NULL);  //bottom wall should be a ledge                                     
  scene_add_body(scene, left_wall);
  scene_add_body(scene, top_wall);
  scene_add_body(scene, right_wall);
  scene_add_body(scene, bottom_wall);
}

list_t *make_ledge_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
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
  body_t *ledge_1 = body_init_with_info(make_ledge_shape(LEDGE_1_CENTROID), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *ledge_2 = body_init_with_info(make_ledge_shape(LEDGE_2_CENTROID), INFINITY_MASS, COLOR, LEDGE, NULL);          
  scene_add_body(scene, ledge_1);
  scene_add_body(scene, ledge_2);
}

list_t *make_block_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - BLOCK_LENGTH / 2;
    else
      point->x = centroid.x + BLOCK_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + BLOCK_LENGTH / 2;
    else
      point->y = centroid.y - BLOCK_LENGTH / 2;
    list_add(points, point);
  }
  return points;
}

void add_blocks(scene_t *scene) {
  body_t *block_1 = body_init_with_info(make_block_shape(BLOCK_1_CENTROID), INFINITY_MASS, COLOR, BLOCK, NULL);
  body_t *block_2 = body_init_with_info(make_block_shape(BLOCK_2_CENTROID), INFINITY_MASS, COLOR, BLOCK, NULL);          
  scene_add_body(scene, block_1);
  scene_add_body(scene, block_2);
}

list_t *make_door_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - DOOR_WIDTH / 2;
    else
      point->x = centroid.x + DOOR_WIDTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + DOOR_HEIGHT / 2;
    else
      point->y = centroid.y - DOOR_HEIGHT / 2;
    list_add(points, point);
  }
  return points;
}

void add_doors(scene_t *scene) {
  body_t *plant_boy_door = body_init_with_info(make_door_shape(PLANT_BOY_DOOR_CENTROID), INFINITY_MASS, COLOR, PLANT_BOY_DOOR, NULL);
  body_t *dirt_girl_door = body_init_with_info(make_door_shape(DIRT_GIRL_DOOR_CENTROID), INFINITY_MASS, COLOR, DIRT_GIRL_DOOR, NULL);                                     
  scene_add_body(scene, plant_boy_door);
  scene_add_body(scene, dirt_girl_door);
}

list_t *make_obstacle_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - OBSTACLE_LENGTH / 2;
    else
      point->x = centroid.x + OBSTACLE_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + OBSTACLE_WIDTH / 2;
    else
      point->y = centroid.y - OBSTACLE_WIDTH / 2;
    list_add(points, point);
  }
  return points;
}

void add_obstacles(scene_t *scene) {
  body_t *plant_boy_obstacle = body_init_with_info(make_obstacle_shape(PLANT_BOY_OBSTACLE_CENTROID), INFINITY_MASS, OBSTACLE_COLOR, PLANT_BOY_OBSTACLE, NULL);
  body_t *dirt_girl_obstacle = body_init_with_info(make_obstacle_shape(DIRT_GIRL_OBSTACLE_CENTROID), INFINITY_MASS, OBSTACLE_COLOR, DIRT_GIRL_OBSTACLE, NULL);                                     
  scene_add_body(scene, plant_boy_obstacle);
  scene_add_body(scene, dirt_girl_obstacle);
}

list_t *make_character_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
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

// TODO: ADD FORCES HERE
void add_characters(scene_t *scene) {
  list_t *plant_boy_shape = make_character_shape(INITIAL_PLANT_BOY_POSITION);
  body_t *plant_boy = body_init_with_info(plant_boy_shape, CHARACTER_MASS, COLOR, PLANT_BOY, NULL);
  scene_add_body(scene, plant_boy);
  list_t *dirt_girl_shape = make_character_shape(INITIAL_DIRT_GIRL_POSITION);
  body_t *dirt_girl = body_init_with_info(dirt_girl_shape, CHARACTER_MASS, COLOR, DIRT_GIRL, NULL);
  scene_add_body(scene, dirt_girl);
}

size_t find_dirt_girl(scene_t *scene){
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == DIRT_GIRL){
      return i;
    }
  }
  return NULL;
}

size_t find_plant_boy(scene_t *scene){
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == PLANT_BOY){
      return i;
    }
  }
  return NULL;
}

list_t *find_ledges(scene_t *scene){
  list_t *ledges = list_init(3, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == LEDGE){
      list_add(ledges, scene_get_body(scene,i));
    }
  }
  return ledges;
}

void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *dirt_girl = scene_get_body(state->scene, find_dirt_girl(state->scene));
  body_t *plant_boy = scene_get_body(state->scene, find_plant_boy(state->scene));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  assert(body_get_info(plant_boy) == PLANT_BOY);
  vector_t centroid_girl = body_get_centroid(dirt_girl);
  vector_t centroid_boy = body_get_centroid(plant_boy);
  vector_t new_centroid_girl = {.x = centroid_girl.x, .y = centroid_girl.y};
  vector_t new_centroid_boy = {.x = centroid_boy.x, .y = centroid_boy.y};
  //TODO: Add wall boundaries!!
  if (type == KEY_PRESSED) {
    if (key == D_KEY) {
      vector_t velocity = {.x = CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(dirt_girl, velocity);
    }
    if (key == A_KEY) {
      vector_t velocity = {.x = -CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(dirt_girl, velocity);
    }
    if (key == W_KEY) {
      list_t *ledges = find_ledges(state->scene);
      printf("size of ledges: %zu\n", list_size(ledges));
      for (size_t i = 0; i < list_size(ledges); i++) {
        body_t *ledge = list_get(ledges, i);
        assert(dirt_girl);
        assert(ledge);
        jump_up(state->scene, dirt_girl, ledge, ELASTICITY);
      }
    }
    if (key == RIGHT_ARROW) {
      vector_t velocity = {.x = CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(plant_boy, velocity);
    }
    if (key == LEFT_ARROW) {
      vector_t velocity = {.x = -CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(plant_boy, velocity);
    }
    if (key == UP_ARROW) {
      list_t *ledges = find_ledges(state->scene);
      printf("size of ledges: %zu\n", list_size(ledges));
      for (size_t i = 0; i < list_size(ledges); i++) {
        body_t *ledge = list_get(ledges, i);
        assert(plant_boy);
        assert(ledge);
        jump_up(state->scene, plant_boy, ledge, ELASTICITY);
      }
    }
  } else {
    body_set_velocity(dirt_girl, ZERO_VECTOR);
    body_set_velocity(plant_boy, ZERO_VECTOR);
  }
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  add_walls(result);
  add_ledges(result);
  add_blocks(result);
  add_doors(result);
  add_obstacles(result);
  // TODO: ADD FERTILIZER (WRITE FUNCTION)
  //add_fertilizer(result);
  add_characters(result); // add plant boy and dirt girl last since all the forces will be added with those two
  return result;
}

// Game ends when both players overlap w finish line or either player lands in an obstacle
bool is_game_over(scene_t *scene) {
  bool plant_boy_finished = false; // how do we determine this?
  bool dirt_girl_finished = false;
  bool plant_boy_dead = false;
  bool dirt_girl_dead = false;
  if (plant_boy_dead || dirt_girl_dead)
    return true;
  return (plant_boy_finished && dirt_girl_finished);
}


void reset_game(scene_t *scene) {
  // remove 2 characters - do we not need to change any of the other bodies?
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    if (body_get_info(curr_body) == PLANT_BOY) {
      body_set_centroid(curr_body, INITIAL_PLANT_BOY_POSITION);
      body_reset(curr_body);
    }
    if (body_get_info(curr_body) == DIRT_GIRL) {
      body_set_centroid(curr_body, INITIAL_DIRT_GIRL_POSITION);
      body_reset(curr_body);
    }
  }
  // redraw all characters
  add_characters(scene);
  // re-add force creators to the 2 players
  // TODO: INCLUDE THE FORCE ADDING CODE HERE
  /*
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene, i)) == BRICK) {
      create_one_sided_destructive_collision(scene, ball,
                                             scene_get_body(scene, i));
      create_physics_collision(scene, ELASTICITY, ball,
                               scene_get_body(scene, i));
    }
  }*/
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
  scene_t *scene = state->scene;
  if (is_game_over(scene)) {
    reset_game(scene);
  }
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    if (body_get_info(body) != WALL)
      sdl_draw_polygon(shape, body_get_color(body));
  }
  scene_tick(scene, dt);
  sdl_show();
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}