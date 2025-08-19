SUBDIRS = common expr misc parser pat thunk coreir plugins

bin_PROGRAMS = lazyscript lazyscript_format

lazy_script_common_libs = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    coreir/liblscoreir.la \
    common/liblscommon.la \
    $(GC_LIBS)

lazyscript_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_LDADD = $(lazy_script_common_libs)

lazyscript_format_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscript_format_LDADD = $(lazy_script_common_libs)

# Export program symbols so dlopen()'d plugins can resolve symbols from the main binary
lazyscript_LDFLAGS = -export-dynamic

lazyscript_SOURCES = \
    lazyscript.c \
    lazyscript.h \
    lstypes.h

lazyscript_format_SOURCES = \
    lazyscript_format.c \
    lazyscript.h \
    lstypes.h

# --- QA helpers ---
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
    rm -f $(top_srcdir)/logs/cppcheck.log $(top_srcdir)/logs/lizard_*.txt $(top_srcdir)/logs/loc.txt || true
