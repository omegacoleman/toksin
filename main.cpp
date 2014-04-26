#include <glib.h>
#include <gio/gio.h>
#include <protocol.h>
#include <toksin.h>

/* this function will get called everytime a client attempts to connect */
gboolean incoming_callback  (GSocketService *service,
                    GSocketConnection *connection,
                    GObject *source_object,
                    gpointer user_data)
{
  gsize bytes_transferred;
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
      g_input_stream_read_all  (istream,
                            &i_magic,
                            sizeof(magic_code),
                            &bytes_transferred, 
                            NULL,
                            NULL);
      if (i_magic != MAGIC)
      {
          g_warning("Client %s sent unexpected magic number (%x).",
                    s_addr, i_magic);
          return 0;
      }

      operation_code oc;
      g_input_stream_read_all  (istream,
                            &oc,
                            sizeof(operation_code),
                            &bytes_transferred, 
                            NULL,
                            NULL);
      switch(oc)
      {
      case OPC_GET_RANGE:
      {
          operation_code res = RSP_SET_BLOCK;
		  op_get_range get_range;
		  g_input_stream_read_all  (istream,
								&get_range,
								sizeof(op_get_range),
                     &bytes_transferred, 
								NULL,
								NULL);
		  rsp_set_block set_block;
		  set_block.startx = get_range.xa;
		  set_block.starty = get_range.ya;
		  set_block.amount = get_range.xb - get_range.xa;
		  for(; set_block.starty < get_range.yb; set_block.starty++)
		  {
			  g_output_stream_write_all  (ostream,
									 &MAGIC,
									 sizeof(magic_code),
                         &bytes_transferred, 
									 NULL,
									 NULL);
			  g_output_stream_write_all  (ostream,
									 &res,
									 sizeof(operation_code),
                         &bytes_transferred, 
									 NULL,
									 NULL);
			  g_output_stream_write_all  (ostream,
									 &set_block,
									 sizeof(rsp_set_block),
                         &bytes_transferred, 
									 NULL,
									 NULL);
			  g_output_stream_write_all  (ostream,
				  &(c_world.solids[set_block.starty * WORLD_WIDTH + set_block.startx]),
				  sizeof(block) * set_block.amount,
             &bytes_transferred, 
				  NULL,
				  NULL);
		  }
      }
          break;
      case OPC_PING:
      {
          operation_code res = RSP_PONG;
          g_output_stream_write_all  (ostream,
                                 &MAGIC,
                                 sizeof(magic_code),
                                 &bytes_transferred, 
                                 NULL,
                                 NULL);
          g_output_stream_write_all  (ostream,
                                 &res,
                                 sizeof(operation_code),
                                 &bytes_transferred, 
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
  init_world();
  GError * error = NULL;
  GSocketService * service = g_threaded_socket_service_new (-1);
  g_socket_listener_add_inet_port ((GSocketListener*)service, 1500, NULL, &error);
  if (error != NULL)
  {
      g_error (error->message);
  }
  g_signal_connect (service,
                    "run",
                    G_CALLBACK (incoming_callback),
                    NULL);
  g_socket_service_start (service);
  g_print ("Waiting for client!\n");
  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  return 0;
}
