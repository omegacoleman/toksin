#include <glib.h>
#include <gio/gio.h>
#include <protocol.h>

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
  GInputStream * istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
  GOutputStream * ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
  g_output_stream_write  (ostream,
                          &MAGIC,
                          sizeof(magic_code),
                          NULL,
                          &error);
  if (error != NULL)
  {
      g_error (error->message);
  }

  operation_code op = OPC_PING;
  g_output_stream_write  (ostream,
                          &op,
                          sizeof(operation_code),
                          NULL,
                          &error);
  if (error != NULL)
  {
      g_error (error->message);
  }
  gint64 tping = g_get_real_time();

  g_output_stream_write  (ostream,
                          &MAGIC,
                          sizeof(magic_code),
                          NULL,
                          &error);
  if (error != NULL)
  {
      g_error (error->message);
  }



  magic_code i_magic;
  g_input_stream_read  (istream,
                        &i_magic,
                        sizeof(magic_code),
                        NULL,
                        NULL);
  g_message("Recieved magic number : %x", i_magic);
  operation_code pong;
  g_input_stream_read  (istream,
                        &pong,
                        sizeof(operation_code),
                        NULL,
                        NULL);
  g_message("Recieved pong : %x", pong);
  gint64 tpong = g_get_real_time();
  g_message("Your ping : %d", tpong - tping);

  op = OPC_CLOSE;
  g_output_stream_write  (ostream,
                          &op,
                          sizeof(operation_code),
                          NULL,
                          &error);
  if (error != NULL)
  {
      g_error (error->message);
  }
  return 0;
}
