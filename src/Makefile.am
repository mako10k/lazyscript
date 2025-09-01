AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = common expr misc parser pat thunk lstr plugins

bin_PROGRAMS = lazyscript lazyscript_format lslsti_check lslstr_dump lslstr_roundtrip

lazy_script_common_libs = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    \
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
    runtime/effects.c \
    runtime/trace.c \
    runtime/modules.c \
    builtins/dump.c \
    builtins/to_string.c \
    builtins/print.c \
    builtins/seq.c \
    builtins/arith.c \
    builtins/builtin_loader.c \
    builtins/require.c \
    builtins/ns.c \
    lazyscript.h \
    lstypes.h

lazyscript_format_SOURCES = \
    lazyscript_format.c \
    lazyscript.h \
    lstypes.h

# (removed) lsllvmir_dump was relying on Core IR; disabled with coreir removal

# (removed) lazyscriptc (Core IR frontend) disabled with coreir removal

lslsti_check_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
# Arrange libraries so providers come after users for static linking
lslsti_check_LDADD = \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lslsti_check_SOURCES = \
    tools/lslsti_check.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lslstr_dump_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lslstr_dump_LDADD = \
    lstr/liblslstr.la \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lslstr_dump_SOURCES = \
    tools/lslstr_dump.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lslstr_roundtrip_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lslstr_roundtrip_LDADD = \
    lstr/liblslstr.la \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lslstr_roundtrip_SOURCES = \
    tools/lslstr_roundtrip.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h
