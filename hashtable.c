#include "hashtable.h"
#include "siphash.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

static const uint8_t k[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

// compare two keys, return 1 if same 0 if not sames
static int cmpKey(const void *key1, size_t keylen1, const void *key2, size_t keylen2) {
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

static HashtableEntry *htFindEntry(Hashtable *ht, const void *key, size_t keylen) {
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry *hte = ht->table[idx];

    // search for the entry, multiple entries may have hashed to the same spot so
    // also look in the linked list
    while (hte != NULL && hte->next != NULL && !cmpKey(hte->key, hte->keylen, key, keylen)) {
        hte = hte->next;
    }
    if (hte != NULL && !cmpKey(hte->key, hte->keylen, key, keylen)) {
        return NULL;
    }
    return hte;
}

static void htExpandAndRehash(Hashtable *ht) {
    HashtableEntry **newTable = malloc((1 << ht->exp + 1) * sizeof(HashtableEntry));
    memset(newTable, 0, (1 << ht->exp + 1) * sizeof(HashtableEntry));
    for (uint64_t i = 0; i < (1 << ht->exp); i++) {
        HashtableEntry *hte = ht->table[i];
        while (hte != NULL) {
            HashtableEntry *next = hte->next;
            uint64_t idx = htHashFunction(hte->key, hte->keylen) % (1 << ht->exp + 1);

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

Hashtable *htCreateTable() {
    Hashtable *ht = malloc(sizeof(Hashtable));
    memset(ht, 0, sizeof(Hashtable));
    if (ht) {
        ht->len = 0;
        ht->exp = HASHTABLE_DEFALTCAP;
        ht->table = malloc((1 << HASHTABLE_DEFALTCAP) * sizeof(HashtableEntry));
        memset(ht->table, 0, (1 << HASHTABLE_DEFALTCAP) * sizeof(HashtableEntry));
    }

    return ht;
}

int htDeleteTable(Hashtable *ht) {
    // free all the entries in the table
    for (uint64_t i = 0; i < (1 << ht->exp); i++) {
        HashtableEntry *hte = ht->table[i];
        while (hte != NULL) {
            HashtableEntry *curr = hte;
            hte = hte->next;
            free(curr->key);
            free(curr);
        }
    }
    // free table
    free(ht->table);
    // free struct
    free(ht);
    return 0;
}

uint64_t htHashFunction(const void *key, size_t keylen) {
    // Using siphash for the hashing function
    uint64_t hash;
    siphash(key, keylen, k, (uint8_t *)&hash, sizeof(hash));
    return hash;
}

HashtableValue htFind(Hashtable *ht, const void *key, size_t keylen) {
    HashtableEntry *hte = htFindEntry(ht, key, keylen);

    if (hte == NULL) {
        // if not found, return a NONE value type
        HashtableValue htv;
        htv.entryType = NONE;
        htv.v.val = 0;
        return htv;
    }
    return hte->htv;
}

int htAdd(Hashtable *ht, const void *key, size_t keylen, HashtableValue htv) {
    // if entry already exists, no-op, return 1 to indicate entry already exists
    if (htFind(ht, key, keylen).entryType != NONE) {
        return 1;
    }
    // if more elements in hash table than size, we need to expand and re-hash
    if (ht->len == (1 << ht->exp)) {
        htExpandAndRehash(ht);
    }
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry *hte = malloc(sizeof(HashtableEntry));
    hte->key = malloc(keylen);
    memcpy(hte->key, key, keylen);
    hte->keylen = keylen;
    hte->htv.entryType = htv.entryType;
    hte->htv.v = htv.v;

    // add the entry at the top of the list
    hte->next = ht->table[idx];
    ht->table[idx] = hte;

    ht->len++;
    return 0;
}

int htRemove(Hashtable *ht, const void *key, size_t keylen) {
    uint64_t idx = htHashFunction(key, keylen) % (1 << ht->exp);
    HashtableEntry *hte = ht->table[idx];

    // search for the entry, multiple entries may have hashed to the same spot so
    // also look in the linked list
    HashtableEntry *prev = NULL;
    while (hte != NULL && hte->next != NULL && !cmpKey(hte->key, hte->keylen, key, keylen)) {
        prev = hte;
        hte = hte->next;
    }
    if (hte == NULL) {
        return 0; // nothing to remove
    }
    // remove entry
    if (prev != NULL) {
        // entry was somewhere in the linked list
        prev->next = hte->next;
    } else {
        // entry was at head of list
        ht->table[idx] = hte->next;
        ht->table[idx] = NULL;
    }
    free(hte->key);
    free(hte);

    ht->len--;
    return 0;
}

int htReplace(Hashtable *ht, const void *key, size_t keylen, HashtableValue htv) {
    HashtableEntry *hte = htFindEntry(ht, key, keylen);
    if (hte != NULL) {
        hte->htv.entryType = htv.entryType;
        hte->htv.v = htv.v;
        hte->key = realloc(hte->key, keylen);
        memcpy(hte->key, key, keylen);
    }
    return 0;
}

int htSerializeToFile(Hashtable *ht, FILE *file) {
    for (uint64_t i = 0; i < (1 << ht->exp); i++) {
        HashtableEntry *hte = ht->table[i];
        while (hte != NULL) {
            HashtableEntry *curr = hte;
            hte = hte->next;
            fprintf(file, "%ld ", curr->keylen);
            // write out the key
            for (size_t i = 0; i < curr->keylen; i++) {
                fprintf(file, "%c", *((uint8_t *)curr->key + i));
            }
            fprintf(file, " %d", curr->htv.entryType);
            switch (curr->htv.entryType) {
            case STRING:
                fprintf(file, " %s\n", (char *)curr->htv.v.val);
                break;
            case UNSIGNED_INT:
                fprintf(file, " %ld\n", curr->htv.v.u64);
                break;
            case SIGNED_INT:
                fprintf(file, " %ld\n", curr->htv.v.s64);
                break;
            case DOUBLE:
                fprintf(file, " %lf\n", curr->htv.v.d);
                break;
            }
        }
    }
    return 0;
}

int htDeserializeFromFile(Hashtable *ht, FILE *file) {
    char *line = NULL;
    ssize_t lineBuffSize = 0;
    while (getline(&line, &lineBuffSize, file) > 0) {
        HashtableValue htv;
        // key length
        size_t keylen = atol(strtok(line, " "));
        // get the key
        char *key = strtok(NULL, " ");
        // entry type
        htv.entryType = atoi(strtok(NULL, " "));
        // value
        char *val = strtok(NULL, "\n");

        switch (htv.entryType) {
        case STRING:
            htv.v.val = val;
            break;
        case UNSIGNED_INT:
            htv.v.u64 = atol(val);
            break;
        case SIGNED_INT:
            htv.v.s64 = atol(val);
            break;
        case DOUBLE:
            htv.v.d = atof(val);
            break;
        }
        htAdd(ht, key, keylen, htv);
    }
    return 0;
}
