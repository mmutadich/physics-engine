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
#include <stdbool.h>

// FORCE CONSTANTS
const double GRAVITY = -25000;

// WINDOW CONSTANTS
const vector_t SDL_MIN = {.x = 0, .y = 0};
const vector_t SDL_MAX = {.x = 2000, .y = 1000};
const vector_t ZERO_VECTOR = {.x = 0, .y = 0};
const rgb_color_t COLOR = {1, 0.75, 0.75}; // general color used for everything since we will add drawings on top
const rgb_color_t BOUNDARY_COLOR = {.5, 0.9, 0.5}; //color to see the boundaries
const rgb_color_t BACKGROUND_COLOR = {1, 1, 1}; 

// GENERAL BODY CONSTANTS
const NUM_RECT_POINTS = 4;
const double INFINITY_MASS = INFINITY;
const double ELASTICITY = 0.5;

// CHARACTER CONSTANTS
const vector_t INITIAL_PLANT_BOY_POSITION = {.x = 100, .y = 80};
const vector_t INITIAL_DIRT_GIRL_POSITION = {.x = 300, .y = 80};
const double CHARACTER_VELOCITY = 650;
const double CHARACTER_SIDE_LENGTH = 80;
const double CHARACTER_MASS = 10;

// WALL CONSTANTS
const double WALL_LENGTH = 10;

// LEDGE CONSTANTS
const vector_t LEDGE_FLOOR_CENTROID = {.x = 1000, .y = 20};
const vector_t LEDGE_1_CENTROID = {.x = 700, .y = 350};
const vector_t LEDGE_2_CENTROID = {.x = 1300, .y = 700};
const vector_t BOUNDARY_1_CENTROID = {.x = 700, .y = 330};
const vector_t BOUNDARY_2_CENTROID = {.x = 1300, .y = 680};
const double LEDGE_LENGTH = 1700;
const double FLOOR_LENGTH = 2200;
const double LEDGE_HEIGHT = 20;

// BLOCK CONSTANTS
const vector_t BLOCK_1_CENTROID = {.x = 1900, .y = 190};
const vector_t BLOCK_2_CENTROID = {.x = 80, .y = 550};
const vector_t BOUNDARY_BLOCK_1_CENTROID = {.x = 1810, .y = 95};
const vector_t BOUNDARY_BLOCK_2_CENTROID = {.x = 180, .y = 455};
const double BLOCK_LENGTH = 190;

// DOOR CONSTANTS
const vector_t PLANT_BOY_DOOR_RIGHT_CENTROID = {.x = 1577, .y = 780};
const vector_t PLANT_BOY_DOOR_LEFT_CENTROID = {.x = 1627, .y = 780};
const vector_t DIRT_GIRL_DOOR_RIGHT_CENTROID = {.x = 1777, .y = 780};
const vector_t DIRT_GIRL_DOOR_LEFT_CENTROID = {.x = 1827, .y = 780};
const double DOOR_HEIGHT = 120;
const double DOOR_LENGTH = 10;

// OBSTACLE/ICE CONSTANTS
const vector_t PLANT_BOY_OBSTACLE_CENTROID = {.x = 725, .y = 30};
const vector_t DIRT_GIRL_OBSTACLE_CENTROID = {.x = 1310, .y = 710};
const double OBSTACLE_LENGTH = 140;
const double OBSTACLE_HEIGHT = 20;
const rgb_color_t OBSTACLE_COLOR = {0.75, 1, 0.75}; // for visibility on top of ledge
const vector_t ICE_CENTROID = {.x = 1300, .y= 370};

// FERTILIZER CONSTANTS
const vector_t PLANT_BOY_FERTILIZER_CENTROID = {.x = 900, .y = 470};
const vector_t DIRT_GIRL_FERTILIZER_CENTROID = {.x = 1480, .y = 130};
const double FERTILIZER_LENGTH = 40;

// STAR CONSTANTS
const vector_t STAR_CENTROID = {.x = 350, .y = 850};
const rgb_color_t STAR_COLOR = {1, 1, 0}; // for visibility

// PORTAL CONSTANTS
const vector_t ENTRY_PORTAL_CENTROID = {.x = 1980, .y = 260};
const vector_t EXIT_PORTAL_CENTROID = {.x = 1000, .y = 790};
const double PORTAL_HEIGHT = 140;
const double PORTAL_LENGTH = 20;
const rgb_color_t PORTAL_COLOR = {0.5, 0, 1};

