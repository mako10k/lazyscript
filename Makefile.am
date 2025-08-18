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

# --- Tests ---
TESTS = test/run-tests.sh
EXTRA_DIST = \
	test/run-tests.sh \
	test/t01_add.ls test/t01_add.out \
	test/t02_println.ls test/t02_println.out \
	test/t03_seq.ls test/t03_seq.out \
	test/t04_lambda_id.ls test/t04_lambda_id.out