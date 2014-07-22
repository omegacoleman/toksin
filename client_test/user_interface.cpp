#include <SDL.h>
#include <glib.h>
#include "user_interface.hpp"
#include "blocks.hpp"

static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Texture *tex;
SDL_Surface *surface;

Uint8 curr_transp = 255;
Uint8 target_transp = 0;

SDL_Surface *s_b_dirt;
SDL_Surface *s_b_brick;
SDL_Surface *s_b_grass;
SDL_Surface *s_b_sky;
SDL_Surface *s_b_sky_night;
SDL_Surface *s_b_sky_day;
SDL_Surface *s_b_logo;
SDL_Surface *s_b_shadows[2][2][2];

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

void draw_block(min_block_type blck, int x, int y, int rx, int ry, int pw=w, int ph=h, SDL_Surface *extra=NULL)
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
	if (extra != NULL)
	{
		SDL_BlitScaled(extra, NULL, surface, &cr);
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
    win = SDL_CreateWindow("Toksin 1.1.4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
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
	s_b_shadows[0][0][0] = NULL;
	s_b_shadows[1][0][0] = SDL_LoadBMP("./res/shadow_001.bmp");
	s_b_shadows[0][0][1] = SDL_LoadBMP("./res/shadow_100.bmp");
	s_b_shadows[0][1][0] = SDL_LoadBMP("./res/shadow_010.bmp");
	s_b_shadows[1][0][1] = SDL_LoadBMP("./res/shadow_101.bmp");
	s_b_shadows[1][1][1] = SDL_LoadBMP("./res/shadow_111.bmp");
	s_b_shadows[1][1][0] = SDL_LoadBMP("./res/shadow_011.bmp");
	s_b_shadows[0][1][1] = SDL_LoadBMP("./res/shadow_110.bmp");
	SDL_SetColorKey(s_b_shadows[0][0][1], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[0][1][0], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[0][1][1], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[1][0][0], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[1][0][1], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[1][1][0], 2, 0xffffffff);
	SDL_SetColorKey(s_b_shadows[1][1][1], 2, 0xffffffff);
}

void update_sky()
{
	if (target_transp != curr_transp)
	{
		curr_transp += ((target_transp - curr_transp) > 0) ? 5 : -5;
		SDL_BlitSurface(s_b_sky_day, NULL, s_b_sky, NULL);
		SDL_SetSurfaceAlphaMod(s_b_sky_night, curr_transp);
		SDL_SetSurfaceBlendMode(s_b_sky_night, SDL_BLENDMODE_BLEND);
		SDL_BlitSurface(s_b_sky_night, NULL, s_b_sky, NULL);
	}
}

void change_day_and_night()
{
	if (target_transp == 0)
	{
		target_transp = 255;
	}
	else {
		target_transp = 0;
	}
}

bool day_night_need_update()
{
	return (target_transp != curr_transp);
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

#define GET_BLOCK_FROM_BUFFER(b, x, y, bx, by, bw, bh) b[((y + by + bh) % bh) * bw + ((x + bx + bw) % bw)]

void draw_map_with_buff_offset(min_block_type *map, int buff_off_x, int buff_off_y, int buffer_w, int buffer_h, int vx, int vy)
{
#define GET_BLOCK_FROM_THIS(x, y) GET_BLOCK_FROM_BUFFER(map, x, y, buff_off_x, buff_off_y, buffer_w, buffer_h)
	update_sky();
	SDL_FillRect(surface, NULL, 0xFFFFFFFF);
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
			min_block_type cb = GET_BLOCK_FROM_THIS(j, i);
			if (cb == BLCK_AIR)
			{
				int pu = GET_BLOCK_FROM_THIS(j - 1, i) != BLCK_AIR;
				int pl = GET_BLOCK_FROM_THIS(j, i - 1) != BLCK_AIR;
				int plu = GET_BLOCK_FROM_THIS(j - 1, i - 1) != BLCK_AIR;
				draw_block(cb, j, i, vx + j, vy + i, w, h, s_b_shadows[pu][plu][pl]);
			} else {
				draw_block(cb, j, i, vx + j, vy + i);
			}
        }
    }
	for(int i = 0; i < FLASHBACK_MAX; i++)
	{
		if((flashbacks[i].frames_left > 0) && (flashbacks[i].x >= vx) && (flashbacks[i].x < (vx + w)) && (flashbacks[i].y >= vy) && (flashbacks[i].y < (vy + h)))
		{
			if (flashbacks[i].inf == BLCK_AIR)
			{
				int pu = GET_BLOCK_FROM_THIS(flashbacks[i].x - vx - 1, flashbacks[i].y - vy) != BLCK_AIR;
				int pl = GET_BLOCK_FROM_THIS(flashbacks[i].x - vx, flashbacks[i].y - vy - 1) != BLCK_AIR;
				int plu = GET_BLOCK_FROM_THIS(flashbacks[i].x - vx - 1, flashbacks[i].y - vy - 1) != BLCK_AIR;
				draw_block(flashbacks[i].inf, flashbacks[i].x - vx, flashbacks[i].y - vy, flashbacks[i].x, flashbacks[i].y, w, h, s_b_shadows[pu][plu][pl]);
			}
			else {
				draw_block(flashbacks[i].inf, flashbacks[i].x - vx, flashbacks[i].y - vy, flashbacks[i].x, flashbacks[i].y);
			}
			// flashbacks[i].frames_left--;
		}
	}
	ui_redraw_needed = false;
#undef GET_BLOCK_FROM_THIS
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
