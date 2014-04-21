#include <SDL.h>
#include <glib.h>
#include "user_interface.h"
#include "blocks.h"

#define WINDOW_W 800
#define WINDOW_H 600

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;

void init_ui()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
		g_error("SDL_Init Error: %s", SDL_GetError());
	}
	win = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
	if (win == nullptr){
		g_error("SDL_CreateWindow Error: %s", SDL_GetError());
	}
	ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (ren == nullptr){
		g_error("SDL_CreateRenderer Error: %s", SDL_GetError());
	}
}

void draw_frame(SDL_Surface *frame)
{
	SDL_DestroyTexture(tex);
	tex = SDL_CreateTextureFromSurface(ren, frame);
	SDL_FreeSurface(frame);
	if (tex == nullptr){
		g_error("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
	}
	SDL_RenderClear(ren);
	SDL_RenderCopy(ren, tex, NULL, NULL);
	SDL_RenderPresent(ren);
}

SDL_Surface *gen_surface_from_map(min_block_type *map, int w, int h)
{
	SDL_Rect cr;
	cr.w = WINDOW_W / w;
	cr.h = WINDOW_H / h;
	SDL_Surface *surface = SDL_CreateRGBSurface(0, WINDOW_W, WINDOW_H, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			cr.x = j * cr.w;
			cr.y = i * cr.h;
			switch(map[i * w + j] & 0xffff)
			{
			case BLCK_AIR:
				SDL_FillRect(surface, &cr, 0xffffffff);
				break;
			case BLCK_DIRT:
				SDL_FillRect(surface, &cr, 0xff000000);
				break;
			}
		}
	}
	return surface;
}

void quit_ui()
{
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
}
