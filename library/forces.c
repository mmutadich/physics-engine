#include "forces.h"
#include "collision.h"
#include "math.h"
#include "scene.h"
#include "vector.h"
#include "body.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

const double AMPLITUDE = 300;
const double MAX_DISTANCE = 2000;
const double DISTANCE_0 = 5;
const double ELASTIC = 1;
const double INELASTIC = 0;
const double JUMP_IMPULSE = 10000;
const double TRAMPOLINE_IMPULSE = 20000;
const double ICE_VELOCITY_FACTOR = 0.7;
const double P_WIDTH = 50;
const vector_t SPAWN = {.x = 1000, .y = 790};

typedef enum {
  START_SCREEN = 0,
  GAME_SCREEN = 1,
  WIN_SCREEN = 2,
} screen_t;

typedef struct two_body_aux {
  body_t *body1;
  body_t *body2;
  double constant;
} two_body_aux_t;

typedef struct one_body_aux {
  body_t *body;
  double constant;
} one_body_aux_t;

typedef struct collision_aux {
  body_t *body1;
  body_t *body2;
  collision_handler_t handler;
  free_func_t aux_freer;
  bool is_colliding;
  void *aux;
  bool hold_colliding; //true when we want the force to be applied multiple times if is_collision is true
} collision_aux_t;

typedef struct bodies_collision_aux {
  list_t *bodies;
  bodies_collision_handler_t handler;
  free_func_t aux_freer;
  bool is_colliding;
  void *aux;
  bool hold_colliding; //true when we want the force to be applied multiple times if is_collision is true
} bodies_collision_aux_t;

void collision_aux_freer(void *collision_aux) {
  collision_aux_t *ca = (collision_aux_t *)collision_aux;
  assert(ca);
  if (ca->aux_freer != NULL) {
    ca->aux_freer(ca->aux);
  }
  free(ca);
}

void two_body_aux_freer(void *two_body_aux) {
  two_body_aux_t *tba = (two_body_aux_t *)two_body_aux;
  assert(tba);
  free(tba);
}

void one_body_aux_freer(void *one_body_aux) {
  one_body_aux_t *oba = (one_body_aux_t *)one_body_aux;
  assert(oba);
  free(oba);
}

two_body_aux_t *two_body_aux_init(body_t *body1, body_t *body2,
                                  double constant) {
  two_body_aux_t *result = malloc(sizeof(two_body_aux_t));
  assert(result);
  result->body1 = body1;
  result->body2 = body2;
  result->constant = constant;
  return result;
}

one_body_aux_t *one_body_aux_init(body_t *body, double constant) {
  one_body_aux_t *result = malloc(sizeof(one_body_aux_t));
  assert(result);
  result->body = body;
  result->constant = constant;
  return result;
}

collision_aux_t *collision_aux_init(body_t *body1, body_t *body2,
                                    collision_handler_t handler,
                                    free_func_t aux_freer, void *aux) {
  collision_aux_t *result = malloc(sizeof(collision_aux_t));
  assert(result);
  result->body1 = body1;
  result->body2 = body2;
  result->handler = handler;
  result->aux = aux;
  result->aux_freer = aux_freer;
  result->is_colliding = false;
  result->hold_colliding = false;
  return result;
}

vector_t get_distance(vector_t centroid1, vector_t centroid2) {
  vector_t answer = {.x = centroid1.x - centroid2.x,
                     .y = centroid1.y - centroid2.y};
  return answer;
}

void apply_newtonian_gravity(void *two_body_aux) {
  two_body_aux_t *aux = (two_body_aux_t *)two_body_aux;
  body_t *body1 = aux->body1;
  body_t *body2 = aux->body2;
  double G = aux->constant;
  vector_t centroid1 = body_get_centroid(body1);
  vector_t centroid2 = body_get_centroid(body2);
  vector_t distance = get_distance(centroid1, centroid2);
  double d = sqrt(distance.x * distance.x + distance.y * distance.y);
  double net_force = 0;
  if (d > DISTANCE_0) {
    net_force = -G * body_get_mass(body1) * body_get_mass(body2) / (pow(d, 2));
  } else {
    net_force =
        -G * body_get_mass(body1) * body_get_mass(body2) / (pow(DISTANCE_0, 2));
  }
  vector_t force = {.x = 0, .y = 0};
  force.x = net_force * distance.x / d;
  force.y = net_force * distance.y / d;
  body_set_force(body1, vec_add(body_get_force(body1), force));
  body_set_force(body2, vec_subtract(body_get_force(body2), force));
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
  two_body_aux_t *aux = two_body_aux_init(body1, body2, G);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, apply_newtonian_gravity, aux, bodies,
                                 two_body_aux_freer);
}