// TRAMPLOINE CONSTANTS
const vector_t TRAMPOLINE_CENTROID = {.x = 300, .y= 400};
const double TRAMPOLINE_HEIGHT = 40;
const double TRAMPOLINE_LENGTH = 120;

// TREE CONSTANTS
const vector_t PLANT_BOY_TREE_CENTROID = {.x = 625, .y = 50};
const vector_t DIRT_GIRL_TREE_CENTROID = {.x = 1300, .y = 730};

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
  STAR = 15,
  TREE = 16,
  PORTAL = 17,
  TRAMPOLINE = 18,
  ICE = 19,
  BOUNDARY = 20,
} info_t;

typedef enum {
  START_SCREEN = 0,
  GAME_SCREEN = 1,
  WIN_SCREEN = 2,
} screen_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

list_t *get_x_bodies(scene_t *scene, void *info1, void *info2){
  list_t *bodies = list_init(8, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == info1 || body_get_info(scene_get_body(scene,i)) == info2)
      list_add(bodies, scene_get_body(scene,i));
  }
  return bodies;
}

size_t get_x_index(scene_t *scene, void *info){
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene,i)) == info)
      return i;
  }
  return NULL;
}

bool doesnt_contain_star(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) == STAR) {
      return false;
    }
  }
  return true;
}

bool doesnt_contain_tree(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) == TREE) {
      return false;
    }
  }
  return true;
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

void add_walls(scene_t *scene) {
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  vector_t *dimensions_character = malloc(sizeof(vector_t));
  dimensions_character->x = CHARACTER_SIDE_LENGTH;
  dimensions_character->y = CHARACTER_SIDE_LENGTH;
  vector_t *dimensions_wall = malloc(sizeof(vector_t));

  for (size_t i = 0; i < 3; i++) {
    body_t *wall = body_init_with_info(make_wall_shape(i), INFINITY_MASS,
                                        COLOR, WALL, NULL);
    scene_add_body(scene, wall);
    if (i == 0) { 
      dimensions_wall->x = WALL_LENGTH;
      dimensions_wall->y = SDL_MAX.y; }
    if (i == 1) {
      dimensions_wall->x = SDL_MAX.x;
      dimensions_wall->y = WALL_LENGTH; }
    if (i == 2) {
      dimensions_wall->x = WALL_LENGTH;
      dimensions_wall->y = SDL_MAX.y; }
    list_t *dimensions = list_init(2, free);
    list_add(dimensions, dimensions_character);
    list_add(dimensions, dimensions_wall);
    create_boundary_force(scene, dirt_girl, wall, dimensions);
    create_boundary_force(scene, plant_boy, wall, dimensions);
  }
}

list_t *make_rect_shape(vector_t centroid, double length, double height) {
  list_t *points = list_init(NUM_RECT_POINTS, free);
  for (size_t i = 0; i < NUM_RECT_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - length / 2;
    else
      point->x = centroid.x + length / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + height / 2;
    else
      point->y = centroid.y - height / 2;
    list_add(points, point);
  }
  return points;
}

