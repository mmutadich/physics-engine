#include "player.h"
#include "body.h"
#include "color.h"
#include "list.h"
#include "polygon.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct player {
  body_t *body;
  void *info;
  free_func_t info_freer;
} player_t;

player_t *player_init_with_info(body_t *body, void *info,
                                free_func_t info_freer) {
  player_t *result = malloc(sizeof(player_t));
  assert(result);
  result->body = body;
  result->info = info; // PLANT_BOY OR DIRT_GIRL
  result->info_freer = info_freer;
  return result;
}

void player_free(player_t *player) {
  if (player->info != NULL && player->info_freer != NULL)
    player->info_freer(player->info);
  body_free(player->body);
  free(player);
}

body_t *player_get_body(player_t *player) { return player->body; }

void *player_get_info(player_t *player) { return player->info; }
