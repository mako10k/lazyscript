#pragma once

typedef struct lshash lshash_t;

#include "str.h"

// *******************************
// Hash table.
// *******************************
// -------------------------------
// Constructors.
// -------------------------------
/**
 * Make a new hash table.
 * @param capacity Initial capacity.
 * @return New hash table.
 */
lshash_t *lshash(unsigned int capacity);

// -------------------------------
// Accessors.
// -------------------------------
/**
 * Get the number of entries in a hash table.
 * @param hash Hash table.
 * @param key Key.
 * @param value Pointer to the variable to store the value.
 * @return not found: 0, found: non-zero.
 */
int lshash_get(lshash_t *hash, const lsstr_t *key, void **value);

/**
 * Put a key-value pair into a hash table.
 * @param hash Hash table.
 * @param key Key.
 * @param value Value.
 * @param old_value Pointer to the variable to store the old value.
 * @return 1 if the key already exists, 0 otherwise.
 */
int lshash_put(lshash_t *hash, const lsstr_t *key, void *value,
               void **old_value);

/**
 * Remove a key-value pair from a hash table.
 * @param hash Hash table.
 * @param key Key.
 * @param value Pointer to the variable to store the value.
 * @return 1 if the key exists, 0 otherwise.
 */
int lshash_remove(lshash_t *hash, const lsstr_t *key, void **value);

/**
 * Iterate over the entries of a hash table.
 * @param hash Hash table.
 * @param callback Callback function.
 * @param data Data to be passed to the callback function.
 */
void lshash_foreach(lshash_t *hash,
                    void (*callback)(const lsstr_t *, void *, void *),
                    void *data);