vector_t get_spring_force(body_t *body, body_t *anchor, double k) {
  vector_t distance =
      get_distance(body_get_centroid(body), body_get_centroid(anchor));
  vector_t force = {.x = -k * distance.x, .y = -k * distance.y};
  return force;
}

void apply_spring(void *two_body_aux) {
  two_body_aux_t *aux = (two_body_aux_t *)two_body_aux;
  body_t *body = aux->body1;
  body_t *anchor = aux->body2;
  double k = aux->constant;
  vector_t force = get_spring_force(body, anchor, k);
  vector_t body_force = body_get_force(body);
  body_set_force(body, vec_add(body_force, force));
  vector_t anchor_force = body_get_force(anchor);
  body_set_force(anchor, vec_subtract(anchor_force, force));
}

void create_spring(scene_t *scene, double k, body_t *body, body_t *anchor) {
  two_body_aux_t *aux = two_body_aux_init(body, anchor, k);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body);
  list_add(bodies, anchor);
  scene_add_bodies_force_creator(scene, apply_spring, aux, bodies,
                                 two_body_aux_freer);
}

void apply_drag(void *one_body_aux) {
  one_body_aux_t *aux = (one_body_aux_t *)one_body_aux;
  body_t *body = aux->body;
  double gamma = aux->constant;
  vector_t vel = body_get_velocity(body);
  vector_t force = {.x = -gamma * vel.x, .y = -gamma * vel.y};
  vector_t net_force = vec_add(body_get_force(body), force);
  body_set_force(body, net_force);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
  one_body_aux_t *aux = one_body_aux_init(body, gamma);
  list_t *bodies = list_init(1, NULL);
  list_add(bodies, body);
  scene_add_bodies_force_creator(scene, apply_drag, aux, bodies,
                                 one_body_aux_freer);
}

vector_t calculate_impulse(body_t *body1, body_t *body2, double elasticity,
                           vector_t collision_axis) {
  double m1 = body_get_mass(body1);
  double m2 = body_get_mass(body2);
  double u1 = vec_dot(body_get_velocity(body1), collision_axis);
  double u2 = vec_dot(body_get_velocity(body2), collision_axis);
  double impulse;
  if (m1 == INFINITY)
    impulse = m2 * (1 + elasticity) * (u2 - u1);
  if (m2 == INFINITY)
    impulse = m1 * (1 + elasticity) * (u2 - u1);
  else
    impulse = m1 * m2 / (m1 + m2) * (1 + elasticity) * (u2 - u1);
  vector_t vec = vec_normalize(collision_axis);
  return vec_multiply(impulse, vec);
}


vector_t get_impulse(double impulse) {
  //for jumping, we want there to always be a velocity up
  vector_t result = {.x = 0, .y = impulse};
  return result;
}

void apply_destructive_collision(void *two_body_aux) {
  two_body_aux_t *aux = (two_body_aux_t *)two_body_aux;
  list_t *shape1 = body_get_shape(aux->body1);
  list_t *shape2 = body_get_shape(aux->body2);
  if (collision_get_collided(find_collision(shape1, shape2))) {
    body_remove(aux->body1);
    body_remove(aux->body2);
  }
  list_free(shape1);
  list_free(shape2);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
  two_body_aux_t *aux = two_body_aux_init(body1, body2, 0);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, apply_destructive_collision, aux,
                                 bodies, two_body_aux_freer);
}

void apply_collision(void *c_aux) {
  collision_aux_t *collision_aux = (collision_aux_t *)c_aux;
  assert(collision_aux);
  list_t *shape1 = body_get_shape(collision_aux->body1);
  list_t *shape2 = body_get_shape(collision_aux->body2);
  assert(shape1);
  assert(shape2);
  if (collision_get_collided(find_collision(shape1, shape2)) && (!collision_aux->is_colliding || collision_aux->hold_colliding)) {
    vector_t collision_axis = collision_get_axis(find_collision(shape1, shape2));
    collision_aux->handler(collision_aux->body1, collision_aux->body2,
                           collision_axis, collision_aux->aux);
    collision_aux->is_colliding = true;
  }
  if (!collision_get_collided(find_collision(shape1, shape2))) {
    collision_aux->is_colliding = false;
  }
  list_free(shape1);
  list_free(shape2);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  collision_aux_t *collision_aux =
      collision_aux_init(body1, body2, handler, freer, aux);
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, apply_collision, collision_aux, bodies,
                                 collision_aux_freer);
}

