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

// FORCE CONSTANTS
const double GRAVITY = -1000;

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
const double WALL_LENGTH = 10;

// LEDGE CONSTANTS
const vector_t LEDGE_FLOOR_CENTROID = {.x = 1000, .y = 20};
const vector_t LEDGE_1_CENTROID = {.x = 800, .y = 350};
const vector_t LEDGE_2_CENTROID = {.x = 1200, .y = 700};
const double LEDGE_LENGTH = 1600;
const double LEDGE_HEIGHT = 40;

// BLOCK CONSTANTS
const vector_t BLOCK_1_CENTROID = {.x = 1905, .y = 95};
const vector_t BLOCK_2_CENTROID = {.x = 95, .y = 465};
const double BLOCK_LENGTH = 190;

// DOOR CONSTANTS
/*const vector_t PLANT_BOY_DOOR_CENTROID = {.x = 1600, .y = 780};
const vector_t DIRT_GIRL_DOOR_CENTROID = {.x = 1800, .y = 780};
const double DOOR_HEIGHT = 120;
const double DOOR_LENGTH = 80;*/
const vector_t PLANT_BOY_DOOR_RIGHT_CENTROID = {.x = 1565, .y = 780};
const vector_t PLANT_BOY_DOOR_LEFT_CENTROID = {.x = 1635, .y = 780};
const vector_t DIRT_GIRL_DOOR_RIGHT_CENTROID = {.x = 1765, .y = 780};
const vector_t DIRT_GIRL_DOOR_LEFT_CENTROID = {.x = 1835, .y = 780};
const double DOOR_HEIGHT = 120;
const double DOOR_LENGTH = 10;

// OBSTACLE CONSTANTS
const vector_t PLANT_BOY_OBSTACLE_CENTROID = {.x = 600, .y = 30};
const vector_t DIRT_GIRL_OBSTACLE_CENTROID = {.x = 1300, .y = 710};
const double OBSTACLE_LENGTH = 120;
const double OBSTACLE_HEIGHT = 20;
const rgb_color_t OBSTACLE_COLOR = {0.75, 1, 0.75}; // for visibility on top of ledge

// FERTILIZER CONSTANTS
const vector_t PLANT_BOY_FERTILIZER_CENTROID = {.x = 900, .y = 470};
const vector_t DIRT_GIRL_FERTILIZER_CENTROID = {.x = 1500, .y = 120};
const double FERTILIZER_LENGTH = 40;

typedef enum {
  PLANT_BOY = 1,
  DIRT_GIRL = 2,
  PLANT_BOY_OBSTACLE = 3, // obstacle for plantboy
  DIRT_GIRL_OBSTACLE = 4, // obstacle for dirtgirl
  SLUDGE = 5, // obstacle for both players
  PLANT_BOY_DOOR_RIGHT = 6, // finish line for plantboy
  PLANT_BOY_DOOR_LEFT = 7, // finish line for plantboy
  DIRT_GIRL_DOOR_RIGHT = 8, // finish line for dirtgirl
  DIRT_GIRL_DOOR_LEFT = 9, // finish line for dirtgirl
  LEDGE = 10, // for characters to walk on
  BLOCK = 11, // to assist the characters in jumping
  WALL = 12, // edges of screen
  PLANT_BOY_FERTILIZER = 13,
  DIRT_GIRL_FERTILIZER = 14,

} info_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;


list_t *find_ledges(scene_t *scene){
  list_t *ledges = list_init(3, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == LEDGE){
      list_add(ledges, scene_get_body(scene,i));
    }
  }
  return ledges;
}

list_t *get_game_over_bodies(scene_t *scene) {
  list_t *answer = list_init(8, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    void *info = body_get_info(scene_get_body(scene,i));
    if (info == PLANT_BOY_DOOR_LEFT || info == PLANT_BOY_DOOR_RIGHT ||
        info == DIRT_GIRL_DOOR_LEFT || info == DIRT_GIRL_DOOR_RIGHT ||
        info == PLANT_BOY_OBSTACLE || info == DIRT_GIRL_OBSTACLE) {
      list_add(answer, scene_get_body(scene,i));
    }
  }
  return answer;
}

list_t *get_fertilizer_bodies(scene_t *scene) {
  list_t *answer = list_init(8, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    void *info = body_get_info(scene_get_body(scene,i));
    if (info == PLANT_BOY_FERTILIZER || info == DIRT_GIRL_FERTILIZER) {
      list_add(answer, scene_get_body(scene,i));
    }
  }
  return answer;
}

