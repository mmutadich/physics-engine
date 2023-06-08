#include "sdl_wrapper.h"
#include "body.h"
#include "scene.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h> 

const char WINDOW_TITLE[] = "CS 3";
const int WINDOW_WIDTH = 1000;
const int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

//BACKGROUND
const char *BG = "images/background_1.jpeg";

//SPRITES:
const char *DIRT_GIRL_SPRITE = "images/dirt_girl_front_facing.png";
const char *PLANT_BOY_SPRITE = "images/plant_boy_front_facing-removebg-preview.png";
const char *TREE_SPRITE = "images/tree.png";

//OBSTACLES:
const char *DIRT_GIRL_POISON = "images/dirt_girl_poison.png"; //TEXTURE TODO
const char *PLANT_BOY_POISON = "images/plant_boy_poison.png"; 
const char *BLOCK_TO_PUSH = "images/block_to_push.png";

//POWERUPS/BOOSTS:
const char *TRAMPOLINE = "images/trampoline.png"; //TEXTURE TODO
const char *DIRT_GIRL_FERTILIZER = "images/dirt_girl_fertilizer.png"; 
const char *PLANT_BOY_FERTILIZER = "images/plant_boy_fertilizer.png";
const char *STAR_OF_MASTERY = "images/star_of_mastery.png";
const char *PORTAL = "images/portal.png"; //TEXTURE TODO

//DOORS + LEDGES:
const char *DIRT_GIRL_DOOR = "images/dirt_girl_door.jpg";
const char *PLANT_BOY_DOOR = "images/plant_boy_door.jpg";
const char *TOP_LEDGE = "images/top_ledge.png";

//TEXTURE TODO: IMPLEMENT THESE TEXTURES IF ANIMATING
const char *DIRT_GIRL_WALK_LEFT_1 = "images/dirt_girl_walk_left_1.png";
const char *DIRT_GIRL_WALK_LEFT_2 = "images/dirt_girl_walk_left_2.png";
const char *DIRT_GIRL_WALK_RIGHT_1 = "images/dirt_girl_walk_right_1.png";
const char *DIRT_GIRL_WALK_RIGHT_2 = "images/dirt_girl_walk_right_2.png";

const char *PLANT_BOY_WALK_LEFT_1 = "images/plant_boy_walk_left_1.png";
const char *PLANT_BOY_WALK_LEFT_2 = "images/plant_boy_walk_left_2.png";
const char *PLANT_BOY_WALK_RIGHT_1 = "images/plant_boy_walk_right_1.png";
const char *PLANT_BOY_WALK_RIGHT_2 = "images/plant_boy_walk_right_2.png";

/**
 * The coordinate at the center of the screen.
 */
vector_t center;
/**
 * The coordinate difference from the center to the top right corner.
 */
vector_t max_diff;
/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to mesasure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
  int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
  assert(width != NULL);
  assert(height != NULL);
  SDL_GetWindowSize(window, width, height);
  vector_t dimensions = {.x = *width, .y = *height};
  free(width);
  free(height);
  return vec_multiply(0.5, dimensions);
}

/**
 * Computes the scaling factor between scene coordinates and pixel coordinates.
 * The scene is scaled by the same factor in the x and y dimensions,
 * chosen to maximize the size of the scene while keeping it in the window.
 */
double get_scene_scale(vector_t window_center) {
  // Scale scene so it fits entirely in the window
  double x_scale = window_center.x / max_diff.x,
         y_scale = window_center.y / max_diff.y;
  return x_scale < y_scale ? x_scale : y_scale;
}

/** Maps a scene coordinate to a window coordinate */
vector_t get_window_position(vector_t scene_pos, vector_t window_center) {
  // Scale scene coordinates by the scaling factor
  // and map the center of the scene to the center of the window
  vector_t scene_center_offset = vec_subtract(scene_pos, center);
  double scale = get_scene_scale(window_center);
  vector_t pixel_center_offset = vec_multiply(scale, scene_center_offset);
  vector_t pixel = {.x = round(window_center.x + pixel_center_offset.x),
                    // Flip y axis since positive y is down on the screen
                    .y = round(window_center.y - pixel_center_offset.y)};
  return pixel;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
  switch (key) {
  case SDLK_LEFT:
    return LEFT_ARROW;
  case SDLK_UP:
    return UP_ARROW;
  case SDLK_RIGHT:
    return RIGHT_ARROW;
  case SDLK_DOWN:
    return DOWN_ARROW;
  case SDLK_w:
    return W_KEY;
  case SDLK_a:
    return A_KEY;
  case SDLK_d:
    return D_KEY;
  default:
    // Only process 7-bit ASCII characters
    return key == (SDL_Keycode)(char)key ? key : '\0';
  }
}