void add_ledges(scene_t *scene) {
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  vector_t *dimensions_character = malloc(sizeof(vector_t));
  dimensions_character->x = CHARACTER_SIDE_LENGTH;
  dimensions_character->y = CHARACTER_SIDE_LENGTH;
  vector_t *dimensions_ledge = malloc(sizeof(vector_t));
  dimensions_ledge->x = LEDGE_LENGTH;
  dimensions_ledge->y = LEDGE_HEIGHT;

  body_t *ledge_floor = body_init_with_info(make_rect_shape(LEDGE_FLOOR_CENTROID, FLOOR_LENGTH, LEDGE_HEIGHT), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *ledge_1 = body_init_with_info(make_rect_shape(LEDGE_1_CENTROID, LEDGE_LENGTH, LEDGE_HEIGHT), INFINITY_MASS, COLOR, LEDGE, NULL);
  body_t *ledge_2 = body_init_with_info(make_rect_shape(LEDGE_2_CENTROID, LEDGE_LENGTH, LEDGE_HEIGHT), INFINITY_MASS, COLOR, LEDGE, NULL);   
  scene_add_body(scene, ledge_floor);     
  scene_add_body(scene, ledge_1);
  scene_add_body(scene, ledge_2);
  
  body_t *boundary_1 = body_init_with_info(make_rect_shape(BOUNDARY_1_CENTROID, LEDGE_LENGTH, LEDGE_HEIGHT), INFINITY_MASS, BOUNDARY_COLOR, LEDGE, NULL);
  body_t *boundary_2 = body_init_with_info(make_rect_shape(BOUNDARY_2_CENTROID, LEDGE_LENGTH, LEDGE_HEIGHT), INFINITY_MASS, BOUNDARY_COLOR, LEDGE, NULL);   
  scene_add_body(scene, boundary_1);
  scene_add_body(scene, boundary_2);
  list_t *dimensions = list_init(2, free);
  list_add(dimensions, dimensions_character);
  list_add(dimensions, dimensions_ledge);
  create_boundary_force(scene, dirt_girl, boundary_1, dimensions);
  create_boundary_force(scene, plant_boy, boundary_1, dimensions);
  create_boundary_force(scene, dirt_girl, boundary_2, dimensions);
  create_boundary_force(scene, plant_boy, boundary_2, dimensions);
}

void add_blocks(scene_t *scene) {
  body_t *block_1 = body_init_with_info(make_rect_shape(BLOCK_1_CENTROID, BLOCK_LENGTH, WALL_LENGTH), INFINITY_MASS, COLOR, BLOCK, NULL);
  body_t *block_2 = body_init_with_info(make_rect_shape(BLOCK_2_CENTROID, BLOCK_LENGTH, WALL_LENGTH), INFINITY_MASS, COLOR, BLOCK, NULL);          
  scene_add_body(scene, block_1);
  scene_add_body(scene, block_2);

  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  vector_t *dimensions_character = malloc(sizeof(vector_t));
  dimensions_character->x = CHARACTER_SIDE_LENGTH;
  dimensions_character->y = CHARACTER_SIDE_LENGTH;
  vector_t *dimensions_block = malloc(sizeof(vector_t));
  dimensions_block->x = WALL_LENGTH * 2;
  dimensions_block->y = BLOCK_LENGTH;
  body_t *boundary_block_1 = body_init_with_info(make_rect_shape(BOUNDARY_BLOCK_1_CENTROID, WALL_LENGTH * 2, BLOCK_LENGTH), INFINITY_MASS, BOUNDARY_COLOR, BOUNDARY, NULL);
  body_t *boundary_block_2 = body_init_with_info(make_rect_shape(BOUNDARY_BLOCK_2_CENTROID, WALL_LENGTH * 2, BLOCK_LENGTH), INFINITY_MASS, BOUNDARY_COLOR, BOUNDARY, NULL);          
  scene_add_body(scene, boundary_block_1);
  scene_add_body(scene, boundary_block_2);

  list_t *dimensions = list_init(2, free);
  list_add(dimensions, dimensions_character);
  list_add(dimensions, dimensions_block);
  create_boundary_force(scene, dirt_girl, boundary_block_1, dimensions);
  create_boundary_force(scene, plant_boy, boundary_block_1, dimensions);
  create_boundary_force(scene, dirt_girl, boundary_block_2, dimensions);
  create_boundary_force(scene, plant_boy, boundary_block_2, dimensions);
}

void add_doors(scene_t *scene) {
  body_t *plant_boy_door_right = body_init_with_info(make_rect_shape(PLANT_BOY_DOOR_RIGHT_CENTROID, DOOR_LENGTH, DOOR_HEIGHT), INFINITY_MASS, COLOR, PLANT_BOY_DOOR_RIGHT, NULL);
  body_t *plant_boy_door_left = body_init_with_info(make_rect_shape(PLANT_BOY_DOOR_LEFT_CENTROID, DOOR_LENGTH, DOOR_HEIGHT), INFINITY_MASS, COLOR, PLANT_BOY_DOOR_LEFT, NULL);
  body_t *dirt_girl_door_right = body_init_with_info(make_rect_shape(DIRT_GIRL_DOOR_RIGHT_CENTROID, DOOR_LENGTH, DOOR_HEIGHT), INFINITY_MASS, COLOR, DIRT_GIRL_DOOR_RIGHT, NULL);        
  body_t *dirt_girl_door_left = body_init_with_info(make_rect_shape(DIRT_GIRL_DOOR_LEFT_CENTROID, DOOR_LENGTH, DOOR_HEIGHT), INFINITY_MASS, COLOR, DIRT_GIRL_DOOR_LEFT, NULL);                                                               
  scene_add_body(scene, plant_boy_door_right);
  scene_add_body(scene, plant_boy_door_left);
  scene_add_body(scene, dirt_girl_door_right);
  scene_add_body(scene, dirt_girl_door_left);
}

void add_obstacles(scene_t *scene) {
  body_t *plant_boy_obstacle = body_init_with_info(make_rect_shape(PLANT_BOY_OBSTACLE_CENTROID, OBSTACLE_LENGTH, OBSTACLE_HEIGHT), INFINITY_MASS, OBSTACLE_COLOR, PLANT_BOY_OBSTACLE, NULL);
  body_t *dirt_girl_obstacle = body_init_with_info(make_rect_shape(DIRT_GIRL_OBSTACLE_CENTROID, OBSTACLE_LENGTH, OBSTACLE_HEIGHT), INFINITY_MASS, OBSTACLE_COLOR, DIRT_GIRL_OBSTACLE, NULL);                                     
  scene_add_body(scene, plant_boy_obstacle);
  scene_add_body(scene, dirt_girl_obstacle);
  body_t *ice = body_init_with_info(make_rect_shape(ICE_CENTROID, OBSTACLE_LENGTH, OBSTACLE_HEIGHT), INFINITY_MASS, OBSTACLE_COLOR, ICE, NULL);   
  scene_add_body(scene, ice);                             
}

void add_characters(scene_t *scene) {
  list_t *plant_boy_shape = make_rect_shape(INITIAL_PLANT_BOY_POSITION, CHARACTER_SIDE_LENGTH, CHARACTER_SIDE_LENGTH);
  body_t *plant_boy = body_init_with_info(plant_boy_shape, CHARACTER_MASS, COLOR, PLANT_BOY, NULL);
  scene_add_body(scene, plant_boy);
  list_t *dirt_girl_shape = make_rect_shape(INITIAL_DIRT_GIRL_POSITION, CHARACTER_SIDE_LENGTH, CHARACTER_SIDE_LENGTH);
  body_t *dirt_girl = body_init_with_info(dirt_girl_shape, CHARACTER_MASS, COLOR, DIRT_GIRL, NULL);
  scene_add_body(scene, dirt_girl);
}

void add_object_features(scene_t *scene) {
  body_t *plant_boy_fertilizer = body_init_with_info(make_rect_shape(PLANT_BOY_FERTILIZER_CENTROID, FERTILIZER_LENGTH, FERTILIZER_LENGTH), INFINITY_MASS, COLOR, PLANT_BOY_FERTILIZER, NULL);
  body_t *dirt_girl_fertilizer = body_init_with_info(make_rect_shape(DIRT_GIRL_FERTILIZER_CENTROID, FERTILIZER_LENGTH, FERTILIZER_LENGTH), INFINITY_MASS, COLOR, DIRT_GIRL_FERTILIZER, NULL);          
  scene_add_body(scene, plant_boy_fertilizer);
  scene_add_body(scene, dirt_girl_fertilizer);

  body_t *entry_portal = body_init_with_info(make_rect_shape(ENTRY_PORTAL_CENTROID, PORTAL_LENGTH, PORTAL_HEIGHT), INFINITY_MASS, PORTAL_COLOR, PORTAL, NULL);
  body_t *exit_portal = body_init_with_info(make_rect_shape(EXIT_PORTAL_CENTROID, PORTAL_LENGTH, PORTAL_HEIGHT), INFINITY_MASS, PORTAL_COLOR, PORTAL, NULL);
  scene_add_body(scene, entry_portal);
  scene_add_body(scene, exit_portal);

  body_t *trampoline = body_init_with_info(make_rect_shape(TRAMPOLINE_CENTROID, TRAMPOLINE_LENGTH, TRAMPOLINE_HEIGHT), INFINITY_MASS, COLOR, TRAMPOLINE, NULL);
  scene_add_body(scene, trampoline);
}

void add_star(scene_t *scene) {
  body_t *star = body_init_with_info(make_rect_shape(STAR_CENTROID, BLOCK_LENGTH, BLOCK_LENGTH), INFINITY_MASS, STAR_COLOR, STAR, NULL);
  scene_add_body(scene, star);
}

void add_tree(scene_t *scene, vector_t centroid) {
  body_t *tree = body_init_with_info(make_rect_shape(centroid, FERTILIZER_LENGTH, FERTILIZER_LENGTH), INFINITY_MASS, STAR_COLOR, TREE, NULL);
  scene_add_body(scene, tree);
}

void add_universal_gravity(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) != LEDGE || body_get_info(body) != BLOCK) {
      create_universal_gravity(scene, body, GRAVITY);
    }
  }
}

