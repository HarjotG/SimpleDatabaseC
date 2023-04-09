/*
 * Taken inspiration from the redis implementation of the hash table
 * https://github.com/redis/redis/blob/3.2.6/src/dict.h
 *
 */

#pragma once

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef __HASHTABLE_H
#define __HASHTABLE_H

// 2^5 (or 1 << 5) = 32
#define HASHTABLE_DEFAULTCAP 5

typedef enum EntryType {
    STRING,
    UNSIGNED_INT,
    SIGNED_INT,
    DOUBLE,
    NONE,
} EntryType_t;

typedef struct HashtableValue_t {
    EntryType_t entryType;
    union {
        char *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
} HashtableValue_t;

typedef struct HashtableEntry {
    char *key;
    size_t keylen;
    HashtableValue_t htv;
    struct HashtableEntry *next; // Using separate chaining to handle hash-conflicts
} HashtableEntry_t;

typedef struct Hashtable {
    HashtableEntry_t **table; /* Array of pointers to hashtable entries */
    uint64_t len;             /* number of key/value pairs*/
    unsigned char exp;        /* Size of the table array is 1<<exp (size is number of open slots) */
} Hashtable_t;

/**
 * Hash function for the hashtable
 *
 * @param key The key to hash
 * @param keylen The length of the key
 *
 * @returns The hash value of the key
 * */
uint64_t htHashFunction(const char *key, size_t keylen);

/**
 * Create an empty Hashtable
 *
 * @returns The empty Hashtable structure or null on error
 * */
Hashtable_t *htCreateTable();

/**
 * Free the hashtable structure
 *
 * @param ht The Hashtable to free
 *
 * @returns 0 if success, positive value otherwise
 * */
void htDeleteTable(Hashtable_t *ht);

/**
 * Get an entry from the hashtable
 *
 * @param ht The table to search
 * @param key The key
 * @param keylen The size of the key
 *
 * @returns The value if found, otherwise returns a HashtableValue set to the NONE value
 * */
HashtableValue_t htFind(Hashtable_t *ht, const char *key, size_t keylen);

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
int htAdd(Hashtable_t *ht, const char *key, size_t keylen, HashtableValue_t htv);

/**
 * Remove an entry in the hashtable
 *
 * @param ht The hashtable to remove the entry from
 * @param key The key of the entry
 * @param keylen The length of the key
 *
 * @returns 0 if successful, 1 if there is no entry to remove
 * */
int htRemove(Hashtable_t *ht, const char *key, size_t keylen);

/**
 * Replace an entry in the hashtable. If entry does not already exist, add the entry
 *
 * @param ht The hashtable to replace the entry from
 * @param key The key of the entry to replace
 * @param keylen The length of the key
 * @param htv The new value for the key
 *
 * @returns 0
 */
int htReplace(Hashtable_t *ht, const char *key, size_t keylen, HashtableValue_t htv);

#endif /* __HASHTABLE_H */