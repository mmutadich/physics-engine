#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "body.h"
#include "color.h"
#include "list.h"
#include "vector.h"
#include <stdbool.h>

typedef struct player player_t;

player_t *player_init_with_info(body_t *body, void *info,
                                free_func_t info_freer);

void player_free(player_t *player);

body_t *player_get_body(player_t *player);

void *player_get_info(player_t *player);

#endif // #ifndef __PLAYER_H__
