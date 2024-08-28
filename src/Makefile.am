SUBDIRS = common expr misc parser pat thunk

bin_PROGRAMS = lazyscript

AM_CFLAGS = -O0 -g -Wall -Wextra -Werror

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDFLAGS = -Lcommon -Lexpr -Lmisc -Lparser -Lpat -Lthunk
lazyscript_LDADD = $(GC_LIBS) -llscommon -llsexpr -llsmisc -llsparser -llspat -llsthunk

lazyscript_SOURCES = \
    lazyscript.c \
    lazyscript.h \
    lstypes.h
