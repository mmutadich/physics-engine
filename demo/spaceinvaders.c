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
const size_t NUM_ENEMIES = 24; // 3 rows, 8 columns
const size_t NUM_ROWS = 3;
const double ENEMY_RADIUS = 80;
const size_t POINTS_ON_ENEMY_ARC = 5; // points being draw for enemy
const rgb_color_t ENEMY_COLOR = {1, 0.75, 0.75};
const double DISTANCE_BETWEEN_ENEMIES = 200;
const vector_t ENEMY_VELOCITY = {.x = 100, .y = 0};
const vector_t ZERO_VECTOR = {.x = 0, .y = 0};
const double PLAYER_RADIUS = 100;

const size_t TIME_BETWEEN_ENEMY_BULLETS = 5;
const rgb_color_t BULLET_COLOR = {0.75, 1, 0.75};
const double BULLET_MASS = 10;
const vector_t BULLET_VELOCITY = {.x = 0, .y = -1000};
const size_t NUM_BULLET_POINTS = 8;
const double BULLET_RADIUS = 10;

const double PLAYER_HEIGHT = 50;
const size_t NUM_PLAYER_POINTS = 8;
const vector_t INITIAL_PLAYER_POSITION = {.x = 1000, .y = 100};
const rgb_color_t PLAYER_COLOR = {0.75, 0.75, 1};

const double CHARACTER_MASS = 100;
const double VELOCITY_MAGNITUDE = 300;

typedef enum {
  PLAYER = 0,
  ENEMY = 1,
  PLAYER_BULLET = 2,
  ENEMY_BULLET = 3
} info_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

void add_vec_by_value(list_t *list, double x, double y) {
  vector_t *vec = malloc(sizeof(vector_t));
  vec->x = x;
  vec->y = y;
  list_add(list, vec);
}

body_t *get_player(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene, i)) == PLAYER)
      return scene_get_body(scene, i);
  }
  return NULL;
}

list_t *make_player_points() {
  list_t *points = list_init(NUM_PLAYER_POINTS, free);
  double point_coords[5] = {0, 30, 50, 92, 100};
  add_vec_by_value(points, point_coords[0], point_coords[2]);
  add_vec_by_value(points, -1 * point_coords[3], point_coords[1]);
  add_vec_by_value(points, -1 * point_coords[4], point_coords[0]);
  add_vec_by_value(points, -1 * point_coords[3], -1 * point_coords[1]);
  add_vec_by_value(points, point_coords[0], -1 * point_coords[2]);
  add_vec_by_value(points, point_coords[3], -1 * point_coords[1]);
  add_vec_by_value(points, point_coords[4], point_coords[0]);
  add_vec_by_value(points, point_coords[3], point_coords[1]);
  return points;
}

// adds the player body to the scene
void add_player(scene_t *scene) {
  list_t *points = make_player_points();
  body_t *player =
      body_init_with_info(points, CHARACTER_MASS, PLAYER_COLOR, PLAYER, NULL);
  body_set_centroid(player, INITIAL_PLAYER_POSITION);
  scene_add_body(scene, player);
}

/**
 * Makes a list of points for the enemy around the centroid
 */
list_t *make_enemy_points(vector_t centroid) {
  list_t *points = list_init(POINTS_ON_ENEMY_ARC, free);
  double start_angle = -M_PI * 2 / POINTS_ON_ENEMY_ARC;
  for (size_t i = 0; i < POINTS_ON_ENEMY_ARC; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    point->x = centroid.x;
    point->y = centroid.y + ENEMY_RADIUS;
    vector_t v = vec_subtract(*point, centroid);
    v = vec_rotate(v, ((M_PI / POINTS_ON_ENEMY_ARC) * i + start_angle));
    v = vec_add(v, centroid);

    vector_t *vec = malloc(sizeof(vector_t));
    assert(vec);
    vec->x = v.x;
    vec->y = v.y;
    point = vec;
    list_add(points, point);
  }
  vector_t *cen = malloc(sizeof(vector_t));
  cen->x = centroid.x;
  cen->y = centroid.y;
  list_add(points, cen);
  return points;
}

/**
 * Makes the enemies for the initial scene
 * Should initialize them in the correct array of spots
 * Number of enemies never increases
 */
void add_enemies(scene_t *scene) {
  for (size_t i = 0; i < NUM_ENEMIES; i++) {
    double x =
        SDL_MIN.x + (2 * ENEMY_RADIUS) +
        (SDL_MAX.x / (NUM_ENEMIES / NUM_ROWS) * (i % (NUM_ENEMIES / NUM_ROWS)));
    double y = 0;
    if (i < NUM_ENEMIES / NUM_ROWS) // first row
      y = SDL_MAX.y - 1 * ENEMY_RADIUS;
    else if (i < 2 * NUM_ENEMIES / NUM_ROWS) // second row
      y = SDL_MAX.y - 2 * ENEMY_RADIUS;
    else // third row
      y = SDL_MAX.y - 3 * ENEMY_RADIUS;
    vector_t centroid = {.x = x, .y = y};
    list_t *shape = make_enemy_points(centroid);
    body_t *body =
        body_init_with_info(shape, CHARACTER_MASS, ENEMY_COLOR, ENEMY, NULL);
    body_set_velocity(body, ENEMY_VELOCITY);
    scene_add_body(scene, body);
  }
}

/**
 * Makes a list of points for the bullet around the centroid
 */