void add_normal_force(scene_t *scene) {
  list_t *ledges = get_x_bodies(scene, LEDGE, BLOCK);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) != LEDGE || body_get_info(body) != BLOCK) {
      for (size_t j = 0; j < list_size(ledges); j++) {
        body_t *ledge = list_get(ledges, j);
        create_normal_force(scene, body, ledge, GRAVITY);
      }
    }
  }
}

void add_game_over_force(scene_t *scene) {
  list_t *game_over_bodies = get_x_bodies(scene, PLANT_BOY_OBSTACLE, DIRT_GIRL_OBSTACLE); // game over bodies
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(game_over_bodies); i++) {
    body_t *body = list_get(game_over_bodies, i);
    create_game_over_force(scene, dirt_girl, body);
    create_game_over_force(scene, plant_boy, body);
  }
}

void add_win_force(scene_t *scene) {
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);

  list_t *bodies = list_init(10, body_free);
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    void *info = body_get_info(scene_get_body(scene,i));
    if (info == PLANT_BOY_DOOR_LEFT || info == PLANT_BOY_DOOR_RIGHT) {
      list_add(bodies, dirt_girl);
      list_add(bodies, scene_get_body(scene,i));
    }
    else if (info == DIRT_GIRL_DOOR_LEFT || info == DIRT_GIRL_DOOR_RIGHT) {
      list_add(bodies, plant_boy);
      list_add(bodies, scene_get_body(scene,i));
    }
  }
  guarantee_all_collisions(scene, bodies);
}

