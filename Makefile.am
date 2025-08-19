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

# --- QA helpers (top-level) ---
.PHONY: qa qa-clean
qa:
	@mkdir -p $(top_srcdir)/logs
	@echo "[QA] cppcheck (src only)"; \
	cppcheck --enable=all --inconclusive --std=c11 \
		--suppress=missingIncludeSystem \
		--force \
		-I$(top_srcdir)/src \
		--template=gcc \
		$(top_srcdir)/src 2> $(top_srcdir)/logs/cppcheck.log || true; \
	echo "[QA] lizard (complexity/duplicate)"; \
	python3 -m lizard -C 10 $(top_srcdir)/src > $(top_srcdir)/logs/lizard_src.txt || true; \
	python3 -m lizard -C 10 -Eduplicate $(top_srcdir)/src > $(top_srcdir)/logs/lizard_src_highccn.txt || true; \
	find $(top_srcdir)/src -name '*.c' -o -name '*.h' | xargs wc -l > $(top_srcdir)/logs/loc.txt

qa-clean:
	-rm -f \
		$(top_srcdir)/logs/cppcheck.log \
		$(top_srcdir)/logs/lizard_src.txt \
		$(top_srcdir)/logs/lizard_src_highccn.txt \
		$(top_srcdir)/logs/loc.txt || true

# --- Tests ---
TESTS = test/run-tests.sh
EXTRA_DIST = \
	test/run-tests.sh \
	test/t01_add.ls test/t01_add.out \
	test/t02_println.ls test/t02_println.out \
	test/t03_seq.ls test/t03_seq.out \
	test/t04_lambda_id.ls test/t04_lambda_id.out