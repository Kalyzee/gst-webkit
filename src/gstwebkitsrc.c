/*
 * Gstreamer Webkit Plugin
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2018 Kalyzee <ludovic.bouguerra@kalyzee.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-webkitsrc
 *
 * FIXME:Describe webkitsrc here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! webkitsrc ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include <cairo.h>

#include <stdlib.h>
#include "gstwebkitsrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_webkit_src_debug);
#define GST_CAT_DEFAULT gst_webkit_src_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_URL
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,"
        "format=(string){RGB},"
        "width=[1280],height=[720]," "framerate=(fraction)[25/1]")
    );

#define gst_webkit_src_parent_class parent_class
G_DEFINE_TYPE (GstWebkitSrc, gst_webkit_src, GST_TYPE_BASE_SRC);

static void gst_webkit_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_webkit_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_webkit_src_start (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_stop (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_is_seekable (GstBaseSrc * basesrc);

static gboolean gst_webkit_src_event_handler (GstBaseSrc * src, GstEvent * event);
static void gst_webkit_src_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_webkit_src_create (GstBaseSrc * src, guint64 offset,
    guint length, GstBuffer ** buf);

static GstFlowReturn gst_webkit_src_fill (GstPushSrc * psrc, GstBuffer * buffer);

/* GObject vmethod implementations */


static gboolean
gst_webkit_src_query (GstPad    *pad,
                 GstObject *parent,
                 GstQuery  *query);

/* initialize the webkitsrc's class */
static void
gst_webkit_src_class_init (GstWebkitSrcClass * klass)
{
  //

  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbase_src_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbase_src_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = gst_webkit_src_set_property;
  gobject_class->get_property = gst_webkit_src_get_property;

  g_object_class_install_property (gobject_class, PROP_URL,
      g_param_spec_string ("url", "URL", "url page",
          "", G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "WebkitSrc",
    "Html / css / js renderer element",
    "Html / css / js renderer element",
    "Ludovic Bouguerra <ludovic.bouguerra@kalyzee.com>");

  gst_element_class_add_pad_template (gstelement_class,
  gst_static_pad_template_get (&src_factory));

  gstbase_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_webkit_src_is_seekable);
  gstbase_src_class->start = GST_DEBUG_FUNCPTR (gst_webkit_src_start);
  gstbase_src_class->stop = GST_DEBUG_FUNCPTR (gst_webkit_src_stop);
  gstbase_src_class->event = GST_DEBUG_FUNCPTR (gst_webkit_src_event_handler);
  gstbase_src_class->get_times = GST_DEBUG_FUNCPTR (gst_webkit_src_get_times);
  gstpushsrc_class->fill = GST_DEBUG_FUNCPTR (gst_webkit_src_fill);

}



