#ifndef LAZYSCRIPT_LSTI_H
#define LAZYSCRIPT_LSTI_H

#include <stdio.h>
#include <stdint.h>
#include "lstypes.h" // for lssize_t

#ifdef __cplusplus
extern "C" {
#endif

// Magic 'LSTI' and version (v1.0)
#define LSTI_MAGIC 0x4C535449u
#define LSTI_VERSION_MAJOR 1u
#define LSTI_VERSION_MINOR 0u

// File-level flags
#define LSTI_F_STORE_TYPES (1u << 0)
#define LSTI_F_STORE_LOCS (1u << 1)
#define LSTI_F_STORE_TRACE (1u << 2)

// Suggested alignment (2^align_log2 bytes)
typedef enum lsti_align {
  LSTI_ALIGN_8  = 3, // 8B
  LSTI_ALIGN_16 = 4  // 16B
} lsti_align_t;

// Section kinds
typedef enum lsti_sect_kind {
  LSTI_SECT_STRING_BLOB = 1,
  LSTI_SECT_SYMBOL_BLOB = 2,
  LSTI_SECT_PATTERN_TAB = 3,
  LSTI_SECT_THUNK_TAB   = 4,
  LSTI_SECT_ROOTS       = 5,
  LSTI_SECT_TYPE_TAB    = 6
} lsti_sect_kind_t;

// Opaque types
typedef struct lsthunk lsthunk_t;
typedef struct lstenv  lstenv_t;

typedef struct lsti_image {
  const uint8_t* base; // mapped or loaded base
  lssize_t       size; // total size
  uint16_t       section_count;
  uint16_t       align_log2;
  uint32_t       flags;
  // internal: offsets to sections could be indexed via a compact table
} lsti_image_t;

// Writer options
typedef struct lsti_write_opts {
  uint16_t align_log2; // 3 or 4
  uint32_t flags;      // LSTI_F_*
} lsti_write_opts_t;

// API
int lsti_write(FILE* fp, lsthunk_t* const* roots, lssize_t rootc, const lsti_write_opts_t* opt);
int lsti_map(const char* path, lsti_image_t* out_img); // mmap or fread
int lsti_validate(const lsti_image_t* img);            // magic/version/align/sections
int lsti_materialize(const lsti_image_t* img, lsthunk_t*** out_roots, lssize_t* out_rootc,
                     lstenv_t* prelude_env);
int lsti_unmap(lsti_image_t* img);

#ifdef __cplusplus
}
#endif

#endif // LAZYSCRIPT_LSTI_H