void sdl_init(vector_t min, vector_t max) {
  // Check parameters
  assert(min.x < max.x);
  assert(min.y < max.y);

  center = vec_multiply(0.5, vec_add(min, max));
  max_diff = vec_subtract(max, center);
  SDL_Init(SDL_INIT_EVERYTHING);
  window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                            SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
}

bool sdl_is_done(state_t *state) {
  SDL_Event *event = malloc(sizeof(*event));
  assert(event != NULL);
  while (SDL_PollEvent(event)) {
    switch (event->type) {
    case SDL_QUIT:
      free(event);
      return true;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      // Skip the keypress if no handler is configured
      // or an unrecognized key was pressed
      if (key_handler == NULL)
        break;
      char key = get_keycode(event->key.keysym.sym);
      if (key == '\0')
        break;

      uint32_t timestamp = event->key.timestamp;
      if (!event->key.repeat) {
        key_start_timestamp = timestamp;
      }
      key_event_type_t type =
          event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
      double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
      key_handler(key, type, held_time, state);
      break;
    }
  }
  free(event);
  return false;
}

void sdl_clear(void) {
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
}

void sdl_draw_polygon(list_t *points, rgb_color_t color) {
  // Check parameters
  size_t n = list_size(points);
  assert(n >= 3);
  assert(0 <= color.r && color.r <= 1);
  assert(0 <= color.g && color.g <= 1);
  assert(0 <= color.b && color.b <= 1);

  vector_t window_center = get_window_center();

  // Convert each vertex to a point on screen
  int16_t *x_points = malloc(sizeof(*x_points) * n),
          *y_points = malloc(sizeof(*y_points) * n);
  assert(x_points != NULL);
  assert(y_points != NULL);
  for (size_t i = 0; i < n; i++) {
    vector_t *vertex = list_get(points, i);
    vector_t pixel = get_window_position(*vertex, window_center);
    x_points[i] = pixel.x;
    y_points[i] = pixel.y;
  }

  // Draw polygon with the given color
  filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                    color.g * 255, color.b * 255, 255);
  free(x_points);
  free(y_points);
}

void sdl_show(void) {
  // Draw boundary lines
  vector_t window_center = get_window_center();
  vector_t max = vec_add(center, max_diff),
           min = vec_subtract(center, max_diff);
  vector_t max_pixel = get_window_position(max, window_center),
           min_pixel = get_window_position(min, window_center);
  SDL_Rect *boundary = malloc(sizeof(*boundary));
  boundary->x = min_pixel.x;
  boundary->y = max_pixel.y;
  boundary->w = max_pixel.x - min_pixel.x;
  boundary->h = min_pixel.y - max_pixel.y;
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, boundary);
  free(boundary);

  SDL_RenderPresent(renderer);
}

/*void load_textures() {
  TODO: move the texture loading into this method
}*/