static GstFlowReturn
gst_webkit_src_fill (GstPushSrc * psrc, GstBuffer * buffer)
{
  GstWebkitSrc *src;
  GstClockTime next_time;
  GstVideoFrame frame;
  gconstpointer pal;
  gsize palsize;

  src = GST_WEBKIT_SRC (psrc);

  if (G_UNLIKELY (GST_VIDEO_INFO_FORMAT (&src->info) ==
          GST_VIDEO_FORMAT_UNKNOWN))
    goto not_negotiated;

  /* 0 framerate and we are at the second frame, eos */
  if (G_UNLIKELY (src->info.fps_n == 0 && src->n_frames == 1))
    goto eos;

  if (G_UNLIKELY (src->n_frames == -1)) {
    /* EOS for reverse playback */
    goto eos;
  }

  GST_LOG_OBJECT (src,
      "creating buffer from pool for frame %" G_GINT64_FORMAT, src->n_frames);

  if (!gst_video_frame_map (&frame, &src->info, buffer, GST_MAP_WRITE))
    goto invalid_frame;

  GST_BUFFER_PTS (buffer) =
      src->accum_rtime + src->timestamp_offset + src->running_time;
  GST_BUFFER_DTS (buffer) = GST_CLOCK_TIME_NONE;

  gst_object_sync_values (GST_OBJECT (psrc), GST_BUFFER_PTS (buffer));



  if (src->ready){

    GdkPixbuf* pixbuf = gtk_offscreen_window_get_pixbuf(src->window);
    guint8* data = gdk_pixbuf_read_pixels(pixbuf);
    memcpy (GST_VIDEO_FRAME_PLANE_DATA (&frame, 1), 0, palsize);

    g_object_unref(pixbuf);

  }

  if ((pal = gst_video_format_get_palette (GST_VIDEO_FRAME_FORMAT (&frame),
              &palsize))) {
    memcpy (GST_VIDEO_FRAME_PLANE_DATA (&frame, 1), pal, palsize);
  }

  gst_video_frame_unmap (&frame);

  GST_DEBUG_OBJECT (src, "Timestamp: %" GST_TIME_FORMAT " = accumulated %"
      GST_TIME_FORMAT " + offset: %"
      GST_TIME_FORMAT " + running time: %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_PTS (buffer)), GST_TIME_ARGS (src->accum_rtime),
      GST_TIME_ARGS (src->timestamp_offset), GST_TIME_ARGS (src->running_time));

  GST_BUFFER_OFFSET (buffer) = src->accum_frames + src->n_frames;
  if (src->reverse) {
    src->n_frames--;
  } else {
    src->n_frames++;
  }
  GST_BUFFER_OFFSET_END (buffer) = GST_BUFFER_OFFSET (buffer) + 1;
  if (src->info.fps_n) {
    next_time = gst_util_uint64_scale (src->n_frames,
        src->info.fps_d * GST_SECOND, src->info.fps_n);
    if (src->reverse) {
      GST_BUFFER_DURATION (buffer) = src->running_time - next_time;
    } else {
      GST_BUFFER_DURATION (buffer) = next_time - src->running_time;
    }
  } else {
    next_time = src->timestamp_offset;
    /* NONE means forever */
    GST_BUFFER_DURATION (buffer) = GST_CLOCK_TIME_NONE;
  }

  src->running_time = next_time;

  return GST_FLOW_OK;

not_negotiated:
  {
    return GST_FLOW_NOT_NEGOTIATED;
  }
eos:
  {
    GST_DEBUG_OBJECT (src, "eos: 0 framerate, frame %d", (gint) src->n_frames);
    return GST_FLOW_EOS;
  }
invalid_frame:
  {
    GST_DEBUG_OBJECT (src, "invalid frame");
    return GST_FLOW_OK;
  }
}


static gboolean
gst_webkit_src_event_handler (GstBaseSrc * basesrc, GstEvent * event)
{
  GstWebkitSrc *src;

  src = GST_WEBKIT_SRC (basesrc);
  return GST_BASE_SRC_CLASS (parent_class)->event (basesrc, event);
}


static void gst_webkit_src_load_changed (WebKitWebView  *web_view,
                                   WebKitLoadEvent load_event,
                                   gpointer src)
{
    GST_DEBUG ("Webpage state changed");
    if (load_event == WEBKIT_LOAD_FINISHED){
      GST_DEBUG ("Webpage is now ready");
      GST_WEBKIT_SRC(src)->ready = TRUE;
    }
}



/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_webkit_src_init (GstWebkitSrc * src)
{

  GST_DEBUG ("gtk init");
  gtk_init(NULL, NULL);
  src->ready = FALSE;

  GST_DEBUG ("init webview");
  src->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

  GST_DEBUG ("register load-changed callback");
  g_signal_connect(src->web_view, "load-changed", G_CALLBACK(gst_webkit_src_load_changed), (gpointer) src);

  GST_DEBUG ("Setting base SRC attributes Live / FORMAT");
  gst_base_src_set_live((GstBaseSrc *) src, TRUE);
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);

  GST_DEBUG ("Initing GTK offscreen window");
  src->window = gtk_offscreen_window_new ();
  gtk_window_set_default_size(GTK_WINDOW(src->window), 1280, 720);
  gtk_container_add(GTK_CONTAINER(src->window), GTK_WIDGET(src->web_view));

  WebKitSettings *settings = webkit_settings_new ();
  webkit_settings_set_auto_load_images(settings, TRUE);
  webkit_settings_set_enable_javascript(settings, TRUE);
  webkit_settings_set_enable_webgl(settings, TRUE);
  webkit_settings_set_allow_modal_dialogs(settings, FALSE);
  webkit_settings_set_javascript_can_access_clipboard(settings, FALSE);
  webkit_settings_set_enable_page_cache(settings, FALSE);
  webkit_settings_set_enable_accelerated_2d_canvas(settings, TRUE);
  webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
  webkit_settings_set_enable_plugins (settings, TRUE);
  webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
  webkit_web_view_set_settings (WEBKIT_WEB_VIEW(src->web_view), settings);


  gtk_widget_show_all(src->window);

  GST_DEBUG ("End initing gtk offscreen");

}


