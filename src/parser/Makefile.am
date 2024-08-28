noinst_LIBRARIES = liblsparser.a

CLEANFILES = parser.h parser.c lexer.c

BUILT_SOURCES = lexer.c parser.h parser.c

AM_CFLAGS = -O0 -g -Wall -Wextra -Werror -I..
#AM_YFLAGS = -d -Wcounterexamples
AM_YFLAGS = -d

liblsparser_a_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)

liblsparser_a_SOURCES = \
    lexer.l \
    parser.y
