SUBDIRS = common expr misc parser pat thunk plugins

bin_PROGRAMS = lazyscript

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDADD = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    common/liblscommon.la \
    $(GC_LIBS)

# Export program symbols so dlopen()'d plugins can resolve symbols from the main binary
lazyscript_LDFLAGS = -export-dynamic

lazyscript_SOURCES = \
    lazyscript.c \
    lazyscript.h \
    lstypes.h
