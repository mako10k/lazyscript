noinst_LTLIBRARIES = liblsparser.la

CLEANFILES = parser.h parser.c lexer.c

BUILT_SOURCES = lexer.c parser.h parser.c

AM_YFLAGS = -d
#AM_YFLAGS = -d -Wcounterexamples

liblsparser_la_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS) -I..

liblsparser_la_SOURCES = \
    lexer.l \
    parser.y
