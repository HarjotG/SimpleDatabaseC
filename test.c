#include "hashtable.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void test1() {
    Hashtable *ht = htCreateTable();
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        htAdd(ht, &i, sizeof(i), htv);
        assert(ht->len == i + 1);
    }
    // check if the values can be found and if they match inserted vals
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue htv = htFind(ht, &i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }
    htDeleteTable(ht);
}

void test2() {
    Hashtable *ht = htCreateTable();
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    for (int i = 0; i < 999; i++) {
        HashtableValue htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        htAdd(ht, &i, sizeof(i), htv);
        assert(ht->len == i + 1);
    }
    // check if the values can be found and if they match inserted vals
    for (int i = 0; i < 999; i++) {
        HashtableValue htv = htFind(ht, &i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }
    assert(ht->exp == 10);
    htDeleteTable(ht);
}

void test3() {
    Hashtable *ht = htCreateTable();
    HashtableValue htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    htAdd(ht, "first key", 10, htv);
    HashtableValue htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(htvret.v.val == "Test value string\n");
    htRemove(ht, "first key", 10);
    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == NONE);
    htDeleteTable(ht);
}

void test4() {
    Hashtable *ht = htCreateTable();
    for (int i = 0; i < 50; i++) {
        HashtableValue htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        htAdd(ht, &i, sizeof(i), htv);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == 6);
    // remove all values check if the values can be found and if they match inserted vals
    for (int i = 0; i < 50; i++) {
        htRemove(ht, &i, sizeof(i));
    }

    // check no values can be found again
    for (int i = 0; i < 50; i++) {
        HashtableValue htv = htFind(ht, &i, sizeof(i));
        assert(htv.entryType == NONE);
        assert(htv.v.s64 == 0);
    }
    htDeleteTable(ht);
}

void test5() {
    Hashtable *ht = htCreateTable();
    HashtableValue htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    htAdd(ht, "first key", 10, htv);
    HashtableValue htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(htvret.v.val == "Test value string\n");
    HashtableValue htvNew;
    htvNew.entryType = DOUBLE;
    htvNew.v.d = 123.456;
    htReplace(ht, "first key", 10, htvNew);
    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == DOUBLE);
    assert(htvret.v.d == 123.456);
    htDeleteTable(ht);
}

void test6() {
    Hashtable *ht = htCreateTable();
    for (int i = 0; i < 32; i++) {
        HashtableValue htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        htAdd(ht, &i, sizeof(i), htv);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == 5);
    HashtableValue htvResize;
    htvResize.entryType = SIGNED_INT;
    htvResize.v.s64 = 320;
    int i_32key = 32;
    htAdd(ht, &i_32key, sizeof(i_32key), htvResize);
    assert(ht->exp == 6);

    // check all values can be found again
    for (int i = 0; i < 33; i++) {
        HashtableValue htv = htFind(ht, &i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }
    htDeleteTable(ht);
}

void test7() {
    Hashtable *ht = htCreateTable();
    Hashtable *ht2 = htCreateTable();
    char key[255];
    memset(key, 0, 255);
    for (int i = 0; i < 32; i++) {
        HashtableValue htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        sprintf(key, "%d", i);
        htAdd(ht, key, i < 10 ? 1 : 2, htv);
        assert(ht->len == i + 1);
    }
    FILE *filew = fopen("test.txt", "w");
    htSerializeToFile(ht, filew);
    fclose(filew);

    FILE *filer = fopen("test.txt", "r");
    htDeserializeFromFile(ht2, filer);
    fclose(filer);

    // check all values can be found again
    memset(key, 0, 255);
    for (int i = 0; i < 32; i++) {
        sprintf(key, "%i", i);
        HashtableValue htv = htFind(ht2, key, i < 10 ? 1 : 2);
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }
    htDeleteTable(ht);
    htDeleteTable(ht2);
}

int main(void) {
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    printf("Passed!\n");
    return 0;
}