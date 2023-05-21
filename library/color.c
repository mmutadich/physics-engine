#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  float r;
  float g;
  float b;
} rgb_color_t;

rgb_color_t *color_init(float red, float green, float blue) {
  assert(0 < red < 1);
  assert(0 < green < 1);
  assert(0 < blue < 1);
  rgb_color_t *color = malloc(sizeof(rgb_color_t));
  assert(color);
  color->r = red;
  color->g = green;
  color->b = blue;
  return color;
}

bool color_equal(rgb_color_t color1, rgb_color_t color2) {
  return ((color1.r == color2.r) && (color1.g == color2.g) &&
          (color1.b == color2.b));
}