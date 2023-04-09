#include "../src/hashtable.h"
#include "../src/network.h"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

pid_t serverPid = -1;

/* Helper functions*/
void createServerProcess() {
    serverPid = fork();
    if (serverPid == -1) {
        printf("Error creating server process %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (serverPid == 0) {
        // in child process
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        char *argv[2] = {"db", NULL};
        if (execv("./db", argv) == -1) {
            printf("Error executing server on created process: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    } else {
        // in parent process, nothing to do
    }
}

void killServerProcess() {
    if (kill(serverPid, SIGTERM) == -1) {
        printf("Error killing server process %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

int createSocketToServer() {
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        printf("Error creating socket\n");
        return -1;
    }

    struct sockaddr_in localServer;

    localServer.sin_family = AF_INET;
    localServer.sin_port = htons(SERVER_DEFAULT_PORT);
    localServer.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(socketFd, (const struct sockaddr *)&localServer, sizeof(localServer)) < 0) {
        printf("Error conecting socket to server: %d\n", errno);
        return -1;
    }
    return socketFd;
}

/* Tests*/
void testCreateHashtable() {
    Hashtable_t *ht = htCreateTable();
    assert(ht != NULL);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 0);
    assert(ht->table != NULL);
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        assert(ht->table[i] == NULL);
    }
    htDeleteTable(ht);
}

void testAddInt() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = SIGNED_INT;
    htv.v.s64 = -987453;
    assert(htAdd(ht, "KeyForInt", 10, htv) == 0);
    assert(ht->len == 1);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);

    uint64_t idx = htHashFunction("KeyForInt", 10) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];
    assert(hte->htv.entryType == SIGNED_INT);
    assert(hte->htv.v.s64 == -987453);
    htDeleteTable(ht);
}

void testAddUint() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = UNSIGNED_INT;
    htv.v.u64 = 786786;
    assert(htAdd(ht, "KeyForUint", 11, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    uint64_t idx = htHashFunction("KeyForUint", 11) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];
    assert(hte->htv.entryType == UNSIGNED_INT);
    assert(hte->htv.v.u64 == 786786);
    htDeleteTable(ht);
}

void testAddDouble() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = DOUBLE;
    htv.v.d = 78676.124168;
    assert(htAdd(ht, "KeyForDouble", 13, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    uint64_t idx = htHashFunction("KeyForDouble", 13) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];
    assert(hte->htv.entryType == DOUBLE);
    assert(hte->htv.v.d == 78676.124168);
    htDeleteTable(ht);
}

void testAddString() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "String Value";
    assert(htAdd(ht, "KeyForString", 13, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    uint64_t idx = htHashFunction("KeyForString", 13) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];
    assert(hte->htv.entryType == STRING);
    assert(strcmp("String Value", hte->htv.v.val) == 0);
    htDeleteTable(ht);
}

void testAddDupe() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "String Value";
    assert(htAdd(ht, "KeyForString", 13, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    HashtableValue_t htv2;
    htv2.entryType = DOUBLE;
    htv2.v.d = 4545.354354;
    assert(htAdd(ht, "KeyForString", 13, htv2) == 1);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    uint64_t idx = htHashFunction("KeyForString", 13) % (1 << ht->exp);
    HashtableEntry_t *hte = ht->table[idx];
    assert(hte->htv.entryType == STRING);
    assert(strcmp("String Value", hte->htv.v.val) == 0);
    htDeleteTable(ht);
}

void testFindInt() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = SIGNED_INT;
    htv.v.s64 = -987453;
    assert(htAdd(ht, "KeyForInt", 10, htv) == 0);
    assert(ht->len == 1);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);

    HashtableValue_t htvFind = htFind(ht, "KeyForInt", 10);
    assert(htvFind.entryType == SIGNED_INT);
    assert(htvFind.v.s64 == -987453);
    htDeleteTable(ht);
}