list_t *make_wall_shape(char wall) {
  list_t *wall_points = list_init(NUM_RECT_POINTS, free);
  // top left, bottom left, bottom right, top right
  for (size_t i = 0; i < NUM_RECT_POINTS;i++) {        
    if (wall == 0) { // left
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MIN.x;
      else
        point->x = SDL_MIN.x + WALL_LENGTH;
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
        point->y = SDL_MAX.y - WALL_LENGTH;
      list_add(wall_points, point);
    }
    if (wall == 2) { // right
      vector_t *point = malloc(sizeof(vector_t));
      if (i == 0 || i == 1)
        point->x = SDL_MAX.x - WALL_LENGTH;
      else
        point->x = SDL_MAX.x;
      if (i == 0 || i == 3)
        point->y = SDL_MAX.y;
      else
        point->y = SDL_MIN.y;
      list_add(wall_points, point);
    }
  }
  return wall_points;
}

//make a universal gravity for "players"
void add_universal_gravity(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) != LEDGE || body_get_info(body) != WALL) {
      create_universal_gravity(scene, body, GRAVITY);
    }
  }
}

//make normal force between the ground and the players
void add_normal_force(scene_t *scene) {
  list_t *ledges = find_ledges(scene);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) != LEDGE || body_get_info(body) != WALL) {
      for (size_t j = 0; j < list_size(ledges); j++) {
        body_t *ledge = list_get(ledges, j);
        create_normal_force(scene, body, ledge, GRAVITY);
      }
    }
  }
}

void add_walls(scene_t *scene) {
  body_t *left_wall = body_init_with_info(make_wall_shape(0), INFINITY_MASS,
                                          COLOR, WALL, NULL);
  body_t *top_wall = body_init_with_info(make_wall_shape(1), INFINITY_MASS,
                                         COLOR, WALL, NULL);
  body_t *right_wall = body_init_with_info(make_wall_shape(2), INFINITY_MASS,
                                           COLOR, WALL, NULL);
  scene_add_body(scene, left_wall);
  scene_add_body(scene, top_wall);
  scene_add_body(scene, right_wall);
}

list_t *make_ledge_shape(vector_t centroid, double ledge_length) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - ledge_length / 2;
    else
      point->x = centroid.x + ledge_length / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + LEDGE_HEIGHT / 2;
    else
      point->y = centroid.y - LEDGE_HEIGHT / 2;
    list_add(points, point);
  }
  return points;
}

void add_ledges(scene_t *scene) {
  body_t *ledge_floor = body_init_with_info(make_ledge_shape(LEDGE_FLOOR_CENTROID, SDL_MAX.x), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *ledge_1 = body_init_with_info(make_ledge_shape(LEDGE_1_CENTROID, LEDGE_LENGTH), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *ledge_2 = body_init_with_info(make_ledge_shape(LEDGE_2_CENTROID, LEDGE_LENGTH), INFINITY_MASS, COLOR, LEDGE, NULL);   
  scene_add_body(scene, ledge_floor);     
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
  body_t *block_1 = body_init_with_info(make_block_shape(BLOCK_1_CENTROID), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *block_2 = body_init_with_info(make_block_shape(BLOCK_2_CENTROID), INFINITY_MASS, COLOR, LEDGE, NULL);          
  scene_add_body(scene, block_1);
  scene_add_body(scene, block_2);
}

list_t *make_door_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - DOOR_LENGTH / 2;
    else
      point->x = centroid.x + DOOR_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + DOOR_HEIGHT / 2;
    else
      point->y = centroid.y - DOOR_HEIGHT / 2;
    list_add(points, point);
  }
  return points;
}

void add_doors(scene_t *scene) {
  body_t *plant_boy_door_right = body_init_with_info(make_door_shape(PLANT_BOY_DOOR_RIGHT_CENTROID), INFINITY_MASS, COLOR, PLANT_BOY_DOOR_RIGHT, NULL);
  body_t *plant_boy_door_left = body_init_with_info(make_door_shape(PLANT_BOY_DOOR_LEFT_CENTROID), INFINITY_MASS, COLOR, PLANT_BOY_DOOR_LEFT, NULL);
  body_t *dirt_girl_door_right = body_init_with_info(make_door_shape(DIRT_GIRL_DOOR_RIGHT_CENTROID), INFINITY_MASS, COLOR, DIRT_GIRL_DOOR_RIGHT, NULL);        
  body_t *dirt_girl_door_left = body_init_with_info(make_door_shape(DIRT_GIRL_DOOR_LEFT_CENTROID), INFINITY_MASS, COLOR, DIRT_GIRL_DOOR_LEFT, NULL);                                                               
  scene_add_body(scene, plant_boy_door_right);
  scene_add_body(scene, plant_boy_door_left);
  scene_add_body(scene, dirt_girl_door_right);
  scene_add_body(scene, dirt_girl_door_left);
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
      point->y = centroid.y + OBSTACLE_HEIGHT / 2;
    else
      point->y = centroid.y - OBSTACLE_HEIGHT / 2;
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

list_t *make_fertilizer_shape(vector_t centroid) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - FERTILIZER_LENGTH / 2;
    else
      point->x = centroid.x + FERTILIZER_LENGTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + FERTILIZER_LENGTH / 2;
    else
      point->y = centroid.y - FERTILIZER_LENGTH / 2;
    list_add(points, point);
  }
  return points;
}

