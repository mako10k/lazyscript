#include "pat/plist.h"
#include "common/list.h"
#include <assert.h>

struct lsplist {
  lspat_t*         lpl_pat;
  const lsplist_t* lpl_next;
};

const lsplist_t* lsplist_new(void) { return (const lsplist_t*)lslist_new(); }

const lsplist_t* lsplist_push(const lsplist_t* plist, const lspat_t* pat) {
  return (const lsplist_t*)lslist_push((const lslist_t*)plist, pat);
}

const lsplist_t* lsplist_pop(const lsplist_t* plist, const lspat_t** ppat) {
  return (const lsplist_t*)lslist_pop((const lslist_t*)plist, (lslist_data_t*)ppat);
}

const lsplist_t* lsplist_unshift(const lsplist_t* plist, const lspat_t* pat) {
  return (const lsplist_t*)lslist_unshift((const lslist_t*)plist, pat);
}

const lsplist_t* lsplist_shift(const lsplist_t* plist, const lspat_t** ppat) {
  return (const lsplist_t*)lslist_shift((const lslist_t*)plist, (lslist_data_t*)ppat);
}

lssize_t lsplist_count(const lsplist_t* plist) { return lslist_count((const lslist_t*)plist); }

const lspat_t* lsplist_get(const lsplist_t* plist, lssize_t i) {
  return (const lspat_t*)lslist_get((const lslist_t*)plist, i);
}

const lsplist_t* lsplist_concat(const lsplist_t* plist1, const lsplist_t* plist2) {
  return (const lsplist_t*)lslist_concat((const lslist_t*)plist1, (const lslist_t*)plist2);
}

const lsplist_t* lsplist_get_next(const lsplist_t* plist) {
  return (const lsplist_t*)lslist_get_next((const lslist_t*)plist);
}