void jump_collision_handler(body_t *ball, body_t *target, vector_t axis,
                               void *aux) {
  two_body_aux_t *tba = (two_body_aux_t *)aux;
  assert(ball);
  assert(target);
  //TODO GET RID OF MAGIC NUMBERS
  vector_t impulse_vector = get_impulse(JUMP_IMPULSE); 
  body_add_impulse(ball, impulse_vector);
  vector_t opposite_impulse_vector = vec_multiply(-1, impulse_vector);
  body_add_impulse(target, opposite_impulse_vector);
}

//TODO: NEED TO CHANGE THIS BACK TO THE OLD VERSION
void physics_collision_handler(body_t *ball, body_t *target, vector_t axis,
                               void *aux) {
  two_body_aux_t *tba = (two_body_aux_t *)aux;
  assert(ball);
  assert(target);
  //something is wrong with tba->constant?
  //need to change tba constant back
  vector_t impulse_vector =
      calculate_impulse(ball, target, .5, axis);
  body_add_impulse(ball, impulse_vector);
  vector_t opposite_impulse_vector = vec_multiply(-1, impulse_vector);
  body_add_impulse(target, opposite_impulse_vector);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
  two_body_aux_t *aux = two_body_aux_init(body1, body2, elasticity);
  create_collision(scene, body1, body2, physics_collision_handler, aux,
                   two_body_aux_freer);
}

void one_sided_destructive_collision_handler(body_t *body1,
                                             body_t *body_to_destruct,
                                             vector_t axis, void *aux) {
  body_remove(body_to_destruct);
}

void create_one_sided_destructive_collision(scene_t *scene, body_t *body1,
                                            body_t *body_to_destruct) {
  two_body_aux_t *aux = two_body_aux_init(body1, body_to_destruct, 0);
  create_collision(scene, body1, body_to_destruct,
                   one_sided_destructive_collision_handler, aux,
                   two_body_aux_freer);
}

void jump_up(scene_t *scene, body_t *body1, body_t *body2, double elasticity) {
  two_body_aux_t *aux = two_body_aux_init(body1, body2, elasticity);
  collision_aux_t *collision_aux = collision_aux_init(body1, body2, 
                          jump_collision_handler, aux, two_body_aux_freer);
  apply_collision(collision_aux);
}

void apply_universal_gravity(void *aux) {
  one_body_aux_t *oba = (one_body_aux_t*)aux;
  vector_t gravity = {.x = 0, .y = oba->constant};
  body_add_force(oba->body, gravity);
}

void create_universal_gravity(scene_t *scene, body_t *body, double gravity) {
  one_body_aux_t *aux = one_body_aux_init(body, gravity);
  list_t *bodies = list_init(1, NULL);
  list_add(bodies, body);
  scene_add_bodies_force_creator(scene, apply_universal_gravity, aux,
                                 bodies, one_body_aux_freer);
}

void normal_force_collision_handler(body_t *body, body_t *ledge, vector_t axis, void *aux) {
  two_body_aux_t *tba = (two_body_aux_t *)aux;
  assert(body);
  assert(ledge);
  vector_t gravity = {.x = 0, .y = tba->constant};
  body_add_force(body, vec_multiply(-1, gravity));
  body_add_force(ledge, gravity);
  if (axis.y == 0){
    vector_t new_velocity = {.x = 0, .y = body_get_velocity(body).y};
    body_set_velocity(body, new_velocity);
  } else if (axis.x == 0){
    vector_t new_velocity = {.x = body_get_velocity(body).x, .y = 0};
    body_set_velocity(body, new_velocity);
  }
}

void create_collision_hold_on(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
  collision_aux_t *collision_aux =
      collision_aux_init(body1, body2, handler, freer, aux);
  collision_aux->hold_colliding = true;
  list_t *bodies = list_init(2, NULL);
  list_add(bodies, body1);
  list_add(bodies, body2);
  scene_add_bodies_force_creator(scene, apply_collision, collision_aux, bodies,
                                 collision_aux_freer);
}

void create_normal_force(scene_t *scene, body_t *body, body_t *ledge, double gravity) {
  two_body_aux_t *aux = two_body_aux_init(body, ledge, gravity);
  create_collision_hold_on(scene, body, ledge,
                   normal_force_collision_handler, aux,
                   two_body_aux_freer);
}

