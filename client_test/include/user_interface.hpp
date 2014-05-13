#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <SDL.h>
#include "blocks.hpp"

#define WINDOW_W 800
#define WINDOW_H 600

void init_ui();
void draw_frame();
void quit_ui();
void draw_map(min_block_type *map, int w, int h);
void draw_map_with_buff_offset(min_block_type *map, int w, int h, int buff_off_x, int buff_off_y, int buffer_w, int buffer_h);

#endif // USER_INTERFACE_H
