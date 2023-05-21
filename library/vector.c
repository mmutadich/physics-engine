#include "vector.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const vector_t VEC_ZERO = {0.0, 0.0};

int vec_eq(vector_t v1, vector_t v2) { return (v1.x == v2.x && v1.y == v2.y); }

vector_t vec_add(vector_t v1, vector_t v2) {
  vector_t result;
  result.x = v1.x + v2.x;
  result.y = v1.y + v2.y;
  return result;
}

vector_t vec_normalize(vector_t vec) {
  double magnitude = sqrt(pow(vec.x, 2) + pow(vec.y, 2));
  vector_t result = {.x = vec.x / magnitude, .y = vec.y / magnitude};
  return result;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
  vector_t result;
  result.x = v1.x - v2.x;
  result.y = v1.y - v2.y;
  return result;
}

vector_t vec_negate(vector_t v) {
  vector_t result;
  result.x = v.x * -1;
  result.y = v.y * -1;
  return result;
}

vector_t vec_multiply(double scalar, vector_t v) {
  vector_t result;
  result.x = v.x * scalar;
  result.y = v.y * scalar;
  return result;
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
  vector_t result;
  result.x = v.x * cos(angle) - v.y * sin(angle);
  result.y = v.y * cos(angle) + v.x * sin(angle);
  return result;
}