void testFindUint() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = UNSIGNED_INT;
    htv.v.u64 = 786786;
    assert(htAdd(ht, "KeyForUint", 11, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    HashtableValue_t htvFind = htFind(ht, "KeyForUint", 11);
    assert(htvFind.entryType == UNSIGNED_INT);
    assert(htvFind.v.u64 == 786786);
    htDeleteTable(ht);
}

void testFindDouble() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = DOUBLE;
    htv.v.d = 78676.124168;
    assert(htAdd(ht, "KeyForDouble", 13, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    HashtableValue_t htvFind = htFind(ht, "KeyForDouble", 13);
    assert(htvFind.entryType == DOUBLE);
    assert(htvFind.v.d == 78676.124168);
    htDeleteTable(ht);
}

void testFindString() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "String Value";
    assert(htAdd(ht, "Key For String With Space", 26, htv) == 0);
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    assert(ht->len == 1);

    HashtableValue_t htvFind = htFind(ht, "Key For String With Space", 26);
    assert(htvFind.entryType == STRING);
    assert(strcmp("String Value", htvFind.v.val) == 0);
    htDeleteTable(ht);
}

void testFindMany() {
    Hashtable_t *ht = htCreateTable();
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue_t htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        assert(htAdd(ht, (char *)&i, sizeof(i), htv) == 0);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == HASHTABLE_DEFAULTCAP);

    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue_t htv = htFind(ht, (char *)&i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }
    htDeleteTable(ht);
}

void testFindManyCausesRehash() {
    Hashtable_t *ht = htCreateTable();
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP) + 1; i++) {
        HashtableValue_t htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        assert(htAdd(ht, (char *)&i, sizeof(i), htv) == 0);
        assert(ht->len == i + 1);
        assert(ht->len <= (1 << ht->exp));
    }
    // the hashtable must have been expanded and re-hashed to fit all the items
    // We expand and re-hash whenever a new item will make ht->len > (1 << ht->exp)
    assert(ht->exp == HASHTABLE_DEFAULTCAP + 1);

    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP) + 1; i++) {
        HashtableValue_t htv = htFind(ht, (char *)&i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }

    htDeleteTable(ht);
}

void testFindManyMore() {
    Hashtable_t *ht = htCreateTable();
    assert(ht->exp == HASHTABLE_DEFAULTCAP);
    for (int i = 0; i < 999; i++) {
        HashtableValue_t htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        assert(htAdd(ht, (char *)&i, sizeof(i), htv) == 0);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == HASHTABLE_DEFAULTCAP + 5);

    for (int i = 0; i < 999; i++) {
        HashtableValue_t htv = htFind(ht, (char *)&i, sizeof(i));
        assert(htv.entryType == SIGNED_INT);
        assert(htv.v.s64 == i * 10);
    }

    htDeleteTable(ht);
}

void testFindNone() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htvFind = htFind(ht, "Key That doesn't exist", 23);
    assert(htvFind.entryType == NONE);
    htDeleteTable(ht);
}

void testRemoveValue() {
    Hashtable_t *ht = htCreateTable();
    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    assert(htAdd(ht, "first key", 10, htv) == 0);
    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(strcmp(htvret.v.val, "Test value string\n") == 0);
    assert(htRemove(ht, "first key", 10) == 0);
    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == NONE);
    htDeleteTable(ht);
}

void testRemoveAllValues() {
    Hashtable_t *ht = htCreateTable();
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue_t htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        assert(htAdd(ht, (char *)&i, sizeof(i), htv) == 0);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == HASHTABLE_DEFAULTCAP);

    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        assert(htRemove(ht, (char *)&i, sizeof(i)) == 0);
    }

    // check no values can be found again
    for (int i = 0; i < (1 << HASHTABLE_DEFAULTCAP); i++) {
        HashtableValue_t htv = htFind(ht, (char *)&i, sizeof(i));
        assert(htv.entryType == NONE);
        assert(htv.v.s64 == 0);
    }
    htDeleteTable(ht);
}