void game_over_collision_handler(body_t *ball, body_t *target, vector_t axis, void *aux) {
  //we know the aux is a scene
  scene_t *scene = (scene_t*)aux;
  assert(scene);
  scene_set_game_over(scene, true);
}

void create_game_over_force(scene_t *scene, body_t *player, body_t *body) {
  create_collision(scene, player, body,
                   game_over_collision_handler, scene,
                   two_body_aux_freer);
  //the aux is the scene because we need to access this in the game_over_handler
}

void plant_boy_fertilizer_collision_handler(body_t *ball, body_t *target, vector_t axis, void *aux) {
  scene_t *scene = (scene_t*)aux;
  assert(scene);
  vector_t new_centroid = {.x = 70, .y = 910};
  body_set_centroid(target, new_centroid);
  scene_set_plant_boy_fertilizer_collected(scene, true);
}

void create_plant_boy_fertilizer_force(scene_t *scene, body_t *player, body_t *body) {
  create_collision(scene, player, body,
                   plant_boy_fertilizer_collision_handler, scene,
                   two_body_aux_freer);
}

void dirt_girl_fertilizer_collision_handler(body_t *ball, body_t *target, vector_t axis, void *aux) {
  scene_t *scene = (scene_t*)aux;
  assert(scene);
  vector_t new_centroid = {.x = 160, .y = 920};
  body_set_centroid(target, new_centroid);
  scene_set_dirt_girl_fertilizer_collected(scene, true);
}

void create_dirt_girl_fertilizer_force(scene_t *scene, body_t *player, body_t *body) {
  create_collision(scene, player, body,
                   dirt_girl_fertilizer_collision_handler, scene,
                   two_body_aux_freer);
  //in the collision handler, we want to move the fertilizer to the top left corner
}

void boundary_collision_handler(body_t *sprite, body_t *boundary, vector_t axis, void *aux) {
  vector_t centroid_sprite = body_get_centroid(sprite);
  vector_t new_centroid_sprite = {.x = centroid_sprite.x, .y = centroid_sprite.y};
  vector_t *dimensions = (vector_t *)aux;
  double cx = dimensions->x;
  double b_dimensions = dimensions->y;
  vector_t boundary_centroid = body_get_centroid(boundary);
  if (centroid_sprite.x > boundary_centroid.x && centroid_sprite.x <= boundary_centroid.x + cx/2 + b_dimensions/2) {
    printf("right boundary\n");
    new_centroid_sprite.x = boundary_centroid.x + cx + b_dimensions/2;
  }
  if (centroid_sprite.x < boundary_centroid.x && centroid_sprite.x >= boundary_centroid.x - cx/2 - b_dimensions/2) {
    printf("left boundary\n");
    new_centroid_sprite.x = boundary_centroid.x - cx - b_dimensions/2;
  }
  body_set_centroid(sprite, new_centroid_sprite);
}

void create_boundary_force(scene_t *scene, body_t *sprite, body_t *boundary, vector_t *character_dimensions) {
  create_collision(scene, sprite, boundary,
                  boundary_collision_handler, character_dimensions, 
                  two_body_aux_freer);
}
void portal_collision_handler(body_t *sprite, body_t *entry_portal, vector_t axis,
                               void *aux) {
  assert(sprite);
  assert(entry_portal);
  vector_t current_velocity = body_get_velocity(sprite);
  vector_t centroid_sprite = body_get_centroid(sprite);
  vector_t portal_centroid = body_get_centroid(entry_portal);
  if ( centroid_sprite.x > portal_centroid.x){
    vector_t new_centroid = {.x = (SPAWN.x - P_WIDTH), .y = SPAWN.y};
    body_set_centroid(sprite, new_centroid);
  } else {
    vector_t new_centroid = {.x = (SPAWN.x + P_WIDTH), .y = SPAWN.y};
    body_set_centroid(sprite, new_centroid);
  }
  vector_t reverse_x = vec_multiply(-1, current_velocity);
  vector_t new_velocity = {.x = reverse_x.x, .y = current_velocity.y};
  body_set_velocity(sprite, new_velocity);
  //if colliding, what happens?
}

void create_portal_force(scene_t *scene, body_t *sprite, body_t *entry_portal, body_t *exit_portal, double elasticity) {
  //two_body_aux_t *aux = two_body_aux_init(entry_portal, exit_portal, elasticity);
  create_collision(scene, sprite, entry_portal,
                   portal_collision_handler, scene,
                   two_body_aux_freer);
  
}

