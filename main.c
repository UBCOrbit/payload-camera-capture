#include <gst/gst.h>
#include <glib.h>

/*
 * Handle gstreamer bus messages
 */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

int main(int argc, char *argv[])
{
  GMainLoop *loop;

  GstElement *pipeline;
  GstElement *source;
  GstElement *capsfilter;
  GstElement *fileenc;
  GstElement *sink;

  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);

  /* Check Input Arguments */
  if(argc != 2) {
    g_printerr("Usage: %s <Output filename>\n", argv[0]);
    return -1;
  }

  /* Create Elements */
  g_print("Creating Elements");

  pipeline    = gst_pipeline_new ("save-image");
  source      = gst_element_factory_make ("nvcamerasrc", "camera-source");
  capsfilter  = gst_element_factory_make ("capsfilter", NULL);
  fileenc     = gst_element_factory_make ("nvjpegenc", "jpeg-encode");
  sink        = gst_element_factory_make ("filesink", "file-sink");

  /* Check Elements Created */
  if(!pipeline || !source || !capsfilter || !fileenc || !sink)
  {
    g_printerr ("One element could not be created. Exciting.\n");
    return -1;
  }

  /* Configure Elements */
  g_print("Configuring Elements");

  g_object_set(G_OBJECT(source), "num-buffers", 1, NULL);
  g_object_set(G_OBJECT(capsfilter), "caps", gst_caps_from_string("video/x-raw(memory:NVMM), width=(int)1920, height=(int)1080, format=(string)I420, framerate=(fraction)5/1"), NULL);
  g_object_set(G_OBJECT(sink), "location", argv[1], NULL);

  /* Add Message Handler */
  g_print("Adding Message Handler");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* Add Elements to Bin" */
  g_print("Adding Elements to Bin");

  gst_bin_add_many(GST_BIN(pipeline), source, capsfilter, fileenc, sink, NULL);

  /* Link Elements */
  g_print("Linking Elements");

  gst_element_link_many(source, capsfilter, fileenc, sink, NULL);

  /* Play Pipeline */
  g_print("Playing Pipeline");

  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);

  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}

