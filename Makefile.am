SUBDIRS = src

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -Wall -Wextra -Werror
AM_YFLAGS = -d
#AM_YFLAGS = -d -Wcounterexamples

AM_MAKEFLAGS = --no-print-directory

@DX_RULES@

clean-local:
	-rm -rf doc/*

doxygen-prepare:
	mkdir -p doc

docs: doxygen-prepare doxygen-doc