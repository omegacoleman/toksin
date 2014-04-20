#include <glib.h>
#include <gio/gio.h>
#include <protocol.h>
#include <blocks.h>

typedef min_block_type block;
bool last_ping_ponged = true;
gint64 last_ping_time;
#define SCR_W 30
#define SCR_H 15
block screen[SCR_W * SCR_H];
unsigned int vx, vy;

void do_magic(GInputStream * istream, GOutputStream * ostream)
{
	g_output_stream_write(ostream, &MAGIC, sizeof(magic_code), NULL, NULL);
}

void ping(GInputStream * istream, GOutputStream * ostream)
{
	do_magic(istream, ostream);
	if (last_ping_ponged)
	{
		operation_code op = OPC_PING;
		last_ping_time = g_get_real_time();
		g_output_stream_write(ostream, &op, sizeof(operation_code), NULL, NULL);
	}
}

void flush_screen(GInputStream * istream, GOutputStream * ostream)
{
	do_magic(istream, ostream);
	operation_code op = OPC_GET_RANGE;
	g_output_stream_write(ostream, &op, sizeof(operation_code), NULL, NULL);
	op_get_range pend;
	pend.xa = vx;
	pend.ya = vy;
	pend.xb = vx + SCR_W;
	pend.yb = vy + SCR_H;
	g_output_stream_write(ostream, &pend, sizeof(op_get_range), NULL, NULL);
}

void end_connection(GInputStream * istream, GOutputStream * ostream)
{
	do_magic(istream, ostream);
	operation_code op = OPC_CLOSE;
	g_output_stream_write(ostream, &op, sizeof(operation_code), NULL, NULL);
}

void event_hooker(GInputStream * istream, GOutputStream * ostream)
{
	magic_code i_magic;
	g_input_stream_read(istream, &i_magic, sizeof(magic_code), NULL, NULL);
	g_assert(i_magic == MAGIC);
	operation_code rsp_opc;
	g_input_stream_read(istream, &rsp_opc, sizeof(operation_code), NULL, NULL);
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
			g_input_stream_read(istream, &rsp_struct, sizeof(rsp_set_block), NULL, NULL);
			unsigned int startp = (rsp_struct.starty - vy) * SCR_W + (rsp_struct.startx - vx);
			g_input_stream_read(istream, &(screen[startp]), sizeof(block) * rsp_struct.amount, NULL, NULL);
		}
		break;
	}
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
	g_print("Bye! \n");
}

#ifdef _MSC_VER

#include <windows.h>

BOOL CtrlHandler(DWORD fdwCtrlType) 
{
	shut();
	exit(EXIT_SUCCESS);
	return FALSE;
}

#endif

int main (int argc, char *argv[])
{
	g_type_init();
	GError * error = NULL;
	GSocketConnection * connection = NULL;
	GSocketClient * client = g_socket_client_new();
	connection = g_socket_client_connect_to_host (client,
												(gchar*)"localhost",
												1500,
												NULL,
												&error);
	if (error != NULL)
	{
		g_error (error->message);
	}
	istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
	ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
#ifdef _MSC_VER
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
#else
	atexit(&shut);
#endif
	vx = 20;
	vy = 0x8f - 7;
	flush_screen(istream, ostream);
	while (1)
	{
		event_hooker(istream, ostream);
		print_screen();
	}
	return 0;
}