void trampoline_collision_handler(body_t *sprite, body_t *trampoline, vector_t axis,
                               void *aux) {
  assert(sprite);
  assert(trampoline);
  vector_t impulse_vector = get_impulse(TRAMPOLINE_IMPULSE); 
  body_add_impulse(sprite, impulse_vector);
}

void create_trampoline_force(scene_t *scene, body_t *sprite, body_t *trampoline, double elasticity) {
  create_collision(scene, sprite, trampoline,
                   trampoline_collision_handler, scene,
                   two_body_aux_freer);
}

void ice_collision_handler(body_t *sprite, body_t *ice, vector_t axis,
                               void *aux) {
  assert(sprite);
  assert(ice);
  vector_t new_velocity = vec_multiply(ICE_VELOCITY_FACTOR, body_get_velocity(sprite));
  body_set_velocity(sprite, new_velocity);
  //body_decrease_velocity(sprite, ICE_VELOCITY_FACTOR);
}

void create_ice_force(scene_t *scene, body_t *sprite, body_t *ice, double elasticity) {
  create_collision_hold_on(scene, sprite, ice, ice_collision_handler, scene, two_body_aux_freer);
}





//start bodies collision aux

bodies_collision_aux_t *bodies_collision_aux_init(list_t *bodies,
                                    bodies_collision_handler_t handler,
                                    free_func_t aux_freer, void *aux) {
  bodies_collision_aux_t *result = malloc(sizeof(collision_aux_t));
  result->bodies = bodies;
  assert(result->bodies);
  result->handler = handler;
  result->aux = aux;
  result->aux_freer = aux_freer;
  result->is_colliding = false;
  result->hold_colliding = false;
  assert(result);
  return result;
}

void apply_collision_multiple(void *c_aux) {
  //the error is not within apply_collision
  bodies_collision_aux_t *collision_aux = (bodies_collision_aux_t *)c_aux;
  assert(collision_aux);
  bool keep_track = true;
  for (size_t i = 0; i < list_size(collision_aux->bodies) - 1; i+=2) {
    body_t *body1 = list_get(collision_aux->bodies, i);
    body_t *body2 = list_get(collision_aux->bodies, i + 1);
    list_t *shape1 = body_get_shape(body1);
    list_t *shape2 = body_get_shape(body2);
    assert(shape1);
    assert(shape2);
    if (collision_get_collided(find_collision(shape1, shape2)) && (!collision_aux->is_colliding || collision_aux->hold_colliding)) {
      vector_t collision_axis = collision_get_axis(find_collision(shape1, shape2));
    }
    if (!collision_get_collided(find_collision(shape1, shape2))) {
      keep_track = false;
    }
    list_free(shape1);
    list_free(shape2);
  }
  collision_aux->is_colliding = keep_track;
  if (collision_aux->is_colliding) {
    collision_aux->handler(collision_aux->bodies, collision_aux->aux);
  }
}

void bodies_collision_aux_freer(void *collision_aux) {
  bodies_collision_aux_t *ca = (bodies_collision_aux_t *)collision_aux;
  assert(ca);
  if (ca->aux_freer != NULL) {
    ca->aux_freer(ca->aux);
  }
  //SHOULD NOT FREE THE BODIES
  free(ca);
}

void create_collision_multiple(scene_t *scene, list_t *bodies,
                      bodies_collision_handler_t handler, void *aux,
                      free_func_t aux_freer) {
  //need to make a hard copy of bodies, but have the freer be null
  list_t *bodies_null_freer = list_init(10, NULL);
  for (size_t i = 0; i < list_size(bodies); i++) {
    body_t *body = list_get(bodies, i);
    list_add(bodies_null_freer, body);
  }
  
  bodies_collision_aux_t *collision_aux =
      bodies_collision_aux_init(bodies_null_freer, handler, aux_freer, aux);
  assert(list_size(bodies_null_freer) > 0);
  scene_add_bodies_force_creator(scene, apply_collision_multiple, collision_aux, bodies_null_freer,
                                 bodies_collision_aux_freer);
}

void win_handler(list_t *bodies, void *aux) {
  scene_t *scene = (scene_t*)aux;
  assert(scene);
  scene_set_screen(scene, WIN_SCREEN); //this is the win_screen
  printf("won the game\n");
}

void guarantee_all_collisions(scene_t *scene, list_t *bodies) {
  create_collision_multiple(scene, bodies, win_handler, scene, NULL);
}