static gboolean gst_webkit_go_to_url_cb(gpointer object){
  GstWebkitSrc *src = GST_WEBKIT_SRC (object);
  webkit_web_view_load_uri(src->web_view, "http://www.google.com");
  return FALSE;
}

static void
gst_webkit_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstWebkitSrc *src = GST_WEBKIT_SRC (object);

  switch (prop_id) {
    case PROP_URL:
      src->url = g_value_get_string (value);
      g_idle_add(gst_webkit_go_to_url_cb, (gpointer) object);
      //webkit_web_view_load_uri(src->web_view, src->url);

      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_webkit_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstWebkitSrc *filter = GST_WEBKIT_SRC (object);

  switch (prop_id) {
    case PROP_URL:
      g_value_set_string (value, filter->url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
webkitsrc_init (GstPlugin * webkitsrc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template webkitsrc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_webkit_src_debug, "webkitsrc",
      0, "Webkit SRC debugger init");

  return gst_element_register (webkitsrc, "webkitsrc", GST_RANK_NONE,
      GST_TYPE_WEBKIT_SRC);
}




static void
gst_webkit_src_prepare_buffer (GstWebkitSrc * src, guint8 * data, gsize size)
{
  if (size == 0)
    return;

  g_print("BUFFER %d", size);
  memcpy (data, 0, size);



}

static GstBuffer *
gst_webkit_src_alloc_buffer (GstWebkitSrc * src, guint size)
{
  GstBuffer *buf;
  gpointer data;

  buf = gst_buffer_new ();

  if (size != 0) {
    data = g_malloc (size);

    gst_webkit_src_prepare_buffer (src, data, size);

    gst_buffer_append_memory (buf,
        gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));
  }

  return buf;
}



static void
gst_webkit_src_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstClockTime timestamp = GST_BUFFER_PTS (buffer);

  if (GST_CLOCK_TIME_IS_VALID (timestamp)) {
    /* get duration to calculate end time */
    GstClockTime duration = GST_BUFFER_DURATION (buffer);

    if (GST_CLOCK_TIME_IS_VALID (duration)) {
      *end = timestamp + duration;
    }
    *start = timestamp;
  }
}


static gboolean
gst_webkit_src_start (GstBaseSrc * basesrc)
{
  GstWebkitSrc *src;

  src = GST_WEBKIT_SRC (basesrc);

  return TRUE;
}

static gboolean
gst_webkit_src_stop (GstBaseSrc * basesrc)
{
  GstWebkitSrc *src;

  src = GST_WEBKIT_SRC (basesrc);

  GST_OBJECT_LOCK (src);
  if (src->parent) {
    gst_buffer_unref (src->parent);
    src->parent = NULL;
  }


  g_object_unref(src->window);
  GST_OBJECT_UNLOCK (src);

  return TRUE;
}

static gboolean
gst_webkit_src_is_seekable (GstBaseSrc * basesrc)
{
  return FALSE;
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "webkitsrc"
#endif

/* gstreamer looks for this structure to register webkitsrcs
 *
 * exchange the string 'Template webkitsrc' with your webkitsrc description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    webkitsrc,
    "webkitsrc",
    webkitsrc_init,
    VERSION,
    "LGPL",
    "Webkit",
    "http://www.kalyzee.com/"
)
