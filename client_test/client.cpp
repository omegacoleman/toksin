#include <glib.h>
#include <gio/gio.h>
#include "protocol.hpp"
#include "blocks.hpp"
#include "user_interface.hpp"

typedef min_block_type block;
bool last_ping_ponged = true;
bool flushing = false;
gint64 last_ping_time;
#define SCR_W_NORM 40
#define SCR_H_NORM 30
#define SCR_W_X 80
#define SCR_H_X 60
#define SCREEN_STEP 5
#define SCREEN_LARGE_STEP 15
#define PR_STEP 5
int vx, vy;
int pr_vx, pr_vy;
magic_code i_magic;
GError* error = NULL;
block world_landscape[WORLD_HEIGHT * WORLD_WIDTH];
bool need_redraw = true;

int SCR_W = SCR_W_NORM;
int SCR_H = SCR_H_NORM;
void event_hooker(GInputStream* istream, GAsyncResult* result, GOutputStream * ostream);
void start_poll_events(GInputStream * istream, GOutputStream * ostream);

#define ifnsucceed(e, w) if(e != NULL){g_error("Error occured when : %s", w);}

void do_magic(GInputStream * istream, GOutputStream * ostream)
{
    gsize bytes_write;
    g_output_stream_write_all(ostream, &MAGIC, sizeof(magic_code), &bytes_write, NULL, &error);
	ifnsucceed(error, "do_magic");
    g_assert(bytes_write == sizeof(magic_code));
}

void ping(GInputStream * istream, GOutputStream * ostream)
{
    gsize bytes_write;
    do_magic(istream, ostream);
    if (last_ping_ponged)
    {
        operation_code op = OPC_PING;
        last_ping_time = g_get_real_time();
        g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
		ifnsucceed(error, "ping");
        g_assert(bytes_write == sizeof(operation_code));
    }
}

void flush_screen(GInputStream * istream, GOutputStream * ostream)
{
    gsize bytes_write;
    if (!flushing)
    {
        flushing = true;
        do_magic(istream, ostream);
        operation_code op = OPC_GET_RANGE;
        g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
		ifnsucceed(error, "writing opcode in flush_screen");
        g_assert(bytes_write == sizeof(operation_code));
        op_get_range pend;
        pend.xa = 0;
        pend.ya = 0;
		pend.xb = WORLD_WIDTH - 1;
		pend.yb = WORLD_HEIGHT - 1;
        g_output_stream_write_all(ostream, &pend, sizeof(op_get_range), &bytes_write, NULL, &error);
		ifnsucceed(error, "pending operation in flush_screen");
        g_assert(bytes_write == sizeof(op_get_range));
    }
}

void flush_screen(GInputStream * istream, GOutputStream * ostream, int l ,int t, int r, int b)
{
    gsize bytes_write;
    if (!flushing)
    {
        flushing = true;
        do_magic(istream, ostream);
        operation_code op = OPC_GET_RANGE;
        g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
		ifnsucceed(error, "writing opcode in flush_screen");
        g_assert(bytes_write == sizeof(operation_code));
        op_get_range pend;
        pend.xa = l;
        pend.ya = t;
        pend.xb = r;
        pend.yb = b;
        g_output_stream_write_all(ostream, &pend, sizeof(op_get_range), &bytes_write, NULL, &error);
		ifnsucceed(error, "pending operation in flush_screen");
        g_assert(bytes_write == sizeof(op_get_range));
    }
}

void do_dig(GInputStream * istream, GOutputStream * ostream, uint32_t x, uint32_t y)
{
    op_dig dig;
    dig.xa = x + vx;
    dig.ya = y + vy;
    gsize bytes_write;
    do_magic(istream, ostream);
    operation_code op = OPC_DIG;
    g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
	ifnsucceed(error, "writing opcode in do_dig");
    g_assert(bytes_write == sizeof(operation_code));
    g_output_stream_write_all(ostream, &dig, sizeof(op_dig), &bytes_write, NULL, &error);
	ifnsucceed(error, "pending operation in do_dig");
    g_assert(bytes_write == sizeof(op_dig));
}

void do_place(GInputStream * istream, GOutputStream * ostream, uint32_t x, uint32_t y)
{
    op_place placement;
    placement.xa = x + vx;
    placement.ya = y + vy;
    gsize bytes_write;
    do_magic(istream, ostream);
	operation_code op = OPC_PLACE;
    g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
	ifnsucceed(error, "writing opcode in do_place");
    g_assert(bytes_write == sizeof(operation_code));
    g_output_stream_write_all(ostream, &placement, sizeof(op_dig), &bytes_write, NULL, &error);
	ifnsucceed(error, "pending operation in do_place");
    g_assert(bytes_write == sizeof(op_dig));
}

