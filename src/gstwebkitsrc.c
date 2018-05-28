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
        "format=(string){RGBA},"
        "width=(int){1280},height=(int){720}," "framerate=(fraction){25/1}")
    );

#define gst_webkit_src_parent_class parent_class
G_DEFINE_TYPE (GstWebkitSrc, gst_webkit_src, GST_TYPE_PUSH_SRC);

static void gst_webkit_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_webkit_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_webkit_src_start (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_stop (GstBaseSrc * basesrc);
static gboolean gst_webkit_src_is_seekable (GstBaseSrc * basesrc);


static GstFlowReturn gst_webkit_src_fill (GstPushSrc * psrc, GstBuffer * buffer);

static gboolean
gst_webkit_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps);

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
  GstPushSrcClass *gstpush_src_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbase_src_class = (GstBaseSrcClass *) klass;
  gstpush_src_class = (GstPushSrcClass *) klass;

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
  gstpush_src_class->fill = GST_DEBUG_FUNCPTR (gst_webkit_src_fill);
  gstbase_src_class->set_caps = GST_DEBUG_FUNCPTR(gst_webkit_src_setcaps);
}

static gboolean
gst_webkit_src_setcaps (GstBaseSrc * bsrc, GstCaps * caps)
{

  GST_LOG ("*****Entering set caps*****");
  const GstStructure *structure;
  structure = gst_caps_get_structure (caps, 0);
  GstWebkitSrc *src = GST_WEBKIT_SRC (bsrc);
  GstVideoInfo info;


  if (gst_structure_has_name (structure, "video/x-raw")) {
    /* we can use the parsing code */
    if (!gst_video_info_from_caps (&info, caps))
      goto parse_failed;

  }else{
    goto parse_failed;
  }

  src->info = info;

  return TRUE;


parse_failed:
  return FALSE;

}


static GstFlowReturn
gst_webkit_src_fill (GstPushSrc * psrc, GstBuffer * buffer)
{
  GstWebkitSrc *src;
  GstVideoFrame frame;
  GstClockTime next_time;

  //GST_DEBUG ("Entering fill method");

  src = GST_WEBKIT_SRC (psrc);


  //GST_DEBUG ("GET SRC");
  if (G_UNLIKELY (GST_VIDEO_INFO_FORMAT (&src->info) ==
          GST_VIDEO_FORMAT_UNKNOWN))
    goto not_negotiated;

  //GST_DEBUG ("BEFORE MAP");

  if (!gst_video_frame_map (&frame, &src->info, buffer, GST_MAP_WRITE))
    goto invalid_frame;

  //GST_DEBUG ("ZE BUFFER");

  GST_BUFFER_PTS (buffer) =
        src->accum_rtime + src->timestamp_offset + src->running_time;
  GST_BUFFER_DTS (buffer) = GST_CLOCK_TIME_NONE;

  //GST_DEBUG ("LAc ..");

  guint8 *pixels = GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);
  guint stride = GST_VIDEO_FRAME_PLANE_STRIDE (&frame, 0);
  guint pixel_stride = GST_VIDEO_FRAME_COMP_PSTRIDE (&frame, 0);
  guint width = GST_VIDEO_FRAME_WIDTH (&frame);
  guint height = GST_VIDEO_FRAME_HEIGHT (&frame);

  //GST_DEBUG ("ICI ..");
  gst_object_sync_values (GST_OBJECT (psrc), GST_BUFFER_PTS (buffer));


  //GST_DEBUG ("END ..");
  GST_OBJECT_LOCK (src);
  if (src->ready){
    memcpy (pixels, src->data, 1280*720*4* sizeof(unsigned char));
    GST_OBJECT_UNLOCK (src);

  }else{
    GST_OBJECT_UNLOCK (src);
    for (int h = 0; h < height; ++h) {
        for (int w = 0; w < width; ++w) {
          guint8 *pixel = pixels + h * stride + w * pixel_stride;
          unsigned char rgb[4] =  {255, 0, 0, 0};
          memcpy (pixel, rgb, pixel_stride);
        }
    }
  }


  GST_BUFFER_OFFSET (buffer) = src->accum_frames + src->n_frames;
  src->n_frames++;
  GST_BUFFER_OFFSET_END (buffer) = GST_BUFFER_OFFSET (buffer) + 1;
  if (src->info.fps_n) {
    next_time = gst_util_uint64_scale_int (src->n_frames * GST_SECOND,
        src->info.fps_d, src->info.fps_n);
    GST_BUFFER_DURATION (buffer) = next_time - src->running_time;
  } else {
    next_time = src->timestamp_offset;
    /* NONE means forever */
    GST_BUFFER_DURATION (buffer) = GST_CLOCK_TIME_NONE;
  }

  src->running_time = next_time;

  gst_video_frame_unmap (&frame);


  return GST_FLOW_OK;

not_negotiated:
    {
      return GST_FLOW_NOT_NEGOTIATED;
    }

invalid_frame:
  {
    GST_DEBUG_OBJECT (src, "invalid frame");
    return GST_FLOW_OK;
  }
}



static gboolean gst_webkit_src_load_webkit_ready (gpointer psrc)
{
    GstWebkitSrc *src = GST_WEBKIT_SRC (psrc);

    GdkPixbuf* pixbuf = gtk_offscreen_window_get_pixbuf(src->window);
    GST_OBJECT_LOCK (src);
    memcpy(src->data, gdk_pixbuf_read_pixels(pixbuf), 1280*720*4);
    GST_OBJECT_UNLOCK (src);
    g_object_unref(pixbuf);
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
  src->ready = TRUE;

  GST_DEBUG ("init webview");
  src->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

  GST_DEBUG ("register load-changed callback");

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
  src->data = malloc(4*1280*720*sizeof(guint8));

  gtk_widget_realize(src->window);
  gtk_widget_show_all(src->window);

  g_timeout_add(50, gst_webkit_src_load_webkit_ready, (gpointer) src);

  GST_DEBUG ("End initing gtk offscreen");

}


static gboolean gst_webkit_go_to_url_cb(gpointer object){
  GstWebkitSrc *src = GST_WEBKIT_SRC (object);
  GST_OBJECT_LOCK(src);
  webkit_web_view_load_uri(src->web_view, src->url);
  GST_OBJECT_UNLOCK(src);

  return FALSE;
}

static void
gst_webkit_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstWebkitSrc *src = GST_WEBKIT_SRC (object);

  switch (prop_id) {
    case PROP_URL:
      GST_OBJECT_LOCK(src);
      src->url = g_value_dup_string (value);
      GST_OBJECT_UNLOCK(src);
      g_idle_add(gst_webkit_go_to_url_cb, (gpointer) object);

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
