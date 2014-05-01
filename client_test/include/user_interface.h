#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <SDL.h>
#include "blocks.h"

#define WINDOW_W 800
#define WINDOW_H 600

void init_ui();
void draw_frame();
void quit_ui();
void draw_map(min_block_type *map, int w, int h);

#endif // USER_INTERFACE_H
