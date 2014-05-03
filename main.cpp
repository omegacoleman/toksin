#include <glib.h>
#include <gio/gio.h>
#include "protocol.hpp"
#include "toksin.hpp"

gint64 last_modify_time;

typedef struct _callback_data_magic
{
    GOutputStream * ostream;
    magic_code *i_magic;
    char *s_addr;
    gboolean *alive;
} callback_data_magic;

typedef struct _callback_check_need_flush
{
    GInputStream * istream;
    GOutputStream * ostream;
    gint64 *last_flush;
    char *s_addr;
    gboolean *alive;
} callback_check_need_flush;

void start_respond_to_opcode(GInputStream *istream, callback_data_magic *cbdm);
void respond_to_opcode(GInputStream* istream, GAsyncResult* result, callback_data_magic* callback_data);
gboolean check_need_flush(callback_check_need_flush *callback);

typedef struct _flush_source
{
    GSource source;
    callback_check_need_flush *cbcnf;
} flush_source;

typedef struct _connection_grave_source
{
    GSource source;
	gboolean *alive;
    GSocketConnection *connection;
    gint64 *last_flush;
    char *s_addr;
	magic_code* i_magic;
	flush_source *cbcnf_source;
} connection_grave_source;

static gboolean prepare_need_flush(GSource *source, gint *timeout)
{
    *timeout = 50;
    return FALSE;
}

static gboolean check_need_flush(GSource *source)
{
    flush_source *mysource = (flush_source *)source;
    return (*(mysource->cbcnf->last_flush) < last_modify_time);
}

static gboolean dispatch_need_flush(GSource *source, GSourceFunc callback, gpointer user_data)
{
	GError* error = NULL;
    flush_source *mysource = (flush_source *)source;
    gsize bytes_transferred;
    operation_code res = RSP_FLUSH_REQUEST;
    g_output_stream_write_all(mysource->cbcnf->ostream, &MAGIC, sizeof(magic_code), &bytes_transferred, NULL, &error);
	if (error != NULL)
	{
		g_message("Error sending RSP_FLUSH_REQUEST to client(1) %s:%s", mysource->cbcnf->s_addr, error->message);
        *(mysource->cbcnf->alive) = FALSE;
		g_clear_error(&error);
		return FALSE;
	}
    g_output_stream_write_all(mysource->cbcnf->ostream, &res, sizeof(operation_code), &bytes_transferred, NULL, &error);
	if (error != NULL)
	{
		g_message("Error sending RSP_FLUSH_REQUEST to client(2) %s:%s", mysource->cbcnf->s_addr, error->message);
        *(mysource->cbcnf->alive) = FALSE;
		g_clear_error(&error);
		return FALSE;
	}
    *(mysource->cbcnf->last_flush) = g_get_real_time();
    return TRUE;
}

static gboolean prepare_connection_grave(GSource *source, gint *timeout)
{
    *timeout = 50;
    return FALSE;
}

static gboolean check_connection_grave(GSource *source)
{
    connection_grave_source *mysource = (connection_grave_source *)source;
    return !(mysource->alive);
}

static gboolean dispatch_connection_grave(GSource *source, GSourceFunc callback, gpointer user_data)
{
    connection_grave_source *mysource = (connection_grave_source *)source;
	delete mysource->alive;
	g_object_unref(mysource->connection);
	delete mysource->i_magic;
	delete mysource->last_flush;
	delete mysource->s_addr;
	g_source_destroy((GSource *)mysource->cbcnf_source);
	return FALSE;
}

