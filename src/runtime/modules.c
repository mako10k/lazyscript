#include "runtime/modules.h"
#include "common/hash.h"
#include "common/str.h"
#include <string.h>

static lshash_t* g_loaded = NULL; // key: lsstr_t*

int              ls_modules_is_loaded(const char* path_cstr) {
               if (!g_loaded)
    return 0;
  const lsstr_t* k = lsstr_cstr(path_cstr);
               lshash_data_t  v;
               return lshash_get(g_loaded, k, &v);
}

void ls_modules_mark_loaded(const char* path_cstr) {
  if (!g_loaded)
    g_loaded = lshash_new(16);
  const lsstr_t* k = lsstr_cstr(path_cstr);
  lshash_data_t  oldv;
  (void)lshash_put(g_loaded, k, (const void*)1, &oldv);
}
