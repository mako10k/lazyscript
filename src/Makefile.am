SUBDIRS = common expr misc parser pat thunk coreir plugins

bin_PROGRAMS = lazyscript lazyscript_format

lazy_script_common_libs = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    coreir/liblscoreir.la \
    common/liblscommon.la \
    $(GC_LIBS)

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDADD = $(lazy_script_common_libs)

lazyscript_format_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_format_LDADD = $(lazy_script_common_libs)

# Export program symbols so dlopen()'d plugins can resolve symbols from the main binary
lazyscript_LDFLAGS = -export-dynamic

lazyscript_SOURCES = \
    lazyscript.c \
    lazyscript.h \
    lstypes.h

lazyscript_format_SOURCES = \
    lazyscript_format.c \
    lazyscript.h \
    lstypes.h
