#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <SDL.h>
#include "blocks.hpp"

#define WINDOW_W 800
#define WINDOW_H 600

void init_ui(int w_, int h_);
void draw_frame();
void quit_ui();
void draw_map(min_block_type *map, int vx, int vy);
void draw_map_with_buff_offset(min_block_type *map, int buff_off_x, int buff_off_y, int buffer_w, int buffer_h, int vx, int vy);
void reinit_ui(int w_, int h_);
void draw_loading(int percent);
void flashback_init();
void new_flashback(int x, int  y, min_block_type inf);

extern bool ui_redraw_needed;

#endif // USER_INTERFACE_H
