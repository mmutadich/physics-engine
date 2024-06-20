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

const size_t NUM_PADDLE_BRICK_POINTS = 4;
const double PADDLE_BRICK_WIDTH = 190;
const double PADDLE_BRICK_HEIGHT = 50;

const vector_t INITIAL_PADDLE_POSITION = {.x = 1000, .y = 100};
const rgb_color_t PADDLE_BALL_COLOR = {0.75, 0.75, 1};
const double PADDLE_VELOCITY = 300;

const size_t NUM_BRICKS = 30;
const size_t NUM_ROWS = 3;

const size_t NUM_BALL_POINTS = 8;
const double BALL_RADIUS = 10;
const double BALL_MASS = 10;
const vector_t INITIAL_BALL_VELOCITY = {.x = -100, 200};

const rgb_color_t WALL_COLOR = {0, 0, 0};
const double WALL_WIDTH = 10;

const double ELASTICITY = 1;                      // perfectly elastic
const double INFINITY_MASS = INFINITY;            // brick, paddle, wall
const double TIME_BETWEEN_BALL_COLOR_CHANGES = 5; // special feature
const rgb_color_t BALL_COLOR_2 = {1, 0.75, 0.75};

typedef enum {
  PADDLE = 1,
  BRICK = 2,
  BALL = 3,
  WALL = 4,
} info_t;

typedef struct state {
  scene_t *scene;
  double time_elapsed;
} state_t;

list_t *make_paddle_brick_points(vector_t centroid) {
  list_t *points = list_init(NUM_PADDLE_BRICK_POINTS, free);
  // 0: top left, 1: bottom left, 2: bottom right, 3: top right
  // (counter-clockwise)
  for (size_t i = 0; i < NUM_PADDLE_BRICK_POINTS; i++) {
    vector_t *point = malloc(sizeof(vector_t));
    assert(point);
    if (i == 0 || i == 1)
      point->x = centroid.x - PADDLE_BRICK_WIDTH / 2;
    else
      point->x = centroid.x + PADDLE_BRICK_WIDTH / 2;
    if (i == 0 || i == 3)
      point->y = centroid.y + PADDLE_BRICK_HEIGHT / 2;
    else
      point->y = centroid.y - PADDLE_BRICK_HEIGHT / 2;
    list_add(points, point);
  }
  return points;
}

void add_paddle(scene_t *scene) {
  list_t *shape = make_paddle_brick_points(INITIAL_PADDLE_POSITION);
  body_t *paddle = body_init_with_info(shape, INFINITY_MASS, PADDLE_BALL_COLOR,
                                       PADDLE, NULL);
  body_set_centroid(paddle, INITIAL_PADDLE_POSITION);
  scene_add_body(scene, paddle);
}

rgb_color_t get_brick_color(size_t i) {
  double r = 0;
  double g = 0;
  double b = 0;
  double color_vals[3] = {0, 127.0 / 255, 255.0 / 255};
  i = i % (NUM_BRICKS / NUM_ROWS);
  if (i == 2)
    r = color_vals[1];
  if (i == 1 || i == 6)
    g = color_vals[1];
  if (i == 4 || i == 9)
    b = color_vals[1];
  if (i == 0 || i == 1 || i == 8 || i == 9)
    r = color_vals[2];
  if (i >= 2 && i <= 5)
    g = color_vals[2];
  if (i == 5 || i == 6 || i == 7 || i == 8)
    b = color_vals[2];
  rgb_color_t result = {r, g, b};
  return result;
}

void add_bricks(scene_t *scene) {
  for (size_t i = 0; i < NUM_BRICKS; i++) {
    double x =
        SDL_MIN.x + (PADDLE_BRICK_WIDTH / 2) +
        (SDL_MAX.x / (NUM_BRICKS / NUM_ROWS) * (i % (NUM_BRICKS / NUM_ROWS)));
    double y = 0;
    if (i < NUM_BRICKS / NUM_ROWS) // first row
      y = SDL_MAX.y - 1 * PADDLE_BRICK_HEIGHT / 1.5;
    else if (i < 2 * NUM_BRICKS / NUM_ROWS) // second row
      y = SDL_MAX.y - 1.8 * PADDLE_BRICK_HEIGHT;
    else // third row
      y = SDL_MAX.y - 3 * PADDLE_BRICK_HEIGHT;
    vector_t centroid = {.x = x, .y = y};
    list_t *shape = make_paddle_brick_points(centroid);
    rgb_color_t color = get_brick_color(i);
    body_t *body =
        body_init_with_info(shape, INFINITY_MASS, color, BRICK, NULL);
    scene_add_body(scene, body);
  }
}

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

vector_t get_initial_ball_position() {
  vector_t difference = {.x = BALL_RADIUS + PADDLE_BRICK_HEIGHT / 2,
                         .y = BALL_RADIUS + PADDLE_BRICK_HEIGHT / 2};
  vector_t ball_position = vec_add(INITIAL_PADDLE_POSITION, difference);
  return ball_position;
}

