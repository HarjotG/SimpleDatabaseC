#include "hashtable.h"
#include "network.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct Command {
    char *query;
    char *type;
    char *key;
    char *value;
} Command_t;

// Implementation of the database
Hashtable_t *ht;
// Implementation of the server
Server_t *server;

int getKeyType(char *type) {
    if (strcmp(type, "string") == 0) {
        return STRING;
    } else if (strcmp(type, "uint") == 0) {
        return UNSIGNED_INT;
    } else if (strcmp(type, "int") == 0) {
        return SIGNED_INT;
    } else if (strcmp(type, "double") == 0) {
        return DOUBLE;
    }
}

int executeInsertCommand(Hashtable_t *ht, Command_t *command, char *commandResult) {
    HashtableValue_t htv;
    char *err = NULL;
    int retVal;
    switch (getKeyType(command->type)) {
    case STRING:
        htv.entryType = STRING;
        htv.v.val = command->value;
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            sprintf(commandResult, "Key %s already exists", command->key);
        }
        break;
    case UNSIGNED_INT:
        htv.entryType = UNSIGNED_INT;
        htv.v.u64 = (uint64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            retVal = 1;
            sprintf(commandResult, "Error inserting key");
        } else {
            retVal = htAdd(ht, command->key, strlen(command->key), htv);
            if (retVal == 1) {
                sprintf(commandResult, "Key %s already exists", command->key);
            }
        }
        break;
    case SIGNED_INT:
        htv.entryType = SIGNED_INT;
        htv.v.s64 = (int64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            retVal = 1;
            sprintf(commandResult, "Error inserting key");
        } else {
            retVal = htAdd(ht, command->key, strlen(command->key), htv);
            if (retVal == 1) {
                sprintf(commandResult, "Key %s already exists", command->key);
            }
            break;
        }
    case DOUBLE:
        htv.entryType = DOUBLE;
        htv.v.d = strtod(command->value, &err);
        if (strcmp(err, "") != 0) {
            retVal = 1;
            sprintf(commandResult, "Error inserting key");
        }
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            sprintf(commandResult, "Key %s already exists", command->key);
        }
        break;
    default:
        retVal = 1;
        break;
    }
    if (retVal == 0) {
        sprintf(commandResult, "Value inserted successfully");
    }
    return retVal;
}

int executeSelectCommand(Hashtable_t *ht, Command_t *command, char *commandResult) {
    HashtableValue_t htv = htFind(ht, command->key, strlen(command->key));
    if (htv.entryType == NONE) {
        sprintf(commandResult, "Key not found");
        return 0;
    }
    switch (htv.entryType) {
    case STRING:
        sprintf(commandResult, "{%s: %s}", command->key, (char *)htv.v.val);
        return 0;
    case UNSIGNED_INT:
        sprintf(commandResult, "{%s: %ld}", command->key, htv.v.u64);
        return 0;
    case SIGNED_INT:
        sprintf(commandResult, "{%s: %ld}", command->key, htv.v.s64);
        return 0;
    case DOUBLE:
        sprintf(commandResult, "{%s: %lf}", command->key, htv.v.d);
        return 0;
    default:
        return 1;
    }
}

int executeDeleteCommand(Hashtable_t *ht, Command_t *command, char *commandResult) {
    int retval = htRemove(ht, command->key, strlen(command->key));
    if (retval == 0) {
        sprintf(commandResult, "Key removed successfully");
    } else if (retval == 1) {
        sprintf(commandResult, "Key not found");
    }
    return retval;
}