void add_obstacle_force(scene_t *scene) {
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));  
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  body_t *plant_boy_obstacle = scene_get_body(scene, get_x_index(scene, PLANT_BOY_OBSTACLE));
  body_t *dirt_girl_obstacle = scene_get_body(scene, get_x_index(scene, DIRT_GIRL_OBSTACLE));
  create_dirt_girl_obstacle_force(scene, dirt_girl, dirt_girl_obstacle);
  create_plant_boy_obstacle_force(scene, plant_boy, plant_boy_obstacle);
}

void add_fertilizer_force(scene_t *scene) {
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));  
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  body_t *plant_boy_fertilizer = scene_get_body(scene, get_x_index(scene, PLANT_BOY_FERTILIZER));
  body_t *dirt_girl_fertilizer = scene_get_body(scene, get_x_index(scene, DIRT_GIRL_FERTILIZER));
  create_dirt_girl_fertilizer_force(scene, dirt_girl, dirt_girl_fertilizer);
  create_plant_boy_fertilizer_force(scene, plant_boy, plant_boy_fertilizer);
}

void add_portal_force(scene_t *scene) {
  list_t *portals = get_x_bodies(scene, PORTAL, NULL);
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(portals); i+=2) {
    body_t *portal_1 = list_get(portals, i);
    body_t *portal_2 = list_get(portals, i+1);
    create_portal_force(scene, dirt_girl, portal_1, portal_2, 0);
    create_portal_force(scene, plant_boy, portal_1, portal_2, 0);
  }
}

void add_trampoline_force(scene_t *scene) {
  list_t *trampolines = get_x_bodies(scene, TRAMPOLINE, NULL);
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(trampolines); i+=2) {
    body_t *trampoline = list_get(trampolines, i);
    create_trampoline_force(scene, dirt_girl, trampoline, 0);
    create_trampoline_force(scene, plant_boy, trampoline, 0);
  }
}

void add_ice_force(scene_t *scene) {
  list_t *ice_bodies = get_x_bodies(scene, ICE, NULL);
  body_t *dirt_girl = scene_get_body(scene, get_x_index(scene, DIRT_GIRL));
  assert(body_get_info(dirt_girl) == DIRT_GIRL);
  body_t *plant_boy = scene_get_body(scene, get_x_index(scene, PLANT_BOY));
  assert(plant_boy);
  for (size_t i = 0; i < list_size(ice_bodies); i+=2) {
    body_t *ice = list_get(ice_bodies, i);
    create_ice_force(scene, dirt_girl, ice, 0);
    create_ice_force(scene, plant_boy, ice, 0);
  }
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  // bodies
  add_characters(result);
  add_ledges(result);
  add_walls(result);
  add_blocks(result);
  add_doors(result);
  add_obstacles(result);
  add_object_features(result);
  // forces
  add_universal_gravity(result);
  add_normal_force(result);
  add_game_over_force(result);
  add_obstacle_force(result);
  add_fertilizer_force(result);
  //add_boundary_force(result);
  add_portal_force(result);
  add_trampoline_force(result);
  add_ice_force(result);
  add_win_force(result);
  return result;
}

