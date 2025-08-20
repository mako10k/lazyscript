#include "common/list.h"
#include "common/malloc.h"
#include <assert.h>

struct lslist {
  lslist_data_t   ll_data;
  const lslist_t* ll_next;
};

const lslist_t* lslist_new(void) { return NULL; }

const lslist_t* lslist_push(const lslist_t* list, lslist_data_t data) {
  if (list == NULL) {
    lslist_t* new_list = lsmalloc(sizeof(lslist_t));
    new_list->ll_data  = data;
    new_list->ll_next  = NULL;
    return new_list;
  }
  lslist_t* new_list = lsmalloc(sizeof(lslist_t));
  new_list->ll_data  = list->ll_data;
  new_list->ll_next  = lslist_push(list->ll_next, data);
  return new_list;
}

const lslist_t* lslist_pop(const lslist_t* list, lslist_data_t* pdata) {
  if (list == NULL)
    return NULL;
  if (list->ll_next == NULL) {
    *pdata = list->ll_data;
    return NULL;
  }
  lslist_t* new_list = lsmalloc(sizeof(lslist_t));
  new_list->ll_data  = list->ll_data;
  new_list->ll_next  = lslist_pop(list->ll_next, pdata);
  return new_list;
}

const lslist_t* lslist_unshift(const lslist_t* list, lslist_data_t data) {
  lslist_t* new_list = lsmalloc(sizeof(lslist_t));
  new_list->ll_data  = data;
  new_list->ll_next  = list;
  return new_list;
}

const lslist_t* lslist_shift(const lslist_t* list, lslist_data_t* pdata) {
  if (list == NULL)
    return NULL;
  *pdata = list->ll_data;
  return list->ll_next;
}

lssize_t lslist_count(const lslist_t* list) {
  lssize_t count = 0;
  while (list != NULL) {
    count++;
    list = list->ll_next;
  }
  return count;
}

lslist_data_t lslist_get(const lslist_t* list, lssize_t i) {
  while (i > 0) {
    if (list == NULL)
      return NULL;
    list = list->ll_next;
    i--;
  }
  return list == NULL ? NULL : list->ll_data;
}

const lslist_t* lslist_concat(const lslist_t* list1, const lslist_t* list2) {
  if (list1 == NULL)
    return list2;
  lslist_t* new_list = lsmalloc(sizeof(lslist_t));
  new_list->ll_data  = list1->ll_data;
  new_list->ll_next  = lslist_concat(list1->ll_next, list2);
  return new_list;
}

const lslist_t* lslist_get_next(const lslist_t* list) {
  return list == NULL ? NULL : list->ll_next;
}