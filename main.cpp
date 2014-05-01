#include <glib.h>
#include <gio/gio.h>
#include <protocol.h>
#include <toksin.h>

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
} callback_check_need_flush;

void start_respond_to_opcode(GInputStream *istream, callback_data_magic *cbdm);
void respond_to_opcode(GInputStream* istream, GAsyncResult* result, callback_data_magic* callback_data);
gboolean check_need_flush(callback_check_need_flush *callback);

typedef struct _flush_source
{
    GSource source;
    callback_check_need_flush *cbcnf;
} flush_source;

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
    flush_source *mysource = (flush_source *)source;
    gsize bytes_transferred;
    operation_code res = RSP_FLUSH_REQUEST;
    g_output_stream_write_all(mysource->cbcnf->ostream, &MAGIC, sizeof(magic_code), &bytes_transferred, NULL, NULL);
    g_output_stream_write_all(mysource->cbcnf->ostream, &res, sizeof(operation_code), &bytes_transferred, NULL, NULL);
    *(mysource->cbcnf->last_flush) = g_get_real_time();
    return TRUE;
}

/* this function will get called everytime a client attempts to connect */
gboolean incoming_callback  (GSocketService *service,
                             GSocketConnection *connection,
                             GObject *source_object,
                             gpointer user_data)
{
    gint64 last_flush = g_get_real_time();
    gboolean alive = true;
    magic_code i_magic = 0;
    GError * error = NULL;
    GSocketAddress *addr = g_socket_connection_get_remote_address(connection, &error);
    char *s_addr = g_inet_address_to_string(g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(addr)));
    if (error != NULL)
    {
        g_warning(error->message);
        return FALSE;
    }
    g_message("Connection established with client %s\n", s_addr);
    GInputStream * istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
    GOutputStream * ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    callback_data_magic cbdm;
    cbdm.i_magic = &i_magic;
    cbdm.ostream = ostream;
    cbdm.s_addr = s_addr;
    cbdm.alive = &alive;
    start_respond_to_opcode(istream, &cbdm);
    callback_check_need_flush cbcnf;
    cbcnf.istream = istream;
    cbcnf.ostream = ostream;
    cbcnf.last_flush = &last_flush;
    GSourceFuncs cbcnf_source_funcs = {prepare_need_flush, check_need_flush, dispatch_need_flush, NULL};
    flush_source *cbcnf_source = (flush_source *)g_source_new(&cbcnf_source_funcs, sizeof(flush_source));
    cbcnf_source->cbcnf = &cbcnf;
    g_source_attach((GSource *)cbcnf_source, NULL);
    while (alive)
    {
        while(g_main_context_iteration(NULL, TRUE));
    }
    return FALSE;
}

void start_respond_to_opcode(GInputStream *istream, callback_data_magic *cbdm)
{
    g_input_stream_read_async  (istream, cbdm->i_magic, sizeof(magic_code), G_PRIORITY_DEFAULT,  NULL, (GAsyncReadyCallback )respond_to_opcode, cbdm);
}

void respond_to_opcode(GInputStream* istream, GAsyncResult* result, callback_data_magic* callback_data)
{
    g_input_stream_read_finish(istream, result, NULL);
    g_assert(*(callback_data->i_magic) == MAGIC);
    GOutputStream *ostream = callback_data->ostream;

    gsize bytes_transferred;
    operation_code oc;
    g_input_stream_read_all  (istream, &oc, sizeof(operation_code), &bytes_transferred, NULL, NULL);
    switch(oc)
    {
    case OPC_GET_RANGE:
    {
        operation_code res = RSP_SET_BLOCK;
        op_get_range get_range;
        g_input_stream_read_all  (istream, &get_range, sizeof(op_get_range), &bytes_transferred, NULL, NULL);
        rsp_set_block set_block;
        set_block.startx = get_range.xa;
        set_block.starty = get_range.ya;
        set_block.amount = get_range.xb - get_range.xa;
        for(; set_block.starty < get_range.yb; set_block.starty++)
        {
            g_output_stream_write_all  (ostream, &MAGIC, sizeof(magic_code), &bytes_transferred, NULL, NULL);
            g_output_stream_write_all  (ostream, &res, sizeof(operation_code), &bytes_transferred, NULL, NULL);
            g_output_stream_write_all  (ostream, &set_block, sizeof(rsp_set_block), &bytes_transferred, NULL, NULL);
            g_output_stream_write_all  (ostream, &(c_world.solids[set_block.starty * WORLD_WIDTH + set_block.startx]), sizeof(block) * set_block.amount, &bytes_transferred, NULL, NULL);
        }
    }
    break;
    case OPC_DIG:
    {
        op_dig dig;
        g_input_stream_read_all  (istream,&dig,sizeof(op_dig), &bytes_transferred,NULL,NULL);
        last_modify_time = g_get_real_time();
        c_world.solids[dig.ya * WORLD_WIDTH + dig.xa] = BLCK_AIR;
    }
    break;
    case OPC_PING:
    {
        operation_code res = RSP_PONG;
        g_output_stream_write_all  (ostream,&MAGIC,sizeof(magic_code),&bytes_transferred,NULL,NULL);
        g_output_stream_write_all  (ostream,&res,sizeof(operation_code),&bytes_transferred,NULL, NULL);
    }
    break;
    case OPC_CLOSE:
        g_message("Connection with client %s closed.\n", callback_data->s_addr);
        callback_data->alive = FALSE;
        return;
    default:
        g_warning("Client %s pend unsupported operation (%x).",
                  callback_data->s_addr, oc);
        callback_data->alive = FALSE;
        return;
        break;
    }
    start_respond_to_opcode(istream, callback_data);
}

int main (int argc, char **argv)
{
    g_type_init();
    init_world();
    GError * error = NULL;
    GSocketService * service = g_threaded_socket_service_new(-1);
    g_socket_listener_add_inet_port((GSocketListener*)service, 1500, NULL, &error);
    if (error != NULL)
    {
        g_error (error->message);
    }
    g_signal_connect(service, "run", G_CALLBACK (incoming_callback), NULL);
    g_socket_service_start(service);
    g_print("Waiting for client!\n");
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    last_modify_time = g_get_real_time();
    g_main_loop_run(loop);
    return 0;
}
