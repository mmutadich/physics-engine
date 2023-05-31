#include "scene.h"
#include "body.h"
#include "forces.h"
#include "list.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

const size_t NUM_BODIES = 10;
const size_t NUM_FORCES = 30;

typedef struct scene {
  list_t *bodies;
  // add list of players here?
  list_t *forces;
  bool game_over; //true when the game is over
} scene_t;

typedef struct force {
  force_creator_t force_creator;
  void *aux;
  free_func_t aux_freer;
  list_t *bodies;
} force_t;

force_t *force_init(force_creator_t force_creator, void *aux,
                    free_func_t aux_freer) {
  force_t *result = malloc(sizeof(force_t));
  assert(result);
  result->force_creator = force_creator;
  result->aux = aux;
  result->aux_freer = aux_freer;
  result->bodies = NULL;
  return result;
}

force_t *force_bodies_init(force_creator_t force_creator, void *aux,
                           free_func_t aux_freer, list_t *bodies) {
  force_t *result = malloc(sizeof(force_t));
  assert(result);
  result->force_creator = force_creator;
  result->aux = aux;
  result->aux_freer = aux_freer;
  result->bodies = bodies;
  return result;
}

void force_free(force_t *force) {
  if (force->aux_freer != NULL)
    force->aux_freer(force->aux);
  if (force->bodies != NULL)
    list_free(force->bodies);
  free(force);
}

/**
 * Allocates memory for an empty scene.
 * Makes a reasonable guess of the number of bodies to allocate space for.
 * Asserts that the required memory is successfully allocated.
 *
 * @return the new scene
 */
scene_t *scene_init(void) {
  scene_t *result = malloc(sizeof(scene_t));
  result->bodies = list_init(NUM_BODIES, body_free);
  result->forces = list_init(NUM_FORCES, force_free);
  result->game_over = false;
  assert(result);
  return result;
}

/**
 * Releases memory allocated for a given scene and all its bodies.
 *
 * @param scene a pointer to a scene returned from scene_init()
 */
void scene_free(scene_t *scene) {
  list_free(scene->bodies);
  list_free(scene->forces);
  free(scene);
}

/**
 * Gets the number of bodies in a given scene.
 *
 * @param scene a pointer to a scene returned from scene_init()
 * @return the number of bodies added with scene_add_body()
 */
size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

/**
 * Gets the body at a given index in a scene.
 * Asserts that the index is valid.
 *
 * @param scene a pointer to a scene returned from scene_init()
 * @param index the index of the body in the scene (starting at 0)
 * @return a pointer to the body at the given index
 */
body_t *scene_get_body(scene_t *scene, size_t index) {
  assert(index >= 0 && index < list_size(scene->bodies));
  return list_get(scene->bodies, index);
}

/**
 * Adds a body to a scene.
 *
 * @param scene a pointer to a scene returned from scene_init()
 * @param body a pointer to the body to add to the scene
 */
void scene_add_body(scene_t *scene, body_t *body) {
  list_add(scene->bodies, body);
}

/**
 * Removes and frees the body at a given index from a scene.
 * Asserts that the index is valid.
 *
 * @param scene a pointer to a scene returned from scene_init()
 * @param index the index of the body in the scene (starting at 0)
 */
void scene_remove_body(scene_t *scene, size_t index) {
  assert(index >= 0 && index < list_size(scene->bodies));
  body_remove(list_get(scene->bodies, index));
}

/**
 * Executes a tick of a given scene over a small time interval.
 * This requires ticking each body in the scene.
 *
 * @param scene a pointer to a scene returned from scene_init()
 * @param dt the time elapsed since the last tick, in seconds
 */
void scene_tick(scene_t *scene, double dt) {
  for (size_t i = 0; i < list_size(scene->forces); i++) {
    force_t *force = list_get(scene->forces, i);
    force_creator_t apply_force = force->force_creator;
    if (force->aux != NULL) {
      apply_force(force->aux);
    }
  }

  for (size_t j = 0; j < list_size(scene->bodies); j++) {
    body_t *body = list_get(scene->bodies, j);
    body_tick(body, dt);
    if (body_is_removed(body)) {
      for (size_t k = 0; k < list_size(scene->forces); k++) {
        force_t *force = list_get(scene->forces, k);
        list_t *bodies = force->bodies;
        if (list_contains(bodies, body)) {
          if (force != NULL) {
            force_free(list_remove(scene->forces, k));
            k--;
          }
        }
      }
      list_remove(scene->bodies, j);
      j--;
      if (body_is_player(body) == false)
        exit(0);
      body_free(body);
    }
  }
}

void scene_add_force_creator(scene_t *scene, force_creator_t force_creator,
                             void *aux, free_func_t freer) {
  list_t *bodies = list_init(10, free);
  scene_add_bodies_force_creator(scene, force_creator, aux, bodies, freer);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
  force_t *force = force_bodies_init(forcer, aux, freer, bodies);
  list_add(scene->forces, force);
}

size_t scene_forces(scene_t *scene) { return list_size(scene->forces); }

void scene_set_game_over(scene_t *scene, bool value) {
  scene->game_over = value;
}

bool scene_get_game_over(scene_t *scene) {
  return scene->game_over;
}