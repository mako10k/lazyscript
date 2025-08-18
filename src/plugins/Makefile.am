plugindir = $(libdir)/lazyscript

plugin_LTLIBRARIES = liblazyscript_prelude.la

liblazyscript_prelude_la_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS) -I.. -fPIC
liblazyscript_prelude_la_LDFLAGS = -module -avoid-version -shared
liblazyscript_prelude_la_LIBADD = \
    ../thunk/liblsthunk.la \
    ../pat/liblspat.la \
    ../expr/liblsexpr.la \
    ../common/liblscommon.la \
    $(GC_LIBS)

liblazyscript_prelude_la_SOURCES = \
    prelude_plugin.c