void testRemoveAllValuesAfterRehash() {
    Hashtable_t *ht = htCreateTable();
    for (int i = 0; i < 50; i++) {
        HashtableValue_t htv;
        htv.entryType = SIGNED_INT;
        htv.v.s64 = i * 10;
        assert(htAdd(ht, (char *)&i, sizeof(i), htv) == 0);
        assert(ht->len == i + 1);
    }
    assert(ht->exp == 6);

    for (int i = 0; i < 50; i++) {
        assert(htRemove(ht, (char *)&i, sizeof(i)) == 0);
    }

    // check no values can be found again
    for (int i = 0; i < 50; i++) {
        HashtableValue_t htv = htFind(ht, (char *)&i, sizeof(i));
        assert(htv.entryType == NONE);
        assert(htv.v.s64 == 0);
    }
    htDeleteTable(ht);
}

void testRemoveNonExistent() {
    Hashtable_t *ht = htCreateTable();
    assert(htRemove(ht, "first key", 10) == 1);

    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == NONE);
    htDeleteTable(ht);
}

void testReplace() {
    Hashtable_t *ht = htCreateTable();

    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    assert(htAdd(ht, "first key", 10, htv) == 0);

    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(strcmp(htvret.v.val, "Test value string\n") == 0);

    HashtableValue_t htvNew;
    htvNew.entryType = DOUBLE;
    htvNew.v.d = 123.456;
    assert(htReplace(ht, "first key", 10, htvNew) == 0);

    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == DOUBLE);
    assert(htvret.v.d == 123.456);

    htDeleteTable(ht);
}

void testReplaceMultiple() {
    Hashtable_t *ht = htCreateTable();

    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    assert(htAdd(ht, "first key", 10, htv) == 0);

    HashtableValue_t htv2;
    htv2.entryType = UNSIGNED_INT;
    htv2.v.u64 = 45615;
    assert(htAdd(ht, "second key", 11, htv2) == 0);

    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(strcmp(htvret.v.val, "Test value string\n") == 0);

    HashtableValue_t htvret2 = htFind(ht, "second key", 11);
    assert(htvret2.entryType == UNSIGNED_INT);
    assert(htvret2.v.u64 == 45615);

    HashtableValue_t htvNew;
    htvNew.entryType = DOUBLE;
    htvNew.v.d = 123.456;
    assert(htReplace(ht, "first key", 10, htvNew) == 0);

    HashtableValue_t htvNew2;
    htvNew2.entryType = DOUBLE;
    htvNew2.v.d = 456.7890;
    assert(htReplace(ht, "second key", 11, htvNew2) == 0);

    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == DOUBLE);
    assert(htvret.v.d == 123.456);

    htvret2 = htFind(ht, "second key", 11);
    assert(htvret2.entryType == DOUBLE);
    assert(htvret2.v.d == 456.7890);
    htDeleteTable(ht);
}

void testReplaceThenRemove() {
    Hashtable_t *ht = htCreateTable();

    HashtableValue_t htv;
    htv.entryType = STRING;
    htv.v.val = "Test value string\n";
    assert(htAdd(ht, "first key", 10, htv) == 0);

    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == STRING);
    assert(strcmp(htvret.v.val, "Test value string\n") == 0);

    HashtableValue_t htvNew;
    htvNew.entryType = DOUBLE;
    htvNew.v.d = 123.456;
    assert(htReplace(ht, "first key", 10, htvNew) == 0);

    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == DOUBLE);
    assert(htvret.v.d == 123.456);

    assert(htRemove(ht, "first key", 10) == 0);

    htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == NONE);

    htDeleteTable(ht);
}

void testReplaceNonExistent() {
    Hashtable_t *ht = htCreateTable();

    HashtableValue_t htv;
    htv.entryType = DOUBLE;
    htv.v.d = 123.456;
    assert(htReplace(ht, "first key", 10, htv) == 0);

    HashtableValue_t htvret = htFind(ht, "first key", 10);
    assert(htvret.entryType == DOUBLE);
    assert(htvret.v.d == 123.456);

    htDeleteTable(ht);
}

void testCreateServer() {
    Server_t *server = createServer(12345);
    assert(server->serverFd > 0);
    assert(server->port == 12345);
    destroyServer(server);
}

