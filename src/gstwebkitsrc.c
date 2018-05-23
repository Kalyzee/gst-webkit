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
        "width=[1,MAX],height=[1,MAX]," "framerate=(fraction)[0/1,MAX]")
    );

#define gst_webkit_src_parent_class parent_class
G_DEFINE_TYPE (GstWebkitSrc, gst_webkit_src, GST_TYPE_BASE_SRC);

static void gst_webkit_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_webkit_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static guint gst_webkit_src_get_size (GstWebkitSrc * src);
static gboolean gst_webkit_src_start (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_stop (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_is_seekable (GstBaseSrc * basesrc);

static gboolean gst_webkit_src_event_handler (GstBaseSrc * src, GstEvent * event);
static void gst_webkit_src_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_webkit_src_create (GstBaseSrc * src, guint64 offset,
    guint length, GstBuffer ** buf);

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
  gstbase_src_class->create = GST_DEBUG_FUNCPTR (gst_webkit_src_create);

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

static gboolean gst_webkit_src_refresh(gpointer pointer){
  GstWebkitSrc *src;

  src = GST_WEBKIT_SRC (pointer);

  gsize size = gst_webkit_src_get_size(src);

  if (src->ready){

    GdkPixbuf* pixbuf = gtk_offscreen_window_get_pixbuf(src->window);

    if (src->current == src->data_1){

      memcpy(src->data_2, gdk_pixbuf_read_pixels(pixbuf), size);
      src->current = src->data_2;

    }else{
      memcpy(src->data_1, gdk_pixbuf_get_pixels(pixbuf), size);
      src->current = src->data_1;

    }
    g_object_unref(pixbuf);

  }


  return TRUE;
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

  gsize size =  gst_webkit_src_get_size(src);
  src->data_1 = malloc(size);
  src->data_2 = malloc(size);
  memset(src->data_1, 0, size);
  memset(src->data_2, 0, size);

  src->current = src->data_1;

  gtk_widget_show_all(src->window);

  g_timeout_add(200, gst_webkit_src_refresh, (gpointer)src);
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

  memcpy (data, src->current, size);



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

static guint
gst_webkit_src_get_size (GstWebkitSrc * src)
{
  return 1280*720*3;
}

static GstBuffer *
gst_webkit_src_create_buffer (GstWebkitSrc * src, gsize * bufsize)
{
  GstBuffer *buf;
  gsize size = gst_webkit_src_get_size (src);
  GstMapInfo info;

  *bufsize = size;
  buf = gst_webkit_src_alloc_buffer (src, size);


  return buf;
}

static void
gst_webkit_src_get_times (GstBaseSrc * basesrc, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstWebkitSrc *src;

  src = GST_WEBKIT_SRC (basesrc);
  *start = -1;
  *end = -1;

}

static GstFlowReturn
gst_webkit_src_create (GstBaseSrc * basesrc, guint64 offset, guint length,
    GstBuffer ** ret)
{
  GstWebkitSrc *src;
  GstBuffer *buf;
  GstClockTime time;
  gsize size;

  src = GST_WEBKIT_SRC (basesrc);

  buf = gst_webkit_src_create_buffer (src, &size);
  GST_BUFFER_OFFSET (buf) = offset;

  GstClock *clock;

  clock = gst_element_get_clock (GST_ELEMENT (src));

  if (clock) {
    time = gst_clock_get_time (clock);
    time -= gst_element_get_base_time (GST_ELEMENT (src));
    gst_object_unref (clock);
  } else {
    /* not an error not to have a clock */
    time = GST_CLOCK_TIME_NONE;
  }


  GST_BUFFER_DTS (buf) = time;
  GST_BUFFER_PTS (buf) = time;



  *ret = buf;
  return GST_FLOW_OK;
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
  free(src->data_1);
  free(src->data_2);

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
