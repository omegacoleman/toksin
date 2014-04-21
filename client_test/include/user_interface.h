#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <SDL.h>
#include "blocks.h"

void init_ui();
void draw_frame(SDL_Surface *frame);
void quit_ui();
SDL_Surface *gen_surface_from_map(min_block_type *map, int w, int h);

#endif // USER_INTERFACE_H
