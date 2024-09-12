SUBDIRS = common expr misc parser pat thunk

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

lazyscript_SOURCES = \
    lazyscript.c \
    lazyscript.h \
    lsassert.h \
    lstypes.h
