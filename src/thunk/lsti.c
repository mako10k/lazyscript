#include "lsti.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef ENABLE_LSTI
// If not enabled, provide stubs that return ENOSYS to keep build/link stable.
int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc, const lsti_write_opts_t* opt) {
  (void)fp; (void)roots; (void)rootc; (void)opt; return -ENOSYS;
}
int lsti_map(const char* path, lsti_image_t* out_img) {
  (void)path; (void)out_img; return -ENOSYS;
}
int lsti_validate(const lsti_image_t* img) {
  (void)img; return -ENOSYS;
}
int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc, struct lstenv* prelude_env) {
  (void)img; (void)out_roots; (void)out_rootc; (void)prelude_env; return -ENOSYS;
}
int lsti_unmap(lsti_image_t* img) {
  (void)img; return -ENOSYS;
}

#else

typedef struct lsti_header_disk {
  uint32_t magic;          // 'LSTI'
  uint16_t version_major;  // =1
  uint16_t version_minor;  // =0
  uint16_t section_count;
  uint16_t align_log2;     // 3 or 4
  uint32_t flags;          // LSTI_F_*
} lsti_header_disk_t;

typedef struct lsti_section_disk {
  uint32_t kind;     // lsti_sect_kind_t
  uint32_t reserved; // 0
  uint64_t file_off; // absolute file offset
  uint64_t size;     // payload size in bytes
} lsti_section_disk_t;

static int fpad_to_align(FILE* fp, uint16_t align_log2) {
  long pos = ftell(fp);
  if (pos < 0) return -EIO;
  long mask = (1L << align_log2) - 1L;
  long pad = ((pos + mask) & ~mask) - pos;
  static const uint8_t zeros[16] = {0};
  while (pad > 0) {
    size_t chunk = (size_t)((pad > (long)sizeof(zeros)) ? sizeof(zeros) : pad);
    if (fwrite(zeros, 1, chunk, fp) != chunk) return -EIO;
    pad -= (long)chunk;
  }
  return 0;
}

int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc, const lsti_write_opts_t* opt) {
  if (!fp || rootc < 0) return -EINVAL;
  uint16_t align_log2 = opt ? opt->align_log2 : (uint16_t)LSTI_ALIGN_8;
  if (align_log2 != (uint16_t)LSTI_ALIGN_8 && align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
  if ((uint64_t)rootc > 0xFFFFFFFFu) return -EOVERFLOW;
  // Header
  lsti_header_disk_t hdr;
  hdr.magic = LSTI_MAGIC;
  hdr.version_major = LSTI_VERSION_MAJOR;
  hdr.version_minor = LSTI_VERSION_MINOR;
  hdr.section_count = 1; // ROOTS only for now
  hdr.align_log2 = align_log2;
  hdr.flags = opt ? opt->flags : 0u;
  if (fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) return -EIO;
  // Placeholder section table
  long sect_tbl_off = ftell(fp);
  if (sect_tbl_off < 0) return -EIO;
  lsti_section_disk_t sec = {0};
  if (fwrite(&sec, 1, sizeof(sec), fp) != sizeof(sec)) return -EIO;
  // Align and write ROOTS payload: u32 count + u32 ids[count]
  if (fpad_to_align(fp, align_log2) != 0) return -EIO;
  long roots_off = ftell(fp);
  if (roots_off < 0) return -EIO;
  uint32_t count32 = (uint32_t)rootc;
  if (fwrite(&count32, 1, sizeof(count32), fp) != sizeof(count32)) return -EIO;
  // For now, write zero IDs as placeholders
  if (rootc > 0) {
    uint32_t zero = 0;
    for (lssize_t i = 0; i < rootc; ++i) {
      if (fwrite(&zero, 1, sizeof(zero), fp) != sizeof(zero)) return -EIO;
    }
  }
  long end_off = ftell(fp);
  if (end_off < 0) return -EIO;
  uint64_t roots_size = (uint64_t)(end_off - roots_off);
  // Backfill section table entry
  if (fseek(fp, sect_tbl_off, SEEK_SET) != 0) return -EIO;
  lsti_section_disk_t roots_entry;
  roots_entry.kind = (uint32_t)LSTI_SECT_ROOTS;
  roots_entry.reserved = 0u;
  roots_entry.file_off = (uint64_t)roots_off;
  roots_entry.size = roots_size;
  if (fwrite(&roots_entry, 1, sizeof(roots_entry), fp) != sizeof(roots_entry)) return -EIO;
  // Seek to end
  if (fseek(fp, 0, SEEK_END) != 0) return -EIO;
  return 0;
}

int lsti_map(const char* path, lsti_image_t* out_img) {
  if (!path || !out_img) return -EINVAL;
  FILE* fp = fopen(path, "rb");
  if (!fp) return -errno;
  if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -EIO; }
  long sz = ftell(fp);
  if (sz < 0) { fclose(fp); return -EIO; }
  if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); return -EIO; }
  uint8_t* buf = (uint8_t*)malloc((size_t)sz);
  if (!buf) { fclose(fp); return -ENOMEM; }
  size_t rd = fread(buf, 1, (size_t)sz, fp);
  fclose(fp);
  if (rd != (size_t)sz) { free(buf); return -EIO; }
  out_img->base = buf;
  out_img->size = (lssize_t)sz;
  if ((lssize_t)sz >= (lssize_t)sizeof(lsti_header_disk_t)) {
    const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)buf;
    out_img->section_count = hdr->section_count;
    out_img->align_log2 = hdr->align_log2;
    out_img->flags = hdr->flags;
  } else {
    out_img->section_count = 0;
    out_img->align_log2 = 0;
    out_img->flags = 0u;
  }
  return 0;
}

int lsti_validate(const lsti_image_t* img) {
  if (!img || !img->base || img->size < (lssize_t)sizeof(lsti_header_disk_t)) return -EINVAL;
  const uint8_t* p = img->base;
  const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)p;
  if (hdr->magic != LSTI_MAGIC) return -EINVAL;
  if (hdr->version_major != LSTI_VERSION_MAJOR) return -EINVAL;
  if (hdr->align_log2 != (uint16_t)LSTI_ALIGN_8 && hdr->align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
  lssize_t need = (lssize_t)sizeof(lsti_header_disk_t) + (lssize_t)hdr->section_count * (lssize_t)sizeof(lsti_section_disk_t);
  if (img->size < need) return -EINVAL;
  const lsti_section_disk_t* sect = (const lsti_section_disk_t*)(p + sizeof(lsti_header_disk_t));
  for (uint16_t i = 0; i < hdr->section_count; ++i) {
    uint64_t off = sect[i].file_off;
    uint64_t sz  = sect[i].size;
    if (sz > (uint64_t)img->size) return -EINVAL;
    if (off > (uint64_t)img->size) return -EINVAL;
    if (off + sz > (uint64_t)img->size) return -EINVAL;
    (void)sect[i].kind; // unknown kinds are allowed; skip
  }
  return 0;
}

int lsti_materialize(const lsti_image_t* img, struct lsthunk*** out_roots, lssize_t* out_rootc, struct lstenv* prelude_env) {
  (void)img; (void)out_roots; (void)out_rootc; (void)prelude_env;
  // Not implemented yet
  return -ENOSYS;
}

int lsti_unmap(lsti_image_t* img) {
  if (!img) return -EINVAL;
  free((void*)img->base);
  img->base = NULL;
  img->size = 0;
  return 0;
}

#endif