/* this function will get called everytime a client attempts to connect */
gboolean incoming_callback  (GSocketService *service,
                             GSocketConnection *connection,
                             GObject *source_object,
                             gpointer user_data)
{
	g_object_ref(connection);
    gint64 *last_flush = new gint64(g_get_real_time());
    gboolean* alive = new gboolean(true);
    magic_code* i_magic = new magic_code(0);
    GError* error = NULL;
    GSocketAddress *addr = g_socket_connection_get_remote_address(connection, &error);
	if (error != NULL)
	{
		g_message("Error fetching client addr :%s", error->message);
		g_clear_error(&error);
		return FALSE;
	}
    char *s_addr = g_inet_address_to_string(g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(addr)));
    g_message("Connection established with client %s", s_addr);
    GInputStream * istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
    GOutputStream * ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    callback_data_magic *cbdm = new callback_data_magic;
    cbdm->i_magic = i_magic;
    cbdm->ostream = ostream;
    cbdm->s_addr = s_addr;
    cbdm->alive = alive;
    start_respond_to_opcode(istream, cbdm);
    callback_check_need_flush *cbcnf = new callback_check_need_flush;
    cbcnf->istream = istream;
    cbcnf->ostream = ostream;
    cbcnf->last_flush = last_flush;
    cbcnf->alive = alive;
    cbcnf->s_addr = s_addr;
    GSourceFuncs *cbcnf_source_funcs = new GSourceFuncs();
	cbcnf_source_funcs->prepare = prepare_need_flush;
	cbcnf_source_funcs->check = check_need_flush;
	cbcnf_source_funcs->dispatch = dispatch_need_flush;
	cbcnf_source_funcs->finalize = NULL;
    flush_source *cbcnf_source = (flush_source *)g_source_new(cbcnf_source_funcs, sizeof(flush_source));
    cbcnf_source->cbcnf = cbcnf;
    g_source_attach((GSource *)cbcnf_source, NULL);
	
    GSourceFuncs *cgs_funcs = new GSourceFuncs();
	cgs_funcs->prepare = prepare_connection_grave;
	cgs_funcs->check = check_connection_grave;
	cgs_funcs->dispatch = dispatch_connection_grave;
	cgs_funcs->finalize = NULL;
    connection_grave_source *cgs = (connection_grave_source *)g_source_new(cgs_funcs, sizeof(connection_grave_source));
	cgs->connection = connection;
    cgs->alive = alive;
	cgs->cbcnf_source = cbcnf_source;
	cgs->i_magic = i_magic;
	cgs->last_flush = last_flush;
	cgs->s_addr = s_addr;
    return FALSE;
}

void start_respond_to_opcode(GInputStream *istream, callback_data_magic *cbdm)
{
    g_input_stream_read_async  (istream, cbdm->i_magic, sizeof(magic_code), G_PRIORITY_DEFAULT,  NULL, (GAsyncReadyCallback )respond_to_opcode, cbdm);
}

