bin_PROGRAMS = lazyscript

BUILT_SOURCES = lexer.c parser.h parser.c

AM_CFLAGS = -Wall -Wextra -Werror
AM_YFLAGS = -d

lazyscript_CFLAGS = $(GC_CFLAGS)
lazyscript_LDADD = $(GC_LIBS)

lazyscript_SOURCES = \
    alge.c \
    alge.h \
    appl.c \
    appl.h \
    array.c \
    array.h \
    bind.c \
    bind.h \
    ealge.c \
    ealge.h \
    eref.c \
    eref.h \
    erref.c \
    erref.h \
    expr.c \
    expr.h \
    hash.c \
    hash.h \
    int.c \
    int.h \
    lambda.c \
    lambda.h \
    lazyscript.c \
    lazyscript.h \
    lexer.l \
    malloc.c \
    malloc.h \
    prog.c \
    prog.h \
    str.c \
    str.h \
    parser.y