void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *dirt_girl;
  body_t *plant_boy;
  if (scene_get_screen(state->scene) == GAME_SCREEN) {
    dirt_girl = scene_get_body(state->scene, get_x_index(state->scene, DIRT_GIRL));
    plant_boy = scene_get_body(state->scene, get_x_index(state->scene, PLANT_BOY));
    assert(body_get_info(dirt_girl) == DIRT_GIRL);
    assert(body_get_info(plant_boy) == PLANT_BOY);
  }
  if (type == KEY_PRESSED) {
    if (key == ' ' && scene_get_screen(state->scene) == START_SCREEN) {
      state->scene = make_initial_scene();
      scene_set_screen(state->scene, GAME_SCREEN);
    }
    if (key == D_KEY) {
      vector_t velocity = {.x = CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(dirt_girl, velocity);
    }
    if (key == A_KEY) {
      vector_t velocity = {.x = -CHARACTER_VELOCITY, .y = 0};
      body_set_velocity(dirt_girl, velocity);
    }
    if (key == W_KEY) {
      if ( held_time < 0.01){
        list_t *ledges = get_x_bodies(state->scene, LEDGE, BLOCK);
        for (size_t i = 0; i < list_size(ledges); i++) {
          body_t *ledge = list_get(ledges, i);
          assert(dirt_girl);
          assert(ledge);
          jump_up(state->scene, dirt_girl, ledge, ELASTICITY);
        }
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
      if (held_time < 0.01 ) {
        list_t *ledges = get_x_bodies(state->scene, LEDGE, BLOCK);
        for (size_t i = 0; i < list_size(ledges); i++) {
          body_t *ledge = list_get(ledges, i);
          assert(plant_boy);
          assert(ledge);
          jump_up(state->scene, plant_boy, ledge, ELASTICITY);
        }
      }
    }
  } else {
    body_set_velocity(dirt_girl, ZERO_VECTOR);
    body_set_velocity(plant_boy, ZERO_VECTOR);
  }
}

scene_t *make_start_scene() {
  scene_t *result = scene_init();
  return result;
}

state_t *emscripten_init() {
  sdl_init(SDL_MIN, SDL_MAX);
  sdl_on_key((key_handler_t)keyer);
  state_t *state = malloc(sizeof(state_t));
  assert(state);
  state->scene = make_start_scene();
  scene_set_screen(state->scene, START_SCREEN);
  state->time_elapsed = 0;
  return state;
}

void emscripten_main(state_t *state) {
  assert(state->scene);
  sdl_clear();
  double dt = time_since_last_tick();

  if (scene_get_screen(state->scene) == WIN_SCREEN) {
    scene_free(state->scene);
    state->scene = make_start_scene();
    scene_set_screen(state->scene, START_SCREEN);
  }
  if (scene_get_screen(state->scene) == START_SCREEN) {
    scene_tick(state->scene, dt);
    sdl_show();
    //need to render the background here?
  }
  if (scene_get_screen(state->scene) == GAME_SCREEN) {
    state->time_elapsed += dt;
    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
      body_t *body = scene_get_body(state->scene, i);
      list_t *shape = body_get_shape(body);
      if (body_get_info(body) != WALL) {
        sdl_draw_polygon(shape, body_get_color(body));
      }
    }
    scene_tick(state->scene, dt);
    sdl_show();
    if (scene_get_game_over(state->scene)) {
      scene_free(state->scene);
      state->scene = make_initial_scene();
      scene_set_screen(state->scene, GAME_SCREEN);
    }
    if (scene_get_plant_boy_fertilizer_collected(state->scene) && scene_get_dirt_girl_fertilizer_collected(state->scene) && doesnt_contain_star(state->scene)) {
      add_star(state->scene);
    }
    if (scene_get_plant_boy_obstacle_hit(state->scene) && doesnt_contain_tree(state->scene)) {
      add_tree(state->scene, PLANT_BOY_TREE_CENTROID);
      printf("plant boy tree\n");
    }
    if (scene_get_plant_boy_obstacle_hit(state->scene) && doesnt_contain_tree(state->scene)) {
      add_tree(state->scene, DIRT_GIRL_TREE_CENTROID);
      printf("dirt girl tree\n");
    }
  }
  sdl_render_scene(state->scene);
}

void emscripten_free(state_t *state) {
  scene_free(state->scene);
  free(state);
}