#include "lsti.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

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

int lsti_write(FILE* fp, struct lsthunk* const* roots, lssize_t rootc, const lsti_write_opts_t* opt) {
  if (!fp || !roots || rootc < 0) return -EINVAL;
  uint16_t align_log2 = opt ? opt->align_log2 : (uint16_t)LSTI_ALIGN_8;
  if (align_log2 != (uint16_t)LSTI_ALIGN_8 && align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
  // Minimal placeholder writer: header only, no sections (not a valid image, but recognizable)
  lsti_header_disk_t hdr;
  hdr.magic = LSTI_MAGIC;
  hdr.version_major = LSTI_VERSION_MAJOR;
  hdr.version_minor = LSTI_VERSION_MINOR;
  hdr.section_count = 0;
  hdr.align_log2 = align_log2;
  hdr.flags = opt ? opt->flags : 0u;
  size_t n = fwrite(&hdr, 1, sizeof(hdr), fp);
  return (n == sizeof(hdr)) ? 0 : -EIO;
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
  out_img->section_count = 0;
  out_img->align_log2 = 0;
  out_img->flags = 0u;
  return 0;
}

int lsti_validate(const lsti_image_t* img) {
  if (!img || !img->base || img->size < (lssize_t)sizeof(lsti_header_disk_t)) return -EINVAL;
  const lsti_header_disk_t* hdr = (const lsti_header_disk_t*)img->base;
  if (hdr->magic != LSTI_MAGIC) return -EINVAL;
  if (hdr->version_major != LSTI_VERSION_MAJOR) return -EINVAL;
  if (hdr->align_log2 != (uint16_t)LSTI_ALIGN_8 && hdr->align_log2 != (uint16_t)LSTI_ALIGN_16) return -EINVAL;
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