void testServerInsertString() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "insert thisIsTheKey string thisIsTheString";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", serverReply);
    assert(strcmp("Value inserted successfully", serverReply) == 0);
    close(socketFd);
}

void testServerInsertInt() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "insert keyforint int -455";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);
    close(socketFd);
}

void testServerInsertUint() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "insert keyforunsignedint uint 5205";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);
    close(socketFd);
}

void testServerInsertDouble() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "insert keyfordouble double 448654.556465";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);
    close(socketFd);
}

void testServerInsertMultiple() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command1[] = "insert keyfordouble2 double 448654.556465";
    char command2[] = "insert stringkey465 string thisisastring";
    char command3[] = "insert keyforint421 int 448654";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    close(socketFd);
}

void testInsertDoubleFail() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "insert keydupe string stringvalue";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key keydupe already exists", serverReply) == 0);
    close(socketFd);
}

void testServerSelectString() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerSelectString string this Is The String For Test";
    char command2[] = "select testServerSelectString";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("%s\n", serverReply);
    assert(strcmp("{testServerSelectString: this Is The String For Test}", serverReply) == 0);
    close(socketFd);
}

void testServerSelectInt() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerSelectInt int -975361";
    char command2[] = "select testServerSelectInt";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerSelectInt: -975361}", serverReply) == 0);
    close(socketFd);
}

void testServerSelectUint() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerSelectUint uint 48521";
    char command2[] = "select testServerSelectUint";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerSelectUint: 48521}", serverReply) == 0);
    close(socketFd);
}

void testServerSelectDouble() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerSelectDouble double 448654.55645";
    char command2[] = "select testServerSelectDouble";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    // assert on string comparison before the last } because of double imprecision
    assert(strncmp("{testServerSelectDouble: 448654.55645}", serverReply, 37) == 0);
    close(socketFd);
}

void testServerSelectKeyNotFound() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "select thisKeyDoesn'tExist";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key not found", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteString() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerDeleteString string thisIsTheStringForTest25";
    char command2[] = "delete testServerDeleteString";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteInt() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerDeleteInt int -13846";
    char command2[] = "delete testServerDeleteInt";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteUint() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerDeleteUint uint 8945";
    char command2[] = "delete testServerDeleteUint";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteDouble() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerDeleteDouble double 448654.4561516";
    char command2[] = "delete testServerDeleteDouble";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteKeyNotFound() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command[] = "delete thisKeyForDeleteDoesn'tExist";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command, sizeof(command), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key not found", serverReply) == 0);
    close(socketFd);
}

void testServerDeleteMultiple() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command1[] = "insert keyfordouble3 double 4123.556465";
    char command2[] = "insert stringkey3 string thisisastringtobedeleted";
    char command3[] = "insert keyforint3 int 4536";
    char command4[] = "delete keyfordouble3";
    char command5[] = "delete stringkey3";
    char command6[] = "delete keyforint3";
    char command7[] = "select keyfordouble3";
    char command8[] = "select stringkey3";
    char command9[] = "select keyforint3";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command4, sizeof(command4), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command5, sizeof(command5), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command6, sizeof(command6), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key removed successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command7, sizeof(command7), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key not found", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command8, sizeof(command8), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key not found", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command9, sizeof(command9), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key not found", serverReply) == 0);

    close(socketFd);
}

void testServerReplaceString() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerReplaceString string thisIsTheStringForTest25";
    char command2[] = "replace testServerReplaceString string thisIsTheReplacedString";
    char command3[] = "select testServerReplaceString";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceString: thisIsTheReplacedString}", serverReply) == 0);
    close(socketFd);
}

void testServerReplaceInt() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerReplaceInt int -13846";
    char command2[] = "replace testServerReplaceInt double 654.354465";
    char command3[] = "select testServerReplaceInt";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    // assert on string comparison before the last } because of double imprecision
    assert(strncmp("{testServerReplaceInt: 654.354465}", serverReply, 33) == 0);
    close(socketFd);
}

void testServerReplaceUint() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerReplaceUint uint 8945";
    char command2[] = "replace testServerReplaceUint uint 123456";
    char command3[] = "select testServerReplaceUint";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceUint: 123456}", serverReply) == 0);
    close(socketFd);
}

