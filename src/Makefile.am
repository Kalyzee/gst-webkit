# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstplugin.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstwebkitoverlay.la libgstwebkitsrc.la



# sources used to compile this plug-in
libgstwebkitoverlay_la_SOURCES = gstwebkitoverlayfilter.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstwebkitoverlay_la_CFLAGS = $(GST_CFLAGS) $(CAIRO_CFLAGS) $(WEBKIT_CFLAGS)
libgstwebkitoverlay_la_LIBADD = $(GST_LIBS)  $(CAIRO_LIBS) $(WEBKIT_LIBS)
libgstwebkitoverlay_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwebkitoverlay_la_LIBTOOLFLAGS = --tag=disable-static



# sources used to compile this plug-in
libgstwebkitsrc_la_SOURCES = gstwebkitsrc.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstwebkitsrc_la_CFLAGS = $(GST_CFLAGS) $(CAIRO_CFLAGS) $(WEBKIT_CFLAGS)
libgstwebkitsrc_la_LIBADD = $(GST_LIBS)  $(CAIRO_LIBS) $(WEBKIT_LIBS)
libgstwebkitsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstwebkitsrc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstwebkitoverlayfilter.h