void respond_to_opcode(GInputStream* istream, GAsyncResult* result, callback_data_magic* callback_data)
{
	GError* error = NULL;
    g_input_stream_read_finish(istream, result, &error);
	if (error != NULL)
	{
		g_message("Error reading magic number on client %s: %s", callback_data->s_addr, error->message);
        *(callback_data->alive) = FALSE;
		g_clear_error(&error);
		return;
	}
    if (*(callback_data->i_magic) != MAGIC)
	{
		g_message("Client %s sent unrecognized magic number. Connection closing.", callback_data->s_addr);
        *(callback_data->alive) = FALSE;
		return;
	}
    GOutputStream *ostream = callback_data->ostream;

    gsize bytes_transferred;
    operation_code oc;
    g_input_stream_read_all  (istream, &oc, sizeof(operation_code), &bytes_transferred, NULL, &error);
	if (error != NULL)
	{
		g_message("Error reading operation code on client %s: %s", callback_data->s_addr, error->message);
        *(callback_data->alive) = FALSE;
		g_clear_error(&error);
		return;
	}
    switch(oc)
    {
    case OPC_GET_RANGE:
    {
        operation_code res = RSP_SET_BLOCK;
        op_get_range get_range;
        g_input_stream_read_all  (istream, &get_range, sizeof(op_get_range), &bytes_transferred, NULL, &error);
		if (error != NULL)
		{
			g_message("Error reading OPC_GET_RANGE struct from client %s: %s", callback_data->s_addr, error->message);
			*(callback_data->alive) = FALSE;
			g_clear_error(&error);
			return;
		}
        rsp_set_block set_block;
        set_block.startx = get_range.xa;
        set_block.starty = get_range.ya;
        set_block.amount = get_range.xb - get_range.xa;
		if ((get_range.xa > get_range.xb) || (get_range.ya > get_range.yb) || (get_range.xb >= WORLD_WIDTH) || (get_range.yb >= WORLD_HEIGHT))
		{
			g_message("Client %s tried to break the world limit, connection closing.", callback_data->s_addr);
			*(callback_data->alive) = FALSE;
			return;
		}
        for(; set_block.starty < get_range.yb; set_block.starty++)
        {
            g_output_stream_write_all  (ostream, &MAGIC, sizeof(magic_code), &bytes_transferred, NULL, &error);
			if (error != NULL)
			{
				g_message("Error sending blocks to client(1) %s: %s", callback_data->s_addr, error->message);
				*(callback_data->alive) = FALSE;
				g_clear_error(&error);
				return;
			}
            g_output_stream_write_all  (ostream, &res, sizeof(operation_code), &bytes_transferred, NULL, &error);
			if (error != NULL)
			{
				g_message("Error sending blocks to client(2) %s: %s", callback_data->s_addr, error->message);
				*(callback_data->alive) = FALSE;
				g_clear_error(&error);
				return;
			}
            g_output_stream_write_all  (ostream, &set_block, sizeof(rsp_set_block), &bytes_transferred, NULL, &error);
			if (error != NULL)
			{
				g_message("Error sending blocks to client(3) %s: %s", callback_data->s_addr, error->message);
				*(callback_data->alive) = FALSE;
				g_clear_error(&error);
				return;
			}
            g_output_stream_write_all  (ostream, &(c_world.solids[set_block.starty * WORLD_WIDTH + set_block.startx]), sizeof(block) * set_block.amount, &bytes_transferred, NULL, &error);
			if (error != NULL)
			{
				g_message("Error sending blocks to client(4) %s: %s", callback_data->s_addr, error->message);
				*(callback_data->alive) = FALSE;
				g_clear_error(&error);
				return;
			}
        }
    }
    break;
    case OPC_DIG:
    {
        op_dig dig;
        g_input_stream_read_all  (istream,&dig,sizeof(op_dig), &bytes_transferred,NULL,&error);
		if (error != NULL)
		{
			g_message("Error reading OPC_DIG struct from client %s: %s", callback_data->s_addr, error->message);
			*(callback_data->alive) = FALSE;
			g_clear_error(&error);
			return;
		}
		if ((dig.xa >= WORLD_WIDTH) || (dig.ya >= WORLD_HEIGHT))
		{
			g_message("Client %s tried to break the world limit, connection closed.", callback_data->s_addr);
			*(callback_data->alive) = FALSE;
			return;
		}
        last_modify_time = g_get_real_time();
        c_world.solids[dig.ya * WORLD_WIDTH + dig.xa] = BLCK_AIR;
    }
    break;
    case OPC_PLACE:
    {
        op_place placement;
        g_input_stream_read_all  (istream,&placement,sizeof(op_place), &bytes_transferred,NULL,&error);
		if (error != NULL)
		{
			g_message("Error reading OPC_PLACE struct from client %s: %s", callback_data->s_addr, error->message);
			*(callback_data->alive) = FALSE;
			g_clear_error(&error);
			return;
		}
		if ((placement.xa >= WORLD_WIDTH) || (placement.ya >= WORLD_HEIGHT))
		{
			g_message("Client %s tried to break the world limit, connection closed.", callback_data->s_addr);
			*(callback_data->alive) = FALSE;
			return;
		}
        last_modify_time = g_get_real_time();
		c_world.solids[placement.ya * WORLD_WIDTH + placement.xa] = BLCK_DIRT;
    }
    break;
    case OPC_PING:
    {
        operation_code res = RSP_PONG;
        g_output_stream_write_all  (ostream,&MAGIC,sizeof(magic_code),&bytes_transferred,NULL,&error);
		if (error != NULL)
		{
			g_message("Error respond RSP_PONG to client(1) %s: %s", callback_data->s_addr, error->message);
			*(callback_data->alive) = FALSE;
			g_clear_error(&error);
			return;
		}
        g_output_stream_write_all  (ostream,&res,sizeof(operation_code),&bytes_transferred,NULL, &error);
		if (error != NULL)
		{
			g_message("Error respond RSP_PONG to client(2) %s: %s", callback_data->s_addr, error->message);
			*(callback_data->alive) = FALSE;
			g_clear_error(&error);
			return;
		}
    }
    break;
    case OPC_CLOSE:
        g_message("Connection with client %s closed.", callback_data->s_addr);
        *(callback_data->alive) = FALSE;
        return;
    default:
        g_warning("Client %s pend unsupported operation (%x). Connection closing",
                  callback_data->s_addr, oc);
        *(callback_data->alive) = FALSE;
        return;
        break;
    }
    start_respond_to_opcode(istream, callback_data);
}

gboolean callback_backup(gpointer data)
{
	dump_world_to_file();
	return TRUE;
}

int main (int argc, char **argv)
{
    g_type_init();
    try_restore_world();
    GError* error = NULL;
    GSocketService * service = g_socket_service_new();
    g_socket_listener_add_inet_port((GSocketListener*)service, 1500, NULL, &error);
    if (error != NULL)
    {
        g_error ("Error starting server : %s", error->message);
    }
    g_signal_connect(service, "incoming", G_CALLBACK (incoming_callback), NULL);
    g_socket_service_start(service);
    g_message("Waiting for client!");
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    last_modify_time = g_get_real_time();
	g_timeout_add_seconds(120, callback_backup, NULL);
	atexit(dump_world_to_file);
    g_main_loop_run(loop);
    return 0;
}