void end_connection(GInputStream * istream, GOutputStream * ostream)
{
    gsize bytes_write;
    do_magic(istream, ostream);
    operation_code op = OPC_CLOSE;
    g_output_stream_write_all(ostream, &op, sizeof(operation_code), &bytes_write, NULL, &error);
	ifnsucceed(error, "end_connection");
    g_assert(bytes_write == sizeof(operation_code));
}

void event_hooker(GInputStream* istream, GAsyncResult* result, GOutputStream * ostream)
{
    gsize bytes_read;
    g_input_stream_read_finish(istream, result, &error);
	ifnsucceed(error, "event_hooker");
    g_assert(i_magic == MAGIC);
    operation_code rsp_opc;
    g_input_stream_read_all(istream, &rsp_opc, sizeof(operation_code), &bytes_read, NULL, &error);
	ifnsucceed(error, "reading rsp code in event_hooker");
    g_assert(bytes_read == sizeof(operation_code));
    switch(rsp_opc)
    {
    case RSP_PONG:
    {
        if (!last_ping_ponged)
        {
            last_ping_ponged = true;
            g_message("Your ping : %d", g_get_real_time() - last_ping_time);
        }
    }
    break;
    case RSP_SET_BLOCK:
    {
        rsp_set_block rsp_struct;
        g_input_stream_read_all(istream, &rsp_struct, sizeof(rsp_set_block), &bytes_read, NULL, &error);
		ifnsucceed(error, "RSP_SET_BLOCK in event_hooker s1");
        g_assert(bytes_read == sizeof(rsp_set_block));
		int startp = rsp_struct.starty * WORLD_WIDTH + rsp_struct.startx;
		g_input_stream_read_all(istream, &(world_landscape[startp]), sizeof(block) * rsp_struct.amount, &bytes_read, NULL, &error);
		ifnsucceed(error, "RSP_SET_BLOCK in event_hooker s2");
		g_assert(bytes_read == (sizeof(block) * rsp_struct.amount));
    }
    break;
    case RSP_FLUSH_REQUEST:
        flush_screen(istream, ostream);
        break;
	case RSP_FLUSH_DONE:
		flushing = false;
		break;
	case RSP_ATOMIC_UPDATE:
		{
			rsp_atomic_update rsp_struct;
			g_input_stream_read_all(istream, &rsp_struct, sizeof(rsp_atomic_update), &bytes_read, NULL, &error);
			ifnsucceed(error, "RSP_ATOMIC_UPDATE in event_hooker s1");
			g_assert(bytes_read == sizeof(rsp_atomic_update));
			world_landscape[rsp_struct.starty * WORLD_WIDTH + rsp_struct.startx] = rsp_struct.atom;
			need_redraw = true;
		}
		break;
    }
    start_poll_events(istream, ostream);
}

void start_poll_events(GInputStream * istream, GOutputStream * ostream)
{
    g_input_stream_read_async(istream, &i_magic, sizeof(magic_code), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback)event_hooker, ostream);
}

GInputStream * istream;
GOutputStream * ostream;

void shut()
{
    end_connection(istream, ostream);
    quit_ui();
    g_print("Bye! \n");
}

bool check_need_redraw()
{
	if(pr_vx > vx)
	{
		pr_vx -= PR_STEP;
		need_redraw = true;
	}
	if(pr_vx < vx)
	{
		pr_vx += PR_STEP;
		need_redraw = true;
	}
	if(pr_vy > vy)
	{
		pr_vy -= PR_STEP;
		need_redraw = true;
	}
	if(pr_vy < vy)
	{
		pr_vy += PR_STEP;
		need_redraw = true;
	}
	return need_redraw;
}

