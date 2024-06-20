#include "polygon.h"
#include "sdl_wrapper.h"
#include "state.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

const size_t NPOINTS = 5;
const double CENTER_X = 500;
const double CENTER_Y = 500;
const rgb_color_t color = {1, 0.75, 0.75};

typedef struct state {
  list_t *polygon;
  vector_t *velocity;
} state_t;

/**
 * Initializes sdl as well as the variables needed
 * Creates and stores all necessary variables for the demo in a created state
 * variable Returns the pointer to this state (This is the state emscripten_main
 * and emscripten_free work with)
 */
state_t *emscripten_init() {
  vector_t sdl_min = {0.0, 0.0};
  vector_t sdl_max = {1000.0, 1000.0};
  sdl_init(sdl_min, sdl_max);

  state_t *result = malloc(sizeof(state_t));
  assert(result);
  result->polygon = make_initial_star((size_t)5);
  result->velocity = malloc(sizeof(vector_t));
  assert(result->velocity);
  result->velocity->x = 600;
  result->velocity->y = 800;

  return result;
}

void draw_star(list_t *polygon, vector_t velocity, double dt) {
  polygon_translate(polygon, (vector_t){velocity.x * dt, velocity.y * dt});
  polygon_rotate(polygon, .05, polygon_centroid(polygon));
}

void update_velocity(vector_t *velocity, list_t *polygon) {
  for (size_t i = 0; i < list_size(polygon); i++) {
    vector_t *point = list_get(polygon, i);
    if (point->x < 5 || point->x > 995)
      velocity->x = -1 * velocity->x;
    if (point->y < 5 || point->y > 995)
      velocity->y = -1 * velocity->y;
  }
}

/**
 * Called on each tick of the program
 * Updates the state variables and display as necessary, depending on the time
 * that has passed
 */
void emscripten_main(state_t *state) {
  sdl_clear();
  double dt = time_since_last_tick();

  update_velocity(state->velocity, state->polygon);
  draw_star(state->polygon, *state->velocity, dt);
  sdl_draw_polygon(state->polygon, color);
  sdl_show();
}

/**
 * Frees anything allocated in the demo
 * Should free everything in state as well as state itself.
 */
void emscripten_free(state_t *state) {
  list_t *polygon = state->polygon;
  list_free(polygon);
  free(state);
}
