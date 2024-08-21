#include "hash.h"
#include "malloc.h"
#include "str.h"
#include <assert.h>
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
  const lsstr_t *khe_key;
  /** Value */
  void *khe_value;
  /** Next entry */
  lshash_entry_t *khe_next;
};

/**
 * Hash table.
 */
struct lshash {
  /** Entries */
  lshash_entry_t **kh_entries;
  /** Number of entries */
  int kh_size;
  /** Capacity */
  int kh_capacity;
};

// -------------------------------
// Constructors.
// -------------------------------
lshash_t *lshash(unsigned int capacity) {
  assert(capacity > 0);
  lshash_t *hash = lsmalloc(sizeof(lshash_t));
  hash->kh_capacity = capacity;
  hash->kh_size = 0;
  hash->kh_entries = lsmalloc(capacity * sizeof(lshash_entry_t *));
  for (unsigned int i = 0; i < capacity; i++)
    hash->kh_entries[i] = NULL;
  return hash;
}

// -------------------------------
// Accessors.
// -------------------------------
int lshash_get(lshash_t *hash, const lsstr_t *key, void **value) {
  assert(hash != NULL);
  assert(key != NULL);
  int hash_value = lsstr_calc_hash(key) % hash->kh_capacity;
  lshash_entry_t *entry = hash->kh_entries[hash_value];
  while (entry != NULL) {
    if (lsstrcmp(entry->khe_key, key) == 0) {
      if (value != NULL)
        *value = entry->khe_value;
      return 1;
    }
    entry = entry->khe_next;
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
static int lshash_put_raw(lshash_entry_t **pentry, int capacity, int *size,
                          const lsstr_t *key, void *value, void **old_value) {
  assert(pentry != NULL);
  assert(size != NULL);
  assert(capacity >= *size);
  assert(key != NULL);
  while (*pentry != NULL) {
    if (lsstrcmp((*pentry)->khe_key, key) == 0) {
      if (old_value != NULL)
        *old_value = (*pentry)->khe_value;
      (*pentry)->khe_value = value;
      return 1;
    }
    pentry = &(*pentry)->khe_next;
  }
  *pentry = lsmalloc(sizeof(lshash_entry_t));
  (*pentry)->khe_key = key;
  (*pentry)->khe_value = value;
  (*pentry)->khe_next = NULL;
  (*size)++;
  return 0;
}

void lshash_foreach(lshash_t *hash,
                    void (*callback)(const lsstr_t *, void *, void *),
                    void *data) {
  assert(hash != NULL);
  assert(callback != NULL);
  for (int i = 0; i < hash->kh_capacity; i++) {
    lshash_entry_t *entry = hash->kh_entries[i];
    while (entry != NULL) {
      callback(entry->khe_key, entry->khe_value, data);
      entry = entry->khe_next;
    }
  }
}

/**
 * Rehashes the given hash table with a new capacity.
 *
 * @param hash The hash table to be rehashed.
 * @param new_capacity The new capacity of the hash table.
 */
static void lshash_rehash(lshash_t *hash, int new_capacity) {
  assert(hash != NULL);
  assert(new_capacity >= hash->kh_size);
  lshash_entry_t **new_entries =
      lsmalloc(new_capacity * sizeof(lshash_entry_t *));
  for (int i = 0; i < new_capacity; i++)
    new_entries[i] = NULL;
  for (int i = 0; i < hash->kh_capacity; i++) {
    lshash_entry_t *entry = hash->kh_entries[i];
    while (entry != NULL) {
      int hash_value = lsstr_calc_hash(entry->khe_key) % new_capacity;
      lshash_put_raw(&new_entries[hash_value], new_capacity, &hash->kh_size,
                     entry->khe_key, entry->khe_value, NULL);
      entry = entry->khe_next;
    }
  }
  lsfree(hash->kh_entries);
  hash->kh_entries = new_entries;
  hash->kh_capacity = new_capacity;
}

int lshash_put(lshash_t *hash, const lsstr_t *key, void *value,
               void **old_value) {
  assert(hash != NULL);
  assert(key != NULL);
  if (hash->kh_size >= hash->kh_capacity * 0.75)
    lshash_rehash(hash, hash->kh_capacity * 2);
  return lshash_put_raw(
      &hash->kh_entries[lsstr_calc_hash(key) % hash->kh_capacity],
      hash->kh_capacity, &hash->kh_size, key, value, old_value);
}

int lshash_remove(lshash_t *hash, const lsstr_t *key, void **value) {
  assert(hash != NULL);
  assert(key != NULL);
  int hash_value = lsstr_calc_hash(key) % hash->kh_capacity;
  lshash_entry_t **pentry = &hash->kh_entries[hash_value];
  while (*pentry != NULL) {
    if (lsstrcmp((*pentry)->khe_key, key) == 0) {
      if (value != NULL)
        *value = (*pentry)->khe_value;
      lshash_entry_t *entry = *pentry;
      *pentry = entry->khe_next;
      lsfree((void *)entry->khe_key);
      lsfree(entry);
      hash->kh_size--;
      return 1;
    }
    pentry = &(*pentry)->khe_next;
  }
  return 0;
}