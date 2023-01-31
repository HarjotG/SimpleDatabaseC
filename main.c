#include "hashtable.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    char *buffer;
    ssize_t buffer_len;
    ssize_t input_len;
} InputStatement;

typedef struct {
    char *query;
    char *type;
    char *key;
    char *value;
} Command;

typedef struct {
    Hashtable *ht; /* The hashtable implementation of the database*/
} Database;

void printPrompt() {
    printf("db > ");
}

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

int executeInsertCommand(Hashtable *ht, Command *command) {
    HashtableValue htv;
    char *err = NULL;
    int retVal;
    switch (getKeyType(command->type)) {
    case STRING:
        htv.entryType = STRING;
        htv.v.val = malloc(strlen(command->value));
        strcpy(htv.v.val, command->value);
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            printf("Key %s already exists\n", command->key);
        }
        return retVal;
    case UNSIGNED_INT:
        htv.entryType = UNSIGNED_INT;
        htv.v.u64 = (uint64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            printf("Key %s already exists\n", command->key);
        }
        return retVal;
    case SIGNED_INT:
        htv.entryType = SIGNED_INT;
        htv.v.s64 = (int64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            printf("Key %s already exists\n", command->key);
        }
        return retVal;
    case DOUBLE:
        htv.entryType = DOUBLE;
        htv.v.d = strtod(command->value, &err);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        retVal = htAdd(ht, command->key, strlen(command->key), htv);
        if (retVal == 1) {
            printf("Key %s already exists\n", command->key);
        }
        return retVal;
    default:
        return 1; // error
    }
}

int executeSelectCommand(Hashtable *ht, Command *command) {
    HashtableValue htv = htFind(ht, command->key, strlen(command->key));
    if (htv.entryType == NONE) {
        printf("Key not found\n");
        return 0;
    }
    switch (htv.entryType) {
    case STRING:
        printf("{%s: %s}\n", command->key, (char *)htv.v.val);
        return 0;

    case UNSIGNED_INT:
        printf("{%s: %ld}\n", command->key, htv.v.u64);
        return 0;
    case SIGNED_INT:
        printf("{%s: %ld}\n", command->key, htv.v.s64);
        return 0;
    case DOUBLE:
        printf("{%s: %lf}\n", command->key, htv.v.d);
        return 0;
    default:
        return 1;
    }
}

int executeDeleteCommand(Hashtable *ht, Command *command) {
    int retval = htRemove(ht, command->key, strlen(command->key));
    return retval;
}

int executeReplaceCommand(Hashtable *ht, Command *command) {
    HashtableValue htv;
    char *err = NULL;
    switch (getKeyType(command->type)) {
    case STRING:
        htv.entryType = STRING;
        htv.v.val = malloc(strlen(command->value));
        strcpy(htv.v.val, command->value);
        return htReplace(ht, command->key, strlen(command->key), htv);
    case UNSIGNED_INT:
        htv.entryType = UNSIGNED_INT;
        htv.v.u64 = (uint64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        return htReplace(ht, command->key, strlen(command->key), htv);
    case SIGNED_INT:
        htv.entryType = SIGNED_INT;
        htv.v.s64 = (int64_t)strtol(command->value, &err, 10);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        return htReplace(ht, command->key, strlen(command->key), htv);
    case DOUBLE:
        htv.entryType = DOUBLE;
        htv.v.d = strtod(command->value, &err);
        if (strcmp(err, "") != 0) {
            return 1; // error
        }
        return htReplace(ht, command->key, strlen(command->key), htv);
    default:
        return 1;
    }
}

void closeDb(Database *db) {
    htDeleteTable(db->ht);
}

int executeDbCommand(InputStatement *st, Database *db) {
    Command command;
    command.query = strtok(st->buffer, " ");
    if (command.query != NULL) {
        command.key = strtok(NULL, " ");
    } else {
        command.key = NULL;
    }
    if (command.key != NULL) {
        command.type = strtok(NULL, " ");
    } else {
        command.type = NULL;
    }
    if (command.type != NULL) {
        // get end of input as value
        command.value = command.type + strlen(command.type) + 1;
    } else {
        command.value = NULL;
    }
    if (strncmp(command.query, "insert", 6) == 0) {
        int retval = executeInsertCommand(db->ht, &command);
        return retval;
    } else if (strncmp(command.query, "select", 6) == 0) {
        int retval = executeSelectCommand(db->ht, &command);
        return retval;
    } else if (strncmp(command.query, "delete", 6) == 0) {
        int retval = executeDeleteCommand(db->ht, &command);
        return retval;
    } else if (strncmp(command.query, "replace", 6) == 0) {
        int retval = executeReplaceCommand(db->ht, &command);
        return retval;
    }
    return 1;
}

int doMetaCommand(InputStatement *st, Database *db) {
    char *metaCommand = strtok(st->buffer, " ");

    if (strcmp(".q", metaCommand) == 0) {
        free(st->buffer);
        closeDb(db);
        printf("Exiting...\n");
        exit(0);
    } else if (strcmp(".save", metaCommand) == 0) {
        char *fileName = strtok(NULL, " ");
        FILE *file = fopen(fileName, "w");
        if (htSerializeToFile(db->ht, file) == 0) {
            fclose(file);
            return 0;
        }
    } else if (strcmp(".load", metaCommand) == 0) {
        char *fileName = strtok(NULL, " ");
        FILE *file = fopen(fileName, "r");
        if (htDeserializeFromFile(db->ht, file) == 0) {
            fclose(file);
            return 0;
        }
    }
}

int readInput(InputStatement *st) {
    ssize_t input_len = getline(&st->buffer, &st->buffer_len, stdin);
    if (input_len < 0) {
        return -1;
    }
    // rid of newline if exists
    if (input_len > 0 && st->buffer[input_len - 1] == '\n') {
        st->buffer[input_len - 1] = '\0';
    }
    st->input_len = input_len - 1;
    return 0;
}

int main(int argc, char *argv[]) {
    InputStatement st;
    st.buffer = NULL;
    st.buffer_len = 0;
    st.input_len = 0;
    Database db;
    db.ht = htCreateTable();
    while (1) {
        printPrompt();
        if (readInput(&st) < 0) {
            printf("End of file reached, exiting...\n");
            free(st.buffer);
            closeDb(&db);
            break;
        } else if (st.buffer[0] == '.') {
            doMetaCommand(&st, &db);
        } else {
            if (executeDbCommand(&st, &db) != 0) {
                printf("Error completing command\n");
            } else {
                printf("Command completed successfully\n");
            }
        }
    }
    return 0;
}
