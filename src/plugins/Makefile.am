plugindir = $(libdir)/lazyscript

plugin_LTLIBRARIES = liblazyscript_prelude.la \
                     liblazyscript_demo.la \
                     liblazyscript_core.la

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

# Demo builtin plugin built to exercise (~prelude builtin)
liblazyscript_demo_la_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS) -I.. -fPIC
liblazyscript_demo_la_LDFLAGS = -module -avoid-version -shared
liblazyscript_demo_la_LIBADD = \
    ../thunk/liblsthunk.la \
    ../expr/liblsexpr.la \
    ../common/liblscommon.la \
    $(GC_LIBS)

liblazyscript_demo_la_SOURCES = \
    demo_builtin.c

# Core builtins plugin (exposes dump/to_str/print/seq/arith/lt/eq via (~builtin "core"))
liblazyscript_core_la_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS) -I.. -fPIC
liblazyscript_core_la_LDFLAGS = -module -avoid-version -shared
liblazyscript_core_la_LIBADD = \
    ../thunk/liblsthunk.la \
    ../expr/liblsexpr.la \
    ../common/liblscommon.la \
    $(GC_LIBS)
liblazyscript_core_la_SOURCES = \
    core_builtins.c
