#include <glib.h>
#include <gio/gio.h>
#include "protocol.hpp"
#include "blocks.hpp"
#include "user_interface.hpp"

typedef min_block_type block;
bool last_ping_ponged = true;
bool flushing = false;
gint64 last_ping_time;
#define SCR_W 40
#define SCR_H 30
#define BUF_W (SCR_W * 3)
#define BUF_H (SCR_H * 3)
#define SCREEN_STEP 1
#define SCREEN_LARGE_STEP 5
block screen[BUF_W * BUF_H];
uint32_t vx, vy;
uint32_t off_vx, off_vy;
magic_code i_magic;
GError* error = NULL;

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
        pend.xa = vx;
        pend.ya = vy;
        pend.xb = vx + SCR_W;
        pend.yb = vy + SCR_H;
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
		int real_x = rsp_struct.startx - vx + off_vx;
		real_x += BUF_W;
		real_x %= BUF_W;
		int startp = ((rsp_struct.starty - vy + off_vy + BUF_H) % BUF_H) * BUF_W + real_x;
		if ((real_x + rsp_struct.amount - 1) > BUF_W)
		{
			g_input_stream_read_all(istream, &(screen[startp]), sizeof(block) * (BUF_W - real_x), &bytes_read, NULL, &error);
			ifnsucceed(error, "RSP_SET_BLOCK in event_hooker s3");
			// g_assert(bytes_read == (sizeof(block) * (BUF_W - real_x)));
			int startp_ = ((rsp_struct.starty - vy + off_vy + BUF_H) % BUF_H) * BUF_W;
			g_input_stream_read_all(istream, &(screen[startp_]), sizeof(block) * (real_x + rsp_struct.amount - BUF_W), &bytes_read, NULL, &error);
			ifnsucceed(error, "RSP_SET_BLOCK in event_hooker s4");
			// g_assert(bytes_read == (sizeof(block) * (real_x + rsp_struct.amount - BUF_W)));
		} else {
			g_input_stream_read_all(istream, &(screen[startp]), sizeof(block) * rsp_struct.amount, &bytes_read, NULL, &error);
			ifnsucceed(error, "RSP_SET_BLOCK in event_hooker s2");
			g_assert(bytes_read == (sizeof(block) * rsp_struct.amount));
		}
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
			if ((vx < rsp_struct.startx) && (rsp_struct.startx < (vx + SCR_W)))
			{
				if ((vy < rsp_struct.starty) && (rsp_struct.starty < (vy + SCR_H)))
				{
					screen[((rsp_struct.starty - vy + off_vy) % BUF_H) * BUF_W + ((rsp_struct.startx - vx + off_vx) % BUF_W)] = rsp_struct.atom;
				}
			}
		}
		break;
    }
    start_poll_events(istream, ostream);
}

void start_poll_events(GInputStream * istream, GOutputStream * ostream)
{
    g_input_stream_read_async(istream, &i_magic, sizeof(magic_code), G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback)event_hooker, ostream);
}

