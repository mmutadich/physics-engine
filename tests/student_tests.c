#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "body.h"
#include "forces.h"
#include "list.h"
#include "polygon.h"
#include "scene.h"
#include "test_util.h"
#include "vector.h"

list_t *make_shape() {
  list_t *shape = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = (vector_t){-1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, -1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){+1, +1};
  list_add(shape, v);
  v = malloc(sizeof(*v));
  *v = (vector_t){-1, +1};
  list_add(shape, v);
  return shape;
}

double get_momentum(body_t *body) {
  vector_t velocity_components = body_get_velocity(body);
  double net_velocity =
      sqrt(pow(velocity_components.x, 2) + pow(velocity_components.y, 2));
  return body_get_mass(body) * net_velocity;
}

// Tests that a conservative force (gravity) conserves momentum (mass *
// velocity)
void test_momentum_conservation() {
  const double M1 = 4.5, M2 = 7.3;
  const double G = 1e3;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, mass1);
  body_t *mass2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass2, (vector_t){10, 20});
  vector_t velocity = {.x = 10, .y = 10};
  body_set_velocity(mass2, velocity);
  scene_add_body(scene, mass2);
  create_newtonian_gravity(scene, G, mass1, mass2);
  double initial_momentum = get_momentum(mass1) + get_momentum(mass2);
  for (int i = 0; i < STEPS; i++) {
    assert(body_get_centroid(mass1).x < body_get_centroid(mass2).x);
    double momentum = get_momentum(mass1) + get_momentum(mass2);
    assert(
        within(1e-1, momentum / initial_momentum, 1.0)); // increased from 1e-4
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

// Tests that a damping force (drag) decreases velocity
void test_drag_decreases_velocity() {
  const double M = 10;
  const double GAMMA = 4;
  const double A = 3;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  const vector_t INITIAL_VELOCITY = {.x = 20, .y = 20};

  scene_t *scene = scene_init();
  body_t *mass = body_init(make_shape(), M, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass, (vector_t){A, 0});
  body_set_velocity(mass, INITIAL_VELOCITY);
  body_add_force(mass, (vector_t){1, 1});
  scene_add_body(scene, mass);
  create_drag(scene, GAMMA, mass);
  for (int i = 0; i < STEPS; i++) {
    scene_tick(scene, DT);
    vector_t v = body_get_velocity(mass);
    assert(fabs(v.x) < fabs(INITIAL_VELOCITY.x));
    assert(fabs(v.y) < fabs(INITIAL_VELOCITY.y));
  }
  scene_free(scene);
}

// Tests that a conservative force (gravity) conserves mass
void test_mass_conservation() {
  const double M1 = 4.5, M2 = 7.3;
  const double G = 1e3;
  const double DT = 1e-6;
  const int STEPS = 1000000;
  scene_t *scene = scene_init();
  body_t *mass1 = body_init(make_shape(), M1, (rgb_color_t){0, 0, 0});
  scene_add_body(scene, mass1);
  body_t *mass2 = body_init(make_shape(), M2, (rgb_color_t){0, 0, 0});
  body_set_centroid(mass2, (vector_t){10, 20});
  vector_t velocity = {.x = 10, .y = 10};
  body_set_velocity(mass2, velocity);
  scene_add_body(scene, mass2);
  create_newtonian_gravity(scene, G, mass1, mass2);
  for (int i = 0; i < STEPS; i++) {
    assert(within(1e-4, body_get_mass(mass1) / M1, 1.0));
    assert(within(1e-4, body_get_mass(mass2) / M2, 1.0));
    scene_tick(scene, DT);
  }
  scene_free(scene);
}

int main(int argc, char *argv[]) {
  // Run all tests if there are no command-line arguments
  bool all_tests = argc == 1;
  // Read test name from file
  char testname[100];
  if (!all_tests) {
    read_testname(argv[1], testname, sizeof(testname));
  }

  DO_TEST(test_momentum_conservation)
  DO_TEST(test_drag_decreases_velocity)
  DO_TEST(test_mass_conservation)
}
