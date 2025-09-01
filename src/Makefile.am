AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = common expr misc parser pat thunk coreir lstr llvmir plugins

bin_PROGRAMS = lazyscript lazyscript_format lsllvmir_dump lazyscriptc lscoreir lslsti_check lslstr_dump lslstr_roundtrip

lazy_script_common_libs = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    coreir/liblscoreir.la \
    llvmir/liblsllvmir.la \
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

lsllvmir_dump_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsllvmir_dump_LDADD = $(lazy_script_common_libs)
lsllvmir_dump_SOURCES = \
    lsllvmir_dump.c \
    lazyscript.h \
    lstypes.h

# Compiler (frontend → Core IR)
lazyscriptc_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lazyscriptc_LDADD = $(lazy_script_common_libs)
lazyscriptc_SOURCES = \
    tools/lazyscriptc_main.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

# Runtime (Core IR evaluator)
lscoreir_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lscoreir_LDADD = $(lazy_script_common_libs)
lscoreir_SOURCES = \
    tools/lscoreir_main.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lslsti_check_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
# Arrange libraries so providers come after users for static linking
lslsti_check_LDADD = \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    coreir/liblscoreir.la \
    llvmir/liblsllvmir.la \
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
    coreir/liblscoreir.la \
    llvmir/liblsllvmir.la \
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
    coreir/liblscoreir.la \
    llvmir/liblsllvmir.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lslstr_roundtrip_SOURCES = \
    tools/lslstr_roundtrip.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h