void print_screen()
{
    g_print("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    for (int i = 0; i < SCR_H; i++)
    {
        for (int j = 0; j < SCR_W; j++)
        {
            switch(screen[i * SCR_W + j] & 0xffff)
            {
            case BLCK_AIR:
                g_print("  ");
                break;
            case BLCK_DIRT:
                g_print("##");
                break;
            }
        }
        g_print("\n");
    }
}

GInputStream * istream;
GOutputStream * ostream;

void shut()
{
    end_connection(istream, ostream);
    quit_ui();
    g_print("Bye! \n");
}

int main (int argc, char *argv[])
{
    g_type_init();
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
    init_ui();
    vx = 0;
    vy = 0x8f - 7;
    off_vx = 0;
    off_vy = 0;
    flush_screen(istream, ostream);
    start_poll_events(istream, ostream);
    while (1)
    {
        while(g_main_context_iteration(NULL, FALSE));
        // g_main_context_iteration(NULL, FALSE);
		if (!flushing)
		{
			draw_map_with_buff_offset(screen, SCR_W, SCR_H, off_vx, off_vy, BUF_W, BUF_H);
			draw_frame();
            const Uint8 *state = SDL_GetKeyboardState(NULL);
            if (state[SDL_SCANCODE_W])
            {
                if (vy >= SCREEN_STEP)
                {
                    vy -= SCREEN_STEP;
					off_vy -= SCREEN_STEP;
					off_vy += BUF_H;
					off_vy %= BUF_H;
					flush_screen(istream, ostream, vx, vy, vx + SCR_W, vy + SCREEN_STEP);
                }
            }else if (state[SDL_SCANCODE_S])
            {
                if (vy < (WORLD_HEIGHT - SCREEN_STEP - SCR_H))
                {
                    vy += SCREEN_STEP;
					off_vy += SCREEN_STEP;
					off_vy += BUF_H;
					off_vy %= BUF_H;
					flush_screen(istream, ostream, vx, vy + SCR_H - SCREEN_STEP, vx + SCR_W, vy + SCR_H);
                }
            }else if (state[SDL_SCANCODE_A])
            {
                if (vx >= SCREEN_STEP)
                {
                    vx -= SCREEN_STEP;
					off_vx -= SCREEN_STEP;
					off_vx += BUF_W;
					off_vx %= BUF_W;
					flush_screen(istream, ostream, vx, vy, vx + SCREEN_STEP, vy + SCR_H);
                }
            }else if (state[SDL_SCANCODE_D])
            {
                if (vx < (WORLD_WIDTH - SCREEN_STEP - SCR_W))
                {
                    vx += SCREEN_STEP;
					off_vx += SCREEN_STEP;
					off_vx += BUF_W;
					off_vx %= BUF_W;
					flush_screen(istream, ostream, vx + SCR_W - SCREEN_STEP, vy, vx + SCR_W, vy + SCR_H);
                }
            }else if (state[SDL_SCANCODE_I])
            {
                if (vy >= SCREEN_LARGE_STEP)
                {
                    vy -= SCREEN_LARGE_STEP;
					off_vy -= SCREEN_LARGE_STEP;
					off_vy += BUF_H;
					off_vy %= BUF_H;
					flush_screen(istream, ostream, vx, vy, vx + SCR_W, vy + SCREEN_LARGE_STEP);
                }
            }else if (state[SDL_SCANCODE_K])
            {
                if (vy < (WORLD_HEIGHT - SCREEN_LARGE_STEP - SCR_H))
                {
                    vy += SCREEN_LARGE_STEP;
					off_vy += SCREEN_LARGE_STEP;
					off_vy += BUF_H;
					off_vy %= BUF_H;
					flush_screen(istream, ostream, vx, vy + SCR_H - SCREEN_LARGE_STEP, vx + SCR_W, vy + SCR_H);
                }
            }else if (state[SDL_SCANCODE_J])
            {
                if (vx >= SCREEN_LARGE_STEP)
                {
                    vx -= SCREEN_LARGE_STEP;
					off_vx -= SCREEN_LARGE_STEP;
					off_vx += BUF_W;
					off_vx %= BUF_W;
					flush_screen(istream, ostream, vx, vy, vx + SCREEN_LARGE_STEP, vy + SCR_H);
                }
            }else if (state[SDL_SCANCODE_L])
            {
                if (vx < (WORLD_WIDTH - SCREEN_LARGE_STEP - SCR_W))
                {
                    vx += SCREEN_LARGE_STEP;
					off_vx += SCREEN_LARGE_STEP;
					off_vx += BUF_W;
					off_vx %= BUF_W;
					flush_screen(istream, ostream, vx + SCR_W - SCREEN_LARGE_STEP, vy, vx + SCR_W, vy + SCR_H);
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
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
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
            }
        }
        g_usleep(600);
    }
    return 0;
}