void step_game()
{
	if (!flushing)
	{
		check_need_redraw();
		if(need_redraw)
		{
			draw_map_with_buff_offset(world_landscape, pr_vx, pr_vy, WORLD_WIDTH, WORLD_HEIGHT, pr_vx, pr_vy);
			draw_frame();
			need_redraw = false;
		}
		if(! check_need_redraw())
		{
			const Uint8 *state = SDL_GetKeyboardState(NULL);
			if (state[SDL_SCANCODE_W])
			{
				if (vy >= SCREEN_STEP)
				{
					vy -= SCREEN_STEP;
				}
			}else if (state[SDL_SCANCODE_S])
			{
				if (vy < (WORLD_HEIGHT - SCREEN_STEP - SCR_H))
				{
					vy += SCREEN_STEP;
				}
			}else if (state[SDL_SCANCODE_A])
			{
				if (vx >= SCREEN_STEP)
				{
					vx -= SCREEN_STEP;
				}
			}else if (state[SDL_SCANCODE_D])
			{
				if (vx < (WORLD_WIDTH - SCREEN_STEP - SCR_W))
				{
					vx += SCREEN_STEP;
				}
			}else if (state[SDL_SCANCODE_I])
			{
				if (vy >= SCREEN_LARGE_STEP)
				{
					vy -= SCREEN_LARGE_STEP;
				}
			}else if (state[SDL_SCANCODE_K])
			{
				if (vy < (WORLD_HEIGHT - SCREEN_LARGE_STEP - SCR_H))
				{
					vy += SCREEN_LARGE_STEP;
				}
			}else if (state[SDL_SCANCODE_J])
			{
				if (vx >= SCREEN_LARGE_STEP)
				{
					vx -= SCREEN_LARGE_STEP;
				}
			}else if (state[SDL_SCANCODE_L])
			{
				if (vx < (WORLD_WIDTH - SCREEN_LARGE_STEP - SCR_W))
				{
					vx += SCREEN_LARGE_STEP;
				}
			}
		}
    }
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
			shut();
            exit(0);
        }else if (e.type == SDL_MOUSEBUTTONDOWN)
        {
			if (!flushing)
			{
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					do_dig(istream , ostream,  e.button.x * SCR_W / WINDOW_W, e.button.y * SCR_H / WINDOW_H);
				}
				if (e.button.button == SDL_BUTTON_RIGHT)
				{
					do_place(istream , ostream,  e.button.x * SCR_W / WINDOW_W, e.button.y * SCR_H / WINDOW_H);
				}
			}
        }else if (e.type == SDL_KEYDOWN)
		{
			if(!flushing)
			{
				if(e.key.keysym.scancode == SDL_SCANCODE_Q)
				{
					SCR_W = SCR_W_X;
					SCR_H = SCR_H_X;
					reinit_ui(SCR_W, SCR_H);
					if (vx >= (WORLD_WIDTH - SCR_W))
					{
						pr_vx = vx = WORLD_WIDTH - SCR_W - 1;
					}
					if (vy >= (WORLD_HEIGHT - SCR_H))
					{
						pr_vy = vy = WORLD_HEIGHT - SCR_H - 1;
					}
					flush_screen(istream, ostream);
					need_redraw = true;
				} else if(e.key.keysym.scancode == SDL_SCANCODE_E)
				{
					SCR_W = SCR_W_NORM;
					SCR_H = SCR_H_NORM;
					reinit_ui(SCR_W, SCR_H);
					if (vx >= (WORLD_WIDTH - SCR_W))
					{
						pr_vx = vx = WORLD_WIDTH - SCR_W - 1;
					}
					if (vy >= (WORLD_HEIGHT - SCR_H))
					{
						pr_vy = vy = WORLD_HEIGHT - SCR_H - 1;
					}
					flush_screen(istream, ostream);
					need_redraw = true;
				}
			}
		} else {
			need_redraw = true;
		}
    }
}

gboolean callback_step(gpointer data)
{
	step_game();
	return TRUE;
}

int main (int argc, char *argv[])
{
    g_type_init();
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GSocketConnection * connection = NULL;
    GSocketClient * client = g_socket_client_new();
    if (argc > 1)
    {
        g_message("Server : %s", argv[1]);
        connection = g_socket_client_connect_to_host (client, (gchar*)argv[1], 1500, NULL, &error);
    }
    else
    {
        connection = g_socket_client_connect_to_host (client, (gchar*)"127.0.0.1", 1500, NULL, &error);
    }
	ifnsucceed(error, "connecting to the host");
    istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
    ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    init_ui(SCR_W, SCR_H);
    pr_vx = vx = 0;
    pr_vy = vy = 0x8f - 7;
    off_vx = 0;
    off_vy = 0;
    flush_screen(istream, ostream);
    start_poll_events(istream, ostream);
	g_timeout_add(30, callback_step, NULL);
    g_main_loop_run(loop);
    return 0;
}