void add_ball(scene_t *scene) {
  list_t *points = make_ball_points(get_initial_ball_position());
  body_t *ball =
      body_init_with_info(points, BALL_MASS, PADDLE_BALL_COLOR, BALL, NULL);
  body_set_centroid(ball, get_initial_ball_position());
  body_set_velocity(ball, INITIAL_BALL_VELOCITY);

  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene, i)) == BRICK) {
      create_physics_collision(scene, ELASTICITY, ball,
                               scene_get_body(scene, i));
      create_one_sided_destructive_collision(scene, ball,
                                             scene_get_body(scene, i));
    }
    if (body_get_info(scene_get_body(scene, i)) == PADDLE) {
      create_physics_collision(scene, ELASTICITY, ball,
                               scene_get_body(scene, i));
    }
    if (body_get_info(scene_get_body(scene, i)) == WALL) {
      create_physics_collision(scene, ELASTICITY, ball,
                               scene_get_body(scene, i));
    }
  }

  scene_add_body(scene, ball);
}

list_t *make_wall_points(char wall) {
  list_t *wall_points = list_init(NUM_PADDLE_BRICK_POINTS, free);
  for (size_t i = 0; i < NUM_PADDLE_BRICK_POINTS;
       i++) {        // top left, bottom left, bottom right, top right
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
  }
  return wall_points;
}

void add_walls(scene_t *scene) {
  body_t *left_wall = body_init_with_info(make_wall_points(0), INFINITY_MASS,
                                          WALL_COLOR, WALL, NULL);
  body_t *top_wall = body_init_with_info(make_wall_points(1), INFINITY_MASS,
                                         WALL_COLOR, WALL, NULL);
  body_t *right_wall = body_init_with_info(make_wall_points(2), INFINITY_MASS,
                                           WALL_COLOR, WALL, NULL);
  scene_add_body(scene, left_wall);
  scene_add_body(scene, top_wall);
  scene_add_body(scene, right_wall);
}

scene_t *make_initial_scene() {
  scene_t *result = scene_init();
  add_paddle(result);
  add_bricks(result);
  add_walls(result);
  add_ball(result);
  return result;
}

void keyer(char key, key_event_type_t type, double held_time, state_t *state) {
  body_t *paddle = scene_get_body(state->scene, 0);
  assert(body_get_info(paddle) == PADDLE);
  vector_t centroid = body_get_centroid(paddle);
  vector_t new_centroid = {.x = centroid.x, .y = centroid.y};
  if (centroid.x <= SDL_MIN.x + PADDLE_BRICK_WIDTH / 2) {
    new_centroid.x = SDL_MIN.x + PADDLE_BRICK_WIDTH / 2;
  }
  if (centroid.x >= SDL_MAX.x - PADDLE_BRICK_WIDTH / 2) {
    new_centroid.x = SDL_MAX.x - PADDLE_BRICK_WIDTH / 2;
  }
  body_set_centroid(paddle, new_centroid);
  if (type == KEY_PRESSED) {
    if (key == RIGHT_ARROW) {
      vector_t velocity = {.x = PADDLE_VELOCITY, .y = 0};
      body_set_velocity(paddle, velocity);
    }
    if (key == LEFT_ARROW) {
      vector_t velocity = {.x = -PADDLE_VELOCITY, .y = 0};
      body_set_velocity(paddle, velocity);
    }
  } else {
    body_set_velocity(paddle, ZERO_VECTOR);
  }
}

bool is_game_over(scene_t *scene) {
  bool does_not_have_bricks_left = true;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *body = scene_get_body(scene, i);
    if (body_get_info(body) == BALL) {
      vector_t centroid = body_get_centroid(body);
      if (centroid.y < INITIAL_PADDLE_POSITION.y)
        return true;
    }
    if (body_get_info(body) == BRICK) {
      does_not_have_bricks_left = false;
    }
  }
  return does_not_have_bricks_left;
}

void reset_game(scene_t *scene) {
  // remove all bricks
  body_t *ball;
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    body_t *curr_body = scene_get_body(scene, i);
    if (body_get_info(curr_body) == BRICK)
      body_remove(curr_body);
    if (body_get_info(curr_body) == PADDLE)
      body_set_centroid(curr_body, INITIAL_PADDLE_POSITION);
    if (body_get_info(curr_body) == BALL) {
      body_set_centroid(curr_body, get_initial_ball_position());
      body_reset(curr_body);
      body_set_velocity(curr_body, INITIAL_BALL_VELOCITY);
      ball = curr_body;
    }
  }
  // redraw all bricks
  add_bricks(scene);
  // re-add force creators to all the bricks
  for (size_t i = 0; i < scene_bodies(scene); i++) {
    if (body_get_info(scene_get_body(scene, i)) == BRICK) {
      create_one_sided_destructive_collision(scene, ball,
                                             scene_get_body(scene, i));
      create_physics_collision(scene, ELASTICITY, ball,
                               scene_get_body(scene, i));
    }
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
  scene_t *scene = state->scene;
  if (is_game_over(scene)) {
    reset_game(scene);
  }
  double dt = time_since_last_tick();
  state->time_elapsed += dt;
  if (state->time_elapsed > TIME_BETWEEN_BALL_COLOR_CHANGES) {
    for (size_t i = 0; i < scene_bodies(scene); i++) {
      body_t *body = scene_get_body(scene, i);
      if (body_get_info(body) == BALL) {
        if (color_equal(body_get_color(body), BALL_COLOR_2))
          body_set_color(body, PADDLE_BALL_COLOR);
        else
          body_set_color(body, BALL_COLOR_2);
      }
    }
    state->time_elapsed = 0;
  }
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
