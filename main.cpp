#include <glib.h>
#include <gio/gio.h>
#include <protocol.h>

/* this function will get called everytime a client attempts to connect */
gboolean incoming_callback  (GSocketService *service,
                    GSocketConnection *connection,
                    GObject *source_object,
                    gpointer user_data)
{
  GError * error = NULL;
  GSocketAddress *addr = g_socket_connection_get_remote_address(connection, &error);
  char *s_addr = g_inet_address_to_string(
              g_inet_socket_address_get_address(G_INET_SOCKET_ADDRESS(addr))
              );
  if (error != NULL)
  {
      g_warning(error->message);
      return FALSE;
  }
  g_message("Connection established with client %s\n", s_addr);
  GInputStream * istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  GOutputStream * ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));

  while (1)
  {
      magic_code i_magic;
      g_input_stream_read  (istream,
                            &i_magic,
                            sizeof(magic_code),
                            NULL,
                            NULL);
      if (i_magic != MAGIC)
      {
          g_warning("Client %s sent unexpected magic number (%x).",
                    s_addr, i_magic);
          return 0;
      }

      operation_code oc;
      g_input_stream_read  (istream,
                            &oc,
                            sizeof(operation_code),
                            NULL,
                            NULL);
      switch(oc)
      {
      case OPC_GET_RANGE:
      {
          operation_code res = RSP_SET_BLOCK;
          g_output_stream_write  (ostream,
                                 &MAGIC,
                                 sizeof(magic_code),
                                 NULL,
                                 NULL);
          g_output_stream_write  (ostream,
                                 &res,
                                 sizeof(operation_code),
                                 NULL,
                                 NULL);
      }
          break;
      case OPC_PING:
      {
          operation_code res = RSP_PONG;
          g_output_stream_write  (ostream,
                                 &MAGIC,
                                 sizeof(magic_code),
                                 NULL,
                                 NULL);
          g_output_stream_write  (ostream,
                                 &res,
                                 sizeof(operation_code),
                                 NULL,
                                 NULL);
      }
          break;
      case OPC_CLOSE:
          g_message("Connection with client %s closed.\n", s_addr);
          return FALSE;
          break;
      default:
          g_warning("Client %s pend unsupported operation (%x).",
                    s_addr, oc);
          return FALSE;
          break;
      }
  }
}

int main (int argc, char **argv)
{
  g_type_init();
  GError * error = NULL;
  GSocketService * service = g_socket_service_new ();
  g_socket_listener_add_inet_port ((GSocketListener*)service, 1500, NULL, &error);
  if (error != NULL)
  {
      g_error (error->message);
  }
  g_signal_connect (service,
                    "incoming",
                    G_CALLBACK (incoming_callback),
                    NULL);
  g_socket_service_start (service);
  g_print ("Waiting for client!\n");
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  return 0;
}
