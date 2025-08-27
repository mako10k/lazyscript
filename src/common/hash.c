#include "common/hash.h"
#include "common/malloc.h"
#include "common/str.h"
#include <assert.h>
#include "common/io.h"
#include <string.h>

// *******************************
// Hash table.
// *******************************
// -------------------------------
// Structures.
// -------------------------------
/**
 * Hash table entry.
 */
typedef struct lshash_entry lshash_entry_t;

/**
 * Hash table entry.
 */
struct lshash_entry {
  /** Key */
  const lsstr_t* lhe_key;
  /** Value */
  lshash_data_t lhe_value;
  /** Next entry */
  lshash_entry_t* lhe_next;
};

/**
 * Hash table.
 */
struct lshash {
  /** Entries */
  lshash_entry_t** lh_entries;
  /** Number of entries */
  int lh_size;
  /** Capacity */
  int lh_capacity;
};

// -------------------------------
// Constructors.
// -------------------------------
lshash_t* lshash_new(lssize_t capacity) {
  assert(capacity > 0);
  lshash_t* hash    = lsmalloc(sizeof(lshash_t));
  hash->lh_capacity = capacity;
  hash->lh_size     = 0;
  hash->lh_entries  = lsmalloc(capacity * sizeof(lshash_entry_t*));
  for (lssize_t i = 0; i < capacity; i++)
    hash->lh_entries[i] = NULL;
  return hash;
}

// -------------------------------
// Accessors.
// -------------------------------
int lshash_get(lshash_t* hash, const lsstr_t* key, lshash_data_t* value) {
  assert(hash != NULL);
  assert(key != NULL);
  int             hash_value = lsstr_calc_hash(key) % hash->lh_capacity;
  lshash_entry_t* entry      = hash->lh_entries[hash_value];
  while (entry != NULL) {
    if (lsstrcmp(entry->lhe_key, key) == 0) {
      if (value != NULL)
        *value = entry->lhe_value;
      return 1;
    }
    entry = entry->lhe_next;
  }
  return 0;
}

/**
 * Inserts a new entry into the hash table.
 *
 * @param pentry    A pointer to the pointer to the hash entry.
 * @param capacity  The capacity of the hash table.
 * @param size      A pointer to the size of the hash table.
 * @param key       The key of the entry to be inserted.
 * @param value     The value of the entry to be inserted.
 * @param old_value A pointer to store the old value if the key already exists.
 * @return 1 if the entry was successfully inserted, 0 otherwise.
 */
static int lshash_put_raw(lshash_entry_t** pentry, int capacity, int* size, const lsstr_t* key,
                          lshash_data_t value, lshash_data_t* old_value) {
  assert(pentry != NULL);
  assert(size != NULL);
  assert(capacity >= *size);
  assert(key != NULL);
  while (*pentry != NULL) {
    if (lsstrcmp((*pentry)->lhe_key, key) == 0) {
      if (old_value != NULL)
        *old_value = (*pentry)->lhe_value;
      (*pentry)->lhe_value = value;
      return 1;
    }
    pentry = &(*pentry)->lhe_next;
  }
  *pentry              = lsmalloc(sizeof(lshash_entry_t));
  (*pentry)->lhe_key   = key;
  (*pentry)->lhe_value = value;
  (*pentry)->lhe_next  = NULL;
  (*size)++;
  return 0;
}

void lshash_foreach(lshash_t* hash, void (*callback)(const lsstr_t*, lshash_data_t, void*),
                    void*     data) {
  assert(hash != NULL);
  assert(callback != NULL);
  for (int i = 0; i < hash->lh_capacity; i++) {
    lshash_entry_t* entry = hash->lh_entries[i];
    while (entry != NULL) {
      callback(entry->lhe_key, entry->lhe_value, data);
      entry = entry->lhe_next;
    }
  }
}

/**
 * Rehashes the given hash table with a new capacity.
 *
 * @param hash The hash table to be rehashed.
 * @param new_capacity The new capacity of the hash table.
 */
static void lshash_rehash(lshash_t* hash, int new_capacity) {
  assert(hash != NULL);
  if (new_capacity < hash->lh_size) {
    // debug + correction
    lsprintf(stderr, 0, "WARN: rehash grow too small: new=%d size=%d -> fix to %d\n", new_capacity,
             hash->lh_size, hash->lh_size * 2);
    new_capacity = hash->lh_size * 2;
  }
  lshash_entry_t** new_entries = lsmalloc(new_capacity * sizeof(lshash_entry_t*));
  for (int i = 0; i < new_capacity; i++)
    new_entries[i] = NULL;
  int new_size = 0;
  for (int i = 0; i < hash->lh_capacity; i++) {
    lshash_entry_t* entry = hash->lh_entries[i];
    while (entry != NULL) {
      int hash_value = lsstr_calc_hash(entry->lhe_key) % new_capacity;
      lshash_put_raw(&new_entries[hash_value], new_capacity, &new_size, entry->lhe_key,
                     entry->lhe_value, NULL);
      entry = entry->lhe_next;
    }
  }
  lsfree(hash->lh_entries);
  hash->lh_entries  = new_entries;
  hash->lh_capacity = new_capacity;
  hash->lh_size     = new_size;
}

int lshash_put(lshash_t* hash, const lsstr_t* key, lshash_data_t value, lshash_data_t* old_value) {
  assert(hash != NULL);
  assert(key != NULL);
  if (hash->lh_capacity <= 0 || hash->lh_capacity > 1<<26 || hash->lh_size < 0) {
    lsprintf(stderr, 0, "WARN: hash corrupt pre-put cap=%d size=%d -> reset\n", hash->lh_capacity, hash->lh_size);
    // attempt to reset to a sane empty table
    hash->lh_capacity = 16;
    hash->lh_size = 0;
    lsfree(hash->lh_entries);
    hash->lh_entries = lsmalloc(hash->lh_capacity * sizeof(lshash_entry_t*));
    for (int i=0;i<hash->lh_capacity;i++) hash->lh_entries[i] = NULL;
  }
  if (hash->lh_size >= hash->lh_capacity * 0.75)
    lshash_rehash(hash, hash->lh_capacity * 2);
  return lshash_put_raw(&hash->lh_entries[lsstr_calc_hash(key) % hash->lh_capacity],
                        hash->lh_capacity, &hash->lh_size, key, value, old_value);
}

int lshash_remove(lshash_t* hash, const lsstr_t* key, lshash_data_t* pvalue) {
  assert(hash != NULL);
  assert(key != NULL);
  int              hash_value = lsstr_calc_hash(key) % hash->lh_capacity;
  lshash_entry_t** pentry     = &hash->lh_entries[hash_value];
  while (*pentry != NULL) {
    if (lsstrcmp((*pentry)->lhe_key, key) == 0) {
      if (pvalue != NULL)
        *pvalue = (*pentry)->lhe_value;
      lshash_entry_t* entry = *pentry;
      *pentry               = entry->lhe_next;
      lsfree((void*)entry->lhe_key);
      lsfree(entry);
      hash->lh_size--;
      return 1;
    }
    pentry = &(*pentry)->lhe_next;
  }
  return 0;
}