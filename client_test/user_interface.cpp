#include <SDL.h>
#include <glib.h>
#include "user_interface.hpp"
#include "blocks.hpp"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
SDL_Surface *surface;

SDL_Surface *s_b_dirt;
SDL_Surface *s_b_brick;
SDL_Surface *s_b_grass;
SDL_Surface *s_b_sky;
SDL_Surface *s_b_sky_night;
SDL_Surface *s_b_sky_day;
SDL_Surface *s_b_logo;

SDL_Rect cr;
int w, h;


typedef struct _flashback_task
{
	int x, y;
	min_block_type inf;
	int frames_left;
}flashback_task;

#define FLASHBACK_MAX 255
#define FLASHBACK_INTERVAL 800
flashback_task flashbacks[FLASHBACK_MAX];
int flashback_top = 0;
bool ui_redraw_needed = false;

void flashback_init()
{
	for(int i = 0; i < FLASHBACK_MAX; i++)
	{
		flashbacks[i].frames_left = 0;
	}
	flashback_top = 0;
}

gboolean callback_del_flashback(gpointer data)
{
	flashback_task *ft = (flashback_task *)data;
	ft->frames_left = 0;
	ui_redraw_needed = true;
	return FALSE;
}

void new_flashback(int x, int  y, min_block_type inf)
{
	flashbacks[flashback_top].x = x;
	flashbacks[flashback_top].y = y;
	flashbacks[flashback_top].inf = inf;
	flashbacks[flashback_top].frames_left = FLASHBACK_INTERVAL;
	g_timeout_add(FLASHBACK_INTERVAL, callback_del_flashback, &flashbacks[flashback_top]);
	flashback_top++;
	flashback_top %= FLASHBACK_MAX;
	ui_redraw_needed = true;
}

void draw_block(min_block_type blck, int x, int y, int rx, int ry, int pw=w, int ph=h)
{
	cr.w = WINDOW_W / pw;
	cr.h = WINDOW_H / ph;
    cr.x = x * cr.w;
    cr.y = y * cr.h;
    switch(blck & 0xffff)
    {
    case BLCK_AIR:
		{
			SDL_Rect s_cr = cr;
			SDL_BlitScaled(s_b_sky, &s_cr, surface, &cr);
		}
        break;
    case BLCK_DIRT:
		{
			SDL_Rect s_cr;
			s_cr.x = 20 * (rx % 20);
			s_cr.y = 20 * (ry % 20);
			s_cr.w = s_cr.h = 20;
			SDL_BlitScaled(s_b_dirt, &s_cr, surface, &cr);
		}
        break;
    case BLCK_GRASS:
		{
			SDL_Rect s_cr;
			s_cr.x = 20 * (rx % 20);
			s_cr.y = 20 * (ry % 20);
			s_cr.w = s_cr.h = 20;
			SDL_BlitScaled(s_b_grass, &s_cr, surface, &cr);
		}
        break;
    case BLCK_BRICK:
		SDL_BlitScaled(s_b_brick, NULL, surface, &cr);
        break;
    }
}

void init_ui(int w_, int h_)
{
	w = w_;
	h = h_;
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        g_error("SDL_Init Error: %s", SDL_GetError());
    }
    win = SDL_CreateWindow("Toksin 1.1.3", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    if (win == NULL)
    {
        g_error("SDL_CreateWindow Error: %s", SDL_GetError());
    }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL)
    {
        g_error("SDL_CreateRenderer Error: %s", SDL_GetError());
    }
    surface = SDL_CreateRGBSurface(0, WINDOW_W, WINDOW_H, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	cr.w = WINDOW_W / w;
	cr.h = WINDOW_H / h;
	s_b_dirt = SDL_LoadBMP("./res/dirt.bmp");
	s_b_brick = SDL_LoadBMP("./res/brick.bmp");
	s_b_grass = SDL_LoadBMP("./res/grass.bmp");
	s_b_sky_day = SDL_LoadBMP("./res/sky_01.bmp");
	s_b_sky_night = SDL_LoadBMP("./res/sky_00.bmp");
	s_b_sky = SDL_LoadBMP("./res/sky_01.bmp");
	s_b_logo = SDL_LoadBMP("./res/logo_a.bmp");
}

void update_sky()
{
	gint sec = g_date_time_get_second(g_date_time_new_now_local());
	Uint8 alph = ABS(sec - 30) * 255 / 30;
	SDL_BlitSurface(s_b_sky_day, NULL, s_b_sky, NULL);
	SDL_SetSurfaceAlphaMod(s_b_sky_night, alph);
	SDL_SetSurfaceBlendMode(s_b_sky_night, SDL_BLENDMODE_BLEND);
	SDL_BlitSurface(s_b_sky_night, NULL, s_b_sky, NULL);
}

void reinit_ui(int w_, int h_)
{
	w = w_;
	h = h_;
	cr.w = WINDOW_W / w;
	cr.h = WINDOW_H / h;
}

void draw_frame()
{
    SDL_DestroyTexture(tex);
    tex = SDL_CreateTextureFromSurface(ren, surface);
    if (tex == NULL)
    {
        g_error("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    }
    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, NULL, NULL);
    SDL_RenderPresent(ren);
}

void draw_map(min_block_type *map, int vx, int vy)
{
	update_sky();
	SDL_FillRect(surface, NULL, 0xFFFFFFFF);
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
			draw_block(map[i * w + j], j, i, vx + j, vy + i);
        }
    }
	for(int i = 0; i < FLASHBACK_MAX; i++)
	{
		if(flashbacks[i].frames_left > 0)
		{
			draw_block(flashbacks[i].inf, flashbacks[i].x, flashbacks[i].y, vx + flashbacks[i].x, vy + flashbacks[i].y);
			flashbacks[i].frames_left--;
		}
	}
	ui_redraw_needed = false;
}

void draw_map_with_buff_offset(min_block_type *map, int buff_off_x, int buff_off_y, int buffer_w, int buffer_h, int vx, int vy)
{
	update_sky();
	SDL_FillRect(surface, NULL, 0xFFFFFFFF);
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
			min_block_type cb = map[((i + buff_off_y + buffer_h) % buffer_h) * buffer_w + ((j + buff_off_x + buffer_w) % buffer_w)];
			draw_block(cb, j, i, vx + j, vy + i);
        }
    }
	for(int i = 0; i < FLASHBACK_MAX; i++)
	{
		if((flashbacks[i].frames_left > 0) && (flashbacks[i].x >= vx) && (flashbacks[i].x < (vx + w)) && (flashbacks[i].y >= vy) && (flashbacks[i].y < (vy + h)))
		{
			draw_block(flashbacks[i].inf, flashbacks[i].x - vx, flashbacks[i].y - vy, flashbacks[i].x, flashbacks[i].y);
			// flashbacks[i].frames_left--;
		}
	}
	ui_redraw_needed = false;
}

void draw_loading(int percent)
{
	SDL_FillRect(surface, NULL, 0xFFFFFFFF);
	SDL_Rect r_s;
	SDL_Rect r_d;
	r_s.x = r_s.y = 0;
	r_s.h = s_b_logo->h;
	r_s.w = s_b_logo->w * percent / 100;
	r_d.x = (WINDOW_W - s_b_logo->w) / 2;
	r_d.y = (WINDOW_H - s_b_logo->h) / 2;
	r_d.h = s_b_logo->h;
	r_d.w = s_b_logo->w * percent / 100;
	SDL_BlitSurface(s_b_logo, &r_s, surface,&r_d);
}

void quit_ui()
{
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surface);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}
