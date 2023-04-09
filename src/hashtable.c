#include "hashtable.h"
#include "siphash.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

static const uint8_t k[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// compare two keys, return 1 if same 0 if not sames
static int cmpKey(const char *key1, size_t keylen1, const char *key2, size_t keylen2) {
    if (keylen1 != keylen2) {
        return 0;
    }
    for (size_t i = 0; i < keylen1; i++) {
        if (((uint8_t *)key1)[i] != ((uint8_t *)key2)[i]) {
            return 0;
        }
    }
    return 1;
}

static HashtableEntry_t *htFindEntry(Hashtable_t *ht, const char *key, size_t keylen) {
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];

    // search for the entry, multiple entries may have hashed to the same spot so
    // also look in the linked list
    while (hte != NULL && hte->next != NULL && !cmpKey(hte->key, hte->keylen, key, keylen)) {
        hte = hte->next;
    }
    if (hte == NULL || !cmpKey(hte->key, hte->keylen, key, keylen)) {
        return NULL;
    }
    return hte;
}

static void htExpandAndRehash(Hashtable_t *ht) {
    HashtableEntry_t **newTable = malloc((1 << (ht->exp + 1)) * sizeof(HashtableEntry_t));
    memset(newTable, 0, (1 << (ht->exp + 1)) * sizeof(HashtableEntry_t));
    for (uint64_t i = 0; i < (1 << ht->exp); i++) {
        HashtableEntry_t *hte = ht->table[i];
        while (hte != NULL) {
            HashtableEntry_t *next = hte->next;
            uint64_t idx = htHashFunction(hte->key, hte->keylen) % (1 << (ht->exp + 1));

            // add the entry at the top of the list
            hte->next = newTable[idx];
            newTable[idx] = hte;
            hte = next;
        }
    }
    free(ht->table);
    ht->table = newTable;
    ht->exp++;
}

Hashtable_t *htCreateTable() {
    Hashtable_t *ht = malloc(sizeof(Hashtable_t));
    memset(ht, 0, sizeof(Hashtable_t));
    if (ht) {
        ht->len = 0;
        ht->exp = HASHTABLE_DEFAULTCAP;
        ht->table = malloc((1 << HASHTABLE_DEFAULTCAP) * sizeof(HashtableEntry_t));
        memset(ht->table, 0, (1 << HASHTABLE_DEFAULTCAP) * sizeof(HashtableEntry_t));
    }

    return ht;
}

void htDeleteTable(Hashtable_t *ht) {
    // free all the entries in the table
    for (uint64_t i = 0; i < (1 << ht->exp); i++) {
        HashtableEntry_t *hte = ht->table[i];
        while (hte != NULL) {
            HashtableEntry_t *curr = hte;
            hte = hte->next;
            if (curr->htv.entryType == STRING) {
                free(curr->htv.v.val);
            }
            free(curr->key);
            free(curr);
        }
    }
    // free table
    free(ht->table);
    // free struct
    free(ht);
}

uint64_t htHashFunction(const char *key, size_t keylen) {
    // Using siphash for the hashing function
    uint64_t hash;
    siphash(key, keylen, k, (uint8_t *)&hash, sizeof(hash));
    return hash;
}

HashtableValue_t htFind(Hashtable_t *ht, const char *key, size_t keylen) {
    HashtableEntry_t *hte = htFindEntry(ht, key, keylen);

    if (hte == NULL) {
        // if not found, return a NONE value type
        HashtableValue_t htv;
        htv.entryType = NONE;
        htv.v.val = 0;
        return htv;
    }
    return hte->htv;
}

int htAdd(Hashtable_t *ht, const char *key, size_t keylen, HashtableValue_t htv) {
    // if entry already exists, no-op, return 1 to indicate entry already exists
    if (htFind(ht, key, keylen).entryType != NONE) {
        return 1;
    }
    // if more elements in hash table than size, we need to expand and re-hash
    if (ht->len == (1 << ht->exp)) {
        htExpandAndRehash(ht);
    }
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry_t *hte = malloc(sizeof(HashtableEntry_t));
    hte->key = malloc(keylen);
    memcpy(hte->key, key, keylen);
    hte->keylen = keylen;
    hte->htv.entryType = htv.entryType;
    if (htv.entryType == STRING) {
        hte->htv.v.val = malloc(strlen(htv.v.val));
        strcpy(hte->htv.v.val, htv.v.val);
    } else {
        hte->htv.v = htv.v;
    }

    // add the entry at the top of the list
    hte->next = ht->table[idx];
    ht->table[idx] = hte;

    ht->len++;
    return 0;
}

int htRemove(Hashtable_t *ht, const char *key, size_t keylen) {
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];

    // search for the entry, multiple entries may have hashed to the same spot so
    // also look in the linked list
    HashtableEntry_t *prev = NULL;
    while (hte != NULL && !cmpKey(hte->key, hte->keylen, key, keylen)) {
        prev = hte;
        hte = hte->next;
    }
    if (hte == NULL) {
        return 1; // nothing to remove
    }
    // remove entry
    if (prev != NULL) {
        // entry was somewhere in the linked list
        prev->next = hte->next;
    } else {
        // entry was at head of list
        ht->table[idx] = hte->next;
    }
    free(hte->key);
    hte->key = NULL;
    free(hte);

    ht->len--;
    return 0;
}

int htReplace(Hashtable_t *ht, const char *key, size_t keylen, HashtableValue_t htv) {
    HashtableEntry_t *hte = htFindEntry(ht, key, keylen);
    if (hte != NULL) {
        hte->htv.entryType = htv.entryType;
        if (htv.entryType == STRING) {
            hte->htv.v.val = malloc(strlen(htv.v.val));
            strcpy(hte->htv.v.val, htv.v.val);
        } else {
            hte->htv.v = htv.v;
        }
        hte->key = realloc(hte->key, keylen);
        hte->keylen = keylen;
        memcpy(hte->key, key, keylen);
    } else {
        htAdd(ht, key, keylen, htv);
    }
    return 0;
}
