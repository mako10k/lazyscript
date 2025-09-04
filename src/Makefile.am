AUTOMAKE_OPTIONS = subdir-objects
SUBDIRS = common expr misc parser pat thunk lstr plugins

# Breaking rename: avoid collision with system `ls` by using `lsi-*` prefix.
# Old -> New
#   lazyscript             -> lsi
#   lazyscript_format      -> lsi-fmt
#   lslsti_check           -> lsi-lsti
#   lslstr_dump            -> lsi-lstr-dump
#   lslstr_roundtrip       -> lsi-lstr-rt
#   lstr_to_llvmir         -> lsi-emit-llvm
bin_PROGRAMS = lsi lsi-fmt lsi-lsti lsi-lstr-dump lsi-lstr-rt lsi-emit-llvm

lazy_script_common_libs = \
    parser/liblsparser.la \
    misc/liblsmisc.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    \
    common/liblscommon.la \
    $(GC_LIBS)

lsi_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsi_LDADD = $(lazy_script_common_libs)

lsi_fmt_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsi_fmt_LDADD = $(lazy_script_common_libs)

# Export program symbols so dlopen()'d plugins can resolve symbols from the main binary
lsi_LDFLAGS = -export-dynamic

lsi_SOURCES = \
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

lsi_fmt_SOURCES = \
    lazyscript_format.c \
    lazyscript.h \
    lstypes.h

# (removed) lsllvmir_dump was relying on Core IR; disabled with coreir removal

# (removed) lazyscriptc (Core IR frontend) disabled with coreir removal

lsi_lsti_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
# Arrange libraries so providers come after users for static linking
lsi_lsti_LDADD = \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lsi_lsti_SOURCES = \
    tools/lslsti_check.c \
    tools/cli_common.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lsi_lstr_dump_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsi_lstr_dump_LDADD = \
    lstr/liblslstr.la \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lsi_lstr_dump_SOURCES = \
    tools/lslstr_dump.c \
    tools/lstr_prog_common.c \
    tools/cli_common.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lsi_lstr_rt_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsi_lstr_rt_LDADD = \
    lstr/liblslstr.la \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lsi_lstr_rt_SOURCES = \
    tools/lslstr_roundtrip.c \
    tools/cli_common.c \
    tools/lstr_prog_common.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h

lsi_emit_llvm_CFLAGS = $(AM_CFLAGS) $(GC_CFLAGS)
lsi_emit_llvm_LDADD = \
    lstr/liblslstr.la \
    parser/liblsparser.la \
    thunk/liblsthunk.la \
    pat/liblspat.la \
    expr/liblsexpr.la \
    misc/liblsmisc.la \
    common/liblscommon.la \
    $(GC_LIBS)
lsi_emit_llvm_SOURCES = \
    tools/lstr_to_llvmir.c \
    tools/llvm_emit_common.c \
    tools/lstr_prog_common.c \
    tools/cli_common.c \
    runtime/trace.c \
    runtime/effects.c \
    lazyscript.h \
    lstypes.h