void testServerReplaceDouble() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }

    char command1[] = "insert testServerReplaceDouble double 448654.4561516";
    char command2[] = "replace testServerReplaceDouble string this Is the replaced string";
    char command3[] = "select testServerReplaceDouble";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceDouble: this Is the replaced string}", serverReply) == 0);
    close(socketFd);
}

void testServerReplaceKeyNotFound() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command1[] = "replace testServerReplaceKeyNotFound int -87921";
    char command2[] = "select testServerReplaceKeyNotFound";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceKeyNotFound: -87921}", serverReply) == 0);
    close(socketFd);
}

void testServerReplaceMultiple() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command1[] = "insert testServerReplaceMultiple double 4123.556465";
    char command2[] = "replace testServerReplaceMultiple string replaced string";
    char command3[] = "select testServerReplaceMultiple";
    char command4[] = "replace testServerReplaceMultiple uint 81565";
    char command5[] = "select testServerReplaceMultiple";
    char command6[] = "replace testServerReplaceMultiple double 4123.556465";
    char command7[] = "select testServerReplaceMultiple";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Value inserted successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceMultiple: replaced string}", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command4, sizeof(command4), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command5, sizeof(command5), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceMultiple: 81565}", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command6, sizeof(command6), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Key replaced successfully", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command7, sizeof(command7), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("{testServerReplaceMultiple: 4123.556465}", serverReply) == 0);

    close(socketFd);
}

void testServerMalformedQueries() {
    int socketFd = createSocketToServer();
    if (socketFd == -1) {
        exit(EXIT_FAILURE);
    }
    char command1[] = "insert key";
    char command2[] = "select";
    char command3[] = "replace key ";
    char command4[] = "delete";
    char serverReply[BUFFER_SIZE];
    memset(serverReply, 0, BUFFER_SIZE);

    if (send(socketFd, command1, sizeof(command1), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Malformed query", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command2, sizeof(command2), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Malformed query", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command3, sizeof(command3), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Malformed query", serverReply) == 0);

    memset(serverReply, 0, BUFFER_SIZE);
    if (send(socketFd, command4, sizeof(command4), 0) < 0) {
        printf("Sending data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    if (recv(socketFd, serverReply, BUFFER_SIZE, 0) < 0) {
        printf("Receiving data over socket failed %d\n", errno);
        exit(EXIT_FAILURE);
    }
    assert(strcmp("Malformed query", serverReply) == 0);
    close(socketFd);
}

int main(void) {
    /* Pre-test inits*/
    createServerProcess();

    /* Tests*/
    testCreateHashtable();

    testAddInt();
    testAddUint();
    testAddDouble();
    testAddString();
    testAddDupe();

    testFindInt();
    testFindUint();
    testFindDouble();
    testFindString();
    testFindMany();
    testFindManyCausesRehash();
    testFindManyMore();
    testFindNone();

    testRemoveValue();
    testRemoveAllValues();
    testRemoveAllValuesAfterRehash();
    testRemoveNonExistent();

    testReplace();
    testReplaceMultiple();
    testReplaceThenRemove();
    testReplaceNonExistent();

    testCreateServer();
    testServerInsertString();
    testServerInsertInt();
    testServerInsertUint();
    testServerInsertDouble();
    testServerInsertMultiple();
    testInsertDoubleFail();

    testServerSelectString();
    testServerSelectInt();
    testServerSelectUint();
    testServerSelectDouble();
    testServerSelectKeyNotFound();

    testServerDeleteString();
    testServerDeleteInt();
    testServerDeleteUint();
    testServerDeleteDouble();
    testServerDeleteMultiple();
    testServerDeleteKeyNotFound();

    testServerReplaceString();
    testServerReplaceInt();
    testServerReplaceUint();
    testServerReplaceDouble();
    testServerReplaceMultiple();
    testServerReplaceKeyNotFound();

    testServerMalformedQueries();

    /* Post-test cleanup*/
    killServerProcess();

    printf("Passed!\n");
    return 0;
}