list_t *make_bullet_points(vector_t centroid) {
  list_t *points = list_init(NUM_BULLET_POINTS, free);
  for (size_t i = 0; i < NUM_BULLET_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    point->x = centroid.x;
    point->y = centroid.y + BULLET_RADIUS;
    vector_t v = vec_subtract(*point, centroid);
    v = vec_rotate(v, (2 * M_PI / NUM_BULLET_POINTS) * i);
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

/**
 * Makes a bullet body that initializes at a certain enemies position?
 * Should be called after a certain amount of time in emscripten main?
 */
body_t *spawn_enemy_bullet(scene_t *scene) {
  size_t rand_enemy_index = rand() % (size_t)scene_bodies(scene);
  while (body_get_info(scene_get_body(scene, rand_enemy_index)) != ENEMY)
    rand_enemy_index = rand() % (size_t)scene_bodies(scene);
  body_t *rand_body = scene_get_body(scene, rand_enemy_index);
  vector_t body_centroid = body_get_centroid(rand_body);
  vector_t difference = {.x = 0, .y = -1 * ENEMY_RADIUS / 2};
  vector_t bullet_centroid = vec_add(body_centroid, difference);
  list_t *points = make_bullet_points(bullet_centroid);
  body_t *result = body_init_with_info(points, BULLET_MASS, BULLET_COLOR,
                                       ENEMY_BULLET, NULL);
  body_set_velocity(result, BULLET_VELOCITY);
  create_destructive_collision(scene, result, get_player(scene));
  return result;
}

/**
 * Makes a bullet body that initializes at the players position?
 * Should be called in the key handler
 */
body_t *spawn_player_bullet(scene_t *scene) {
  body_t *player = get_player(scene); // 0th index = player
  assert(body_get_info(player) == PLAYER);
  vector_t body_centroid = body_get_centroid(player);
  vector_t difference = {.x = 0, .y = PLAYER_HEIGHT / 2};
  vector_t bullet_centroid = vec_add(body_centroid, difference);
  list_t *points = make_bullet_points(bullet_centroid);
  body_t *result = body_init_with_info(points, BULLET_MASS, BULLET_COLOR,
                                       PLAYER_BULLET, NULL);
  body_set_velocity(result, vec_multiply(-1, BULLET_VELOCITY));
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene, i)) == ENEMY) {
      create_destructive_collision(scene, result, scene_get_body(scene, i));
    }
  }
  return result;
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  add_player(result);
  add_enemies(result);
  return result;
}

/**
 * Key handler
 * Edit only bodies of type "player"
 * Should make a player pellet on certain keys
 *
 * @param scene the current scene
 */
void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *player = scene_get_body(state->scene, 0);
  assert(body_get_info(player) == PLAYER);
  vector_t centroid = body_get_centroid(player);
  vector_t new_centroid = {.x = centroid.x, .y = centroid.y};
  if (centroid.x <= SDL_MIN.x + PLAYER_RADIUS) {
    new_centroid.x = SDL_MIN.x + PLAYER_RADIUS;
  }
  if (centroid.x >= SDL_MAX.x - PLAYER_RADIUS) {
    new_centroid.x = SDL_MAX.x - PLAYER_RADIUS;
  }
  body_set_centroid(player, new_centroid);
  if (type == KEY_PRESSED) {
    if (key == RIGHT_ARROW) {
      vector_t velocity = {.x = VELOCITY_MAGNITUDE, .y = 0};
      body_set_velocity(player, velocity);
    }
    if (key == LEFT_ARROW) {
      vector_t velocity = {.x = -VELOCITY_MAGNITUDE, .y = 0};
      body_set_velocity(player, velocity);
    }
    if (key == ' ') {
      scene_add_body(state->scene, spawn_player_bullet(state->scene));
    }
  } else {
    body_set_velocity(player, ZERO_VECTOR);
  }
}

void enemy_move_down(scene_t *scene) {
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) == ENEMY) {
      vector_t centroid = body_get_centroid(body);
      if (centroid.x < SDL_MIN.x + ENEMY_RADIUS ||
          centroid.x > SDL_MAX.x - ENEMY_RADIUS) {
        vector_t current_velocity = body_get_velocity(body);
        vector_t new_velocity = {.x = current_velocity.x * -1,
                                 .y = current_velocity.y};
        body_set_velocity(body, new_velocity);
        vector_t new_centroid = {.x = 0, .y = 0};
        if (centroid.x < SDL_MIN.x + ENEMY_RADIUS) {
          new_centroid.x = centroid.x + ENEMY_RADIUS;
          new_centroid.y = centroid.y - 3 * ENEMY_RADIUS;
        }
        if (centroid.x > SDL_MAX.x - ENEMY_RADIUS) {
          new_centroid.x = centroid.x - ENEMY_RADIUS;
          new_centroid.y = centroid.y - 3 * ENEMY_RADIUS;
        }
        body_set_centroid(body, new_centroid);
      }
    }
  }
}

bool is_game_over(scene_t *scene) {
  if (get_player(scene) == NULL) {
    return true;
  }
  bool does_not_have_enemies_left = true;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) == ENEMY) {
      if (body_get_centroid(body).y < ENEMY_RADIUS * 3) {
        return true;
      }
      does_not_have_enemies_left = false;
    }
  }
  return does_not_have_enemies_left;
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
    exit(0);
  }
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  enemy_move_down(scene);
  if (state->time_elapsed > TIME_BETWEEN_ENEMY_BULLETS) {
    scene_add_body(state->scene, spawn_enemy_bullet(state->scene));
    state->time_elapsed = 0;
  }
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