void add_fertilizer(scene_t *scene) {
  body_t *plant_boy_fertilizer = body_init_with_info(make_fertilizer_shape(PLANT_BOY_FERTILIZER_CENTROID), INFINITY_MASS, COLOR, PLANT_BOY_FERTILIZER, NULL);
  body_t *dirt_girl_fertilizer = body_init_with_info(make_fertilizer_shape(DIRT_GIRL_FERTILIZER_CENTROID), INFINITY_MASS, COLOR, DIRT_GIRL_FERTILIZER, NULL);          
  scene_add_body(scene, plant_boy_fertilizer);
  scene_add_body(scene, dirt_girl_fertilizer);
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

void add_characters(scene_t *scene) {
  list_t *plant_boy_shape = make_character_shape(INITIAL_PLANT_BOY_POSITION);
  body_t *plant_boy = body_init_with_info(plant_boy_shape, CHARACTER_MASS, COLOR, PLANT_BOY, NULL);
  scene_add_body(scene, plant_boy);
  list_t *dirt_girl_shape = make_character_shape(INITIAL_DIRT_GIRL_POSITION);
  body_t *dirt_girl = body_init_with_info(dirt_girl_shape, CHARACTER_MASS, COLOR, DIRT_GIRL, NULL);
  scene_add_body(scene, dirt_girl);
}

size_t get_dirt_girl_index(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == DIRT_GIRL){
      return i;
    }
  }
  return NULL;
}

size_t get_plant_boy_index(scene_t *scene){
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == PLANT_BOY){
      return i;
    }
  }
  return NULL;
}

void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *dirt_girl = scene_get_body(state->scene, get_dirt_girl_index(state->scene));
  body_t *plant_boy = scene_get_body(state->scene, get_plant_boy_index(state->scene));
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

void add_game_over_force(scene_t *scene) {
  list_t *game_over_bodies = get_game_over_bodies(scene);
  body_t *dirt_girl = scene_get_body(scene, get_dirt_girl_index(scene));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_plant_boy_index(scene));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(game_over_bodies); i++) {
    body_t *body = list_get(game_over_bodies, i);
    create_game_over_force(scene, dirt_girl, body);
    create_game_over_force(scene, plant_boy, body);
  }
}

void add_fertilizer_force(scene_t *scene) {
  list_t *fertilizer_bodies = get_fertilizer_bodies(scene);
  body_t *dirt_girl = scene_get_body(scene, get_dirt_girl_index(scene));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_plant_boy_index(scene));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(fertilizer_bodies); i++) {
    body_t *body = list_get(fertilizer_bodies, i);
    create_dirt_girl_fertilizer_force(scene, dirt_girl, body);
    create_plant_boy_fertilizer_force(scene, plant_boy, body);
  }
}


scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  // add bodies
  add_walls(result);
  add_ledges(result);
  add_blocks(result);
  add_doors(result);
  add_obstacles(result);
  add_fertilizer(result);
  // add players
  add_characters(result);
  // forces
  add_universal_gravity(result);
  add_normal_force(result);
  add_game_over_force(result);
  add_fertilizer_force(result);
  return result;
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

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}

void emscripten_main(state_t *state) {
  sdl_clear();
  scene_t *scene = state->scene;
  assert(scene);
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    if (body_get_info(body) != WALL && body_get_info(body) != PLANT_BOY_FERTILIZER && body_get_info(body) != DIRT_GIRL_FERTILIZER) {
      sdl_draw_polygon(shape, body_get_color(body));
    }
    if (scene_get_plant_boy_fertilizer_collected(state->scene) == false) {
      if (body_get_info(body) == PLANT_BOY_FERTILIZER)
        sdl_draw_polygon(shape, body_get_color(body));
    }
    if (scene_get_dirt_girl_fertilizer_collected(state->scene) == false) {
      if (body_get_info(body) == DIRT_GIRL_FERTILIZER)
        sdl_draw_polygon(shape, body_get_color(body));
    }
  }
  scene_tick(scene, dt);
  sdl_show();
  if (scene_get_game_over(state->scene)) {
    scene_free(state->scene);
    state->scene = make_initial_scene();
  }
  if (scene_get_plant_boy_fertilizer_collected(state->scene) && scene_get_dirt_girl_fertilizer_collected(state->scene)) {
    printf("star of mastery\n");
  }
}

