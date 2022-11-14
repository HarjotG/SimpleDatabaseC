/*
 * Taken inspiration from the redis implementation of the hash table
 * https://github.com/redis/redis/blob/3.2.6/src/dict.h
 *
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

// 2^5 (or 1 << 5) = 32
#define HASHTABLE_DEFALTCAP 5

typedef enum EntryType {
    STRING,
    UNSIGNED_INT,
    SIGNED_INT,
    DOUBLE,
    NONE,
} EntryType;

typedef struct HashtableValue {
    EntryType entryType;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
} HashtableValue;

typedef struct HashtableEntry {
    void *key;
    size_t keylen;
    HashtableValue htv;
    struct HashtableEntry *next;
} HashtableEntry;

typedef struct Hashtable {
    HashtableEntry **table; /* Array of pointers to hashtable entries */
    uint64_t len;           /* Length of the table array (number of key/value pairs)*/
    unsigned char exp;      /* Size of the table array is 1<<exp (size is number of open slots) */
} Hashtable;

/**
 * Hash function for the hashtable
 *
 * @param key The key to hash
 * @param keylen The length of the key
 *
 * @returns The hash value of the key
 * */
uint64_t htHashFunction(const void *key, size_t keylen);

/**
 * Create an empty Hashtable
 *
 * @returns The empty Hashtable structure or null on error
 * */
Hashtable *htCreateTable();

/**
 * Free the hashtable structure
 *
 * @param ht The Hashtable to free
 *
 * @returns 0 if success, positive value otherwise
 * */
int htDeleteTable(Hashtable *ht);

/**
 * Get an entry from the hashtable
 *
 * @param ht The table to search
 * @param key The key
 * @param keylen The size of the key
 *
 * @returns The value if found, otherwise returns a NONE value
 * */
HashtableValue htFind(Hashtable *ht, const void *key, size_t keylen);

/**
 * Add an entry to the hashtable
 *
 * @param ht The hashtable to add to
 * @param key The key of the entry
 * @param keylen The length of the key
 * @param htv The value of the entry
 *
 * @returns 0 if insert successful, 1 if key already exists in table
 * */
int htAdd(Hashtable *ht, const void *key, size_t keylen, HashtableValue htv);

/**
 * Remove an entry in the hashtable
 *
 * @param ht The hashtable to remove the entry from
 * @param key The key of the entry
 * @param keylen The length of the key
 * */
int htRemove(Hashtable *ht, const void *key, size_t keylen);

/**
 * Replace an entry in the hashtable
 *
 * @param ht The hashtable to replace the entry from
 * @param key The key of the entry to replace
 * @param keylen The length of the key
 * @param htv The new value for the key
 */
int htReplace(Hashtable *ht, const void *key, size_t keylen, HashtableValue htv);

/**
 * Serialize the hashtable to the specified file
 *
 * @param ht The hashtable to seriailize
 * @param file The file to serialize to
 */
int htSerializeToFile(Hashtable *ht, FILE *file);

/**
 * Deserialize the hashtable from the specified file
 *
 * @param ht The hashtable to seriailize to
 * @param file The file to deserialize from
 */
int htDeserializeFromFile(Hashtable *ht, FILE *file);

#endif /* __HASHTABLE_H */