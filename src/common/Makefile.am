noinst_LIBRARIES = liblscommon.a

AM_CFLAGS = -O0 -g -Wall -Wextra -Werror -I..
#AM_YFLAGS = -d -Wcounterexamples
AM_YFLAGS = -d

liblscommon_a_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)

liblscommon_a_SOURCES = \
    array.c \
    array.h \
    hash.c \
    hash.h \
    int.c \
    int.h \
    io.c \
    io.h \
    loc.c \
    loc.h \
    str.c \
    str.h
