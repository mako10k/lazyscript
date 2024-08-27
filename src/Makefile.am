bin_PROGRAMS = lazyscript

CLEANFILES = parser.h parser.c lexer.c

BUILT_SOURCES = lexer.c parser.h parser.c

AM_CFLAGS = -O0 -g -Wall -Wextra -Werror
#AM_YFLAGS = -d -Wcounterexamples
AM_YFLAGS = -d

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDADD = $(GC_LIBS)

lazyscript_SOURCES = \
    array.c \
    array.h \
    belist.c \
    belist.h \
    bind.c \
    bind.h \
    ealge.c \
    ealge.h \
    eappl.c \
    eappl.h \
    eclosure.c \
    eclosure.h \
    eenv.c \
    eenv.h \
    elambda.c \
    elambda.h \
    elist.c \
    elist.h \
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
    io.c \
    io.h \
    lazyscript.c \
    lazyscript.h \
    lexer.l \
    loc.c \
    loc.h \
    lstypes.h \
    malloc.c \
    malloc.h \
    palge.c \
    palge.h \
    parser.y \
    pas.c \
    pas.h \
    pat.c \
    pat.h \
    plist.c \
    plist.h \
    pref.c \
    pref.h \
    prog.c \
    prog.h \
    str.c \
    str.h \
    talge.c \
    talge.h \
    tappl.c \
    tappl.h \
    tclosure.c \
    tclosure.h \
    tenv.c \
    tenv.h \
    thunk.c \
    thunk.h \
    tlambda.c \
    tlambda.h \
    tlist.c \
    tlist.h \
    tref.c \
    tref.h \
    trref.c \
    trref.h
