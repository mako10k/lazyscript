bin_PROGRAMS = lazyscript

CLEANFILES = parser.h parser.c lexer.c

BUILT_SOURCES = lexer.c parser.h parser.c

AM_CFLAGS = -O0 -g -Wall -Wextra -Werror
#AM_YFLAGS = -d -Wcounterexamples
AM_YFLAGS = -d

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDADD = $(GC_LIBS)

lazyscript_SOURCES = \
    alge.c \
    alge.h \
    appl.c \
    appl.h \
    array.c \
    array.h \
    as.c \
    as.h \
    bind.c \
    bind.h \
    closure.c \
    closure.h \
    ealge.c \
    ealge.h \
    env.c \
    env.h \
    eref.c \
    eref.h \
    erref.c \
    erref.h \
    expr.c \
    expr.h \
    eval.c \
    eval.h \
    hash.c \
    hash.h \
    int.c \
    int.h \
    io.c \
    io.h \
    lambda.c \
    lambda.h \
    lazyscript.c \
    lazyscript.h \
    lexer.l \
    loc.c \
    loc.h \
    malloc.c \
    malloc.h \
    palge.c \
    palge.h \
    parser.y \
    pat.c \
    pat.h \
    pref.c \
    pref.h \
    prog.c \
    prog.h \
    str.c \
    str.h