int executeReplaceCommand(Hashtable_t *ht, Command_t *command, char *commandResult) {
    HashtableValue_t htv;
    char *err = NULL;
    int retval;
    switch (getKeyType(command->type)) {
    case STRING:
        htv.entryType = STRING;
        htv.v.val = command->value;
        retval = htReplace(ht, command->key, strlen(command->key), htv);
        break;
    case UNSIGNED_INT:
        htv.entryType = UNSIGNED_INT;
        htv.v.u64 = (uint64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            retval = 1;
        } else {
            retval = htReplace(ht, command->key, strlen(command->key), htv);
        }
        break;
    case SIGNED_INT:
        htv.entryType = SIGNED_INT;
        htv.v.s64 = (int64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            retval = 1;
        } else {
            retval = htReplace(ht, command->key, strlen(command->key), htv);
        }
        break;
    case DOUBLE:
        htv.entryType = DOUBLE;
        htv.v.d = strtod(command->value, &err);
        if (strcmp(err, "") != 0) {
            retval = 1;
        } else {
            retval = htReplace(ht, command->key, strlen(command->key), htv);
        }
        break;
    default:
        retval = 1;
        break;
    }
    if (retval == 0) {
        sprintf(commandResult, "Key replaced successfully");
    } else {
        sprintf(commandResult, "Error replacing key");
    }
    return retval;
}

void closeDb() {
    printf("Closing database...\n");
    htDeleteTable(ht);
    destroyServer(server);
    exit(0);
}

int executeDbCommand(const char *inputStatement, int inputStatementSize, Hashtable_t *ht, char *commandResult) {
    Command_t command;
    int retval;
    char statementCopy[inputStatementSize];
    memcpy(statementCopy, inputStatement, inputStatementSize);
    char *saveptr;
    if (statementCopy[inputStatementSize - 1] == '\n') {
        // make sure statement is null terminated
        statementCopy[inputStatementSize - 1] = '\0';
    }
    command.query = strtok_r(statementCopy, " ", &saveptr);
    if (command.query != NULL) {
        command.key = strtok_r(NULL, " ", &saveptr);
    } else {
        command.key = NULL;
        retval = 1;
        sprintf(commandResult, "Malformed query");
    }
    if (command.key != NULL) {
        command.type = strtok_r(NULL, " ", &saveptr);
    } else {
        command.type = NULL;
        retval = 1;
        sprintf(commandResult, "Malformed query");
    }
    if (command.type != NULL) {
        // get end of input as value
        command.value = command.type + strlen(command.type) + 1;
    } else {
        command.value = NULL;
        if (strncmp(command.query, "insert", 6) == 0 || strncmp(command.query, "replace", 7) == 0) {
            retval = 1;
            sprintf(commandResult, "Malformed query");
        }
    }
    if (retval != 1) {
        if (strncmp(command.query, "insert", 6) == 0) {
            retval = executeInsertCommand(ht, &command, commandResult);
        } else if (strncmp(command.query, "select", 6) == 0) {
            retval = executeSelectCommand(ht, &command, commandResult);
        } else if (strncmp(command.query, "delete", 6) == 0) {
            retval = executeDeleteCommand(ht, &command, commandResult);
        } else if (strncmp(command.query, "replace", 7) == 0) {
            retval = executeReplaceCommand(ht, &command, commandResult);
        } else {
            retval = 1;
            sprintf(commandResult, "Query not supported");
        }
    }
    return retval;
}

void onData(int clientFd, const char *data, int size, struct sockaddr *addr, socklen_t addrLen) {
    char commandResult[BUFFER_SIZE];
    memset(commandResult, 0, BUFFER_SIZE);
    if (executeDbCommand(data, size, ht, commandResult) != 0) {
        printf("Error completing command\n");
    } else {
        printf("Command completed successfully\n");
    }
    sendClientData(clientFd, commandResult, strlen(commandResult));
}

int main(int argc, char *argv[]) {
    // close program on Ctrl-C
    signal(SIGINT, closeDb);
    signal(SIGTERM, closeDb);

    server = createServer(SERVER_DEFAULT_PORT);
    if (server != NULL) {
        ht = htCreateTable();
        runServer(server, onData);
    }
    // if we return here, we must have encountered an error from runServer() or the server couldn't be created
    // so we will return error
    return 1;
}