void sdl_render_scene(scene_t *scene) {
  sdl_clear();

  SDL_Texture *BG_TEXTURE = IMG_LoadTexture(renderer, BG);

  SDL_Texture *PLANT_BOY_TEXTURE = IMG_LoadTexture(renderer, PLANT_BOY_SPRITE);
  SDL_Rect plant_boy_rect = {500,500,100,100};

  SDL_Texture *DIRT_GIRL_TEXTURE = IMG_LoadTexture(renderer, DIRT_GIRL_SPRITE);
  SDL_Rect dirt_girl_rect = {500,500,100,100};

  SDL_Texture *TREE_TEXTURE = IMG_LoadTexture(renderer, TREE_SPRITE);
  SDL_Rect tree_rect = {500,500,100,100};

  SDL_Texture *BLOCK_TEXTURE = IMG_LoadTexture(renderer, BLOCK_TO_PUSH);
  SDL_Rect block_rect = {500,500,100,100};

  SDL_Texture *DIRT_GIRL_FERTILIZER_TEXTURE = IMG_LoadTexture(renderer, DIRT_GIRL_FERTILIZER);
  SDL_Rect dirt_girl_fertilizer_rect = {0,0,75,75};

  SDL_Texture *PLANT_BOY_FERTILIZER_TEXTURE = IMG_LoadTexture(renderer, PLANT_BOY_FERTILIZER);
  SDL_Rect plant_boy_fertilizer_rect = {0,0,75,75};

  SDL_Texture *STAR_OF_MASTERY_TEXTURE = IMG_LoadTexture(renderer, STAR_OF_MASTERY);
  SDL_Rect star_of_mastery_rect = {500,500,100,100};

  SDL_Texture *DIRT_GIRL_DOOR_TEXTURE = IMG_LoadTexture(renderer, DIRT_GIRL_DOOR);
  SDL_Rect dirt_girl_door_rect = {500,500,100,100};

  SDL_Texture *PLANT_BOY_DOOR_TEXTURE = IMG_LoadTexture(renderer, PLANT_BOY_DOOR);
  SDL_Rect plant_boy_door_rect = {500,500,100,100};
  
  SDL_Texture *PORTAL_TEXTURE = IMG_LoadTexture(renderer, PORTAL);
  SDL_Rect portal_rect = {500,500,25,100};

  size_t body_count = scene_bodies(scene);
  for (size_t i = 0; i < body_count; i++) {
    body_t *body = scene_get_body(scene, i);
    list_t *shape = body_get_shape(body);
    vector_t centroid = body_get_centroid(body);
    vector_t window = get_window_position(centroid, get_window_center());
    //SPRITES:
    if (body_get_info(body) == 1) {
      plant_boy_rect.x = window.x - 140*get_scene_scale(get_window_center());
      plant_boy_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 2) {
      dirt_girl_rect.x = window.x - 140*get_scene_scale(get_window_center());
      dirt_girl_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 16) {
      tree_rect.x = window.x - 140*get_scene_scale(get_window_center());
      tree_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    //OBSTACLES:
    if (body_get_info(body) == 11) {
      block_rect.x = window.x - 140*get_scene_scale(get_window_center());
      block_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    //POWERUPS:
    if (body_get_info(body) == 13) {
      plant_boy_fertilizer_rect.x = window.x - 140*get_scene_scale(get_window_center());
      plant_boy_fertilizer_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 14) {
      dirt_girl_fertilizer_rect.x = window.x - 140*get_scene_scale(get_window_center());
      dirt_girl_fertilizer_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 15) {
      star_of_mastery_rect.x = window.x - 140*get_scene_scale(get_window_center());
      star_of_mastery_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 17) {
      portal_rect.x = window.x - 140*get_scene_scale(get_window_center());
      portal_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    //DOORS + LEDGES:
     if (body_get_info(body) == 9) {
      dirt_girl_door_rect.x = window.x - 140*get_scene_scale(get_window_center());
      dirt_girl_door_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    if (body_get_info(body) == 7) {
      plant_boy_door_rect.x = window.x - 140*get_scene_scale(get_window_center());
      plant_boy_door_rect.y = window.y - 140*get_scene_scale(get_window_center());
    }
    sdl_draw_polygon(shape, body_get_color(body));
    list_free(shape);
  }

  //BACKGROUND IMAGE
  SDL_RenderCopy(renderer, BG_TEXTURE, NULL, NULL); 
  //SPRITES
  SDL_RenderCopy(renderer, PLANT_BOY_TEXTURE, NULL, &plant_boy_rect);
  SDL_RenderCopy(renderer, DIRT_GIRL_TEXTURE, NULL, &dirt_girl_rect);
  SDL_RenderCopy(renderer, TREE_TEXTURE, NULL, &tree_rect);
  //OBJECTS
  SDL_RenderCopy(renderer, BLOCK_TEXTURE, NULL, &block_rect);
  SDL_RenderCopy(renderer, DIRT_GIRL_FERTILIZER_TEXTURE, NULL, &dirt_girl_fertilizer_rect);
  SDL_RenderCopy(renderer, PLANT_BOY_FERTILIZER_TEXTURE, NULL, &plant_boy_fertilizer_rect);
  SDL_RenderCopy(renderer, STAR_OF_MASTERY_TEXTURE, NULL, &star_of_mastery_rect);
  SDL_RenderCopy(renderer, DIRT_GIRL_DOOR_TEXTURE, NULL, &dirt_girl_door_rect);
  SDL_RenderCopy(renderer, PLANT_BOY_DOOR_TEXTURE, NULL, &plant_boy_door_rect);
  SDL_RenderCopy(renderer, PORTAL_TEXTURE, NULL, &portal_rect);

  sdl_show();
}

/*void destroy_textures() {
  SDL_Delay(2000);
  SDL_DestroyTexture(BG_TEXTURE);
  SDL_DestroyTexture(PLANT_BOY_TEXTURE);
  SDL_DestroyTexture(DIRT_GIRL_TEXTURE);
  SDL_DestroyTexture(TREE_TEXTURE);
  SDL_DestroyTexture(BLOCK_TEXTURE);
  SDL_DestroyTexture(DIRT_GIRL_FERTILIZER_TEXTURE);
  SDL_DestroyTexture(PLANT_BOY_FERTILIZER_TEXTURE);
  SDL_DestroyTexture(STAR_OF_MASTERY_TEXTURE);
  SDL_DestroyTexture(DIRT_GIRL_DOOR_TEXTURE);
  SDL_DestroyTexture(PLANT_BOY_DOOR_TEXTURE);
  SDL_DestroyTexture(PORTAL_TEXTURE);
  SDL_RenderPresent(renderer);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}*/

void sdl_on_key(key_handler_t handler) { key_handler = handler; }

double time_since_last_tick(void) {
  clock_t now = clock();
  double difference = last_clock
                          ? (double)(now - last_clock) / CLOCKS_PER_SEC
                          : 0.0; // return 0 the first time this is called
  last_clock = now;
  return difference;
}