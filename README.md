# Simple Database in C (Redis Clone)

## Introduction

How does Redis work under the hood? What better way to learn than by re-creating a simple version of it! We'll be implementing an in-memory database using a hastable in C with the ability to save it to a file in order to gain persistance to disk.

Like Redis, we will implement a fast and simple key-value store that lives in memory. Consequently, reading from the database will be extremely fast since we won't be accessing a slow disk like traditional databases. But, like traditional databases, users will be able to insert a new entry in the database store, remove an existing entry, or update an entry.

Since all we are doing is implementing a key-value store, a hashtable is a perfect candidate for the underlying datastructure. For information on how a hashtable works, check out the explanation here https://www.freecodecamp.org/news/hash-tables/

## Hashtable

Lets start out by creating the hashtable first as its going to be our foundation for the whole application. Lets make a `hashtable.c` source file and `hashtable.h` header file. In the header file, lets declare prototypes for the functions and structs we need for the implementation.

Lets start by declaring the types that we're going to store in the hashtable. We'll store strings, unsigned ints, signed ints, and doubles. Also, we'll include a NONE type for housekeeping later on.

```C
typedef enum EntryType {
    STRING,
    UNSIGNED_INT,
    SIGNED_INT,
    DOUBLE,
    NONE,
} EntryType;
```

Next, lets start on laying out the groundwork for the hashtable. 

Lets declare a struct that represents a single value stored in the hash table. We store the values we  just discussed inside a union and store the type of the value alongside it. This way we will know whether there is an integer, string or whatever stored.

```C
typedef struct HashtableValue {
    EntryType entryType;
    union {
        void *val; // store a void* instead of a char* in case we want to expand the types of data we store
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
} HashtableValue;
```
Now we can define a struct that represents an entry in the hashtable. Each entry in this table will correspond to one key-value pair the user wants to store. 

For each entry, we will need to know its key (represented by the `void *key` and `size_t keylen`) as well as the value associated with the key (represented by the `HashtableValue htv` struct we just created).

For this project, we will use Chaining as our hashtable conflict resolution strategy. So, if two keys hash to the same value we will construct a linked list. Thats why we have a pointer to a `struct HashtableEntry` in the same struct.

```C
typedef struct HashtableEntry {
    void *key;
    size_t keylen;
    HashtableValue htv;
    struct HashtableEntry *next;
} HashtableEntry;
```
Finally, we can define the struct that puts all of this together to represent a Hashtable.

At the start, we won't know how many key-value pairs the user will want to store, so we need to implement a resizeable array to hold all the entries so we can adjust it later on. So, we use an array of pointers to hashtable entries and store the size of the array alongside it. 

You might notice we don't store the size as an integer number. This is because we want our resizeable array to be a power of 2 (this makes it easy to resize by doubling/halfing the size). Since we know that our hashtable array size will be a power of 2 (ex 2^5, 2^6 etc) we can get away with only storing the exponent in `unsigned char exp` (then we can get the size of the array by doing `1 << exp`).

```C
typedef struct Hashtable {
    HashtableEntry **table; /* Array of pointers to hashtable entries */
    uint64_t len;           /* Length of the table array (number of key/value pairs)*/
    unsigned char exp;      /* Size of the table array is 1<<exp (size is number of open slots) */
} Hashtable;
```

Now, with the data structure of the hashtable out of the way, we can focus on the algorithm side of the hashtable.

Lets start off by creating the hash function. Or in this case, using an existing hash function. We use the `siphash` hash function from the [github repo here](https://github.com/veorq/SipHash). This function is pretty simple, just take in the key with its length and return the `uint` hash. Not much else to say.

```C
uint64_t htHashFunction(const void *key, size_t keylen) {
    // Using siphash for the hashing function
    uint64_t hash;
    siphash(key, keylen, k, (uint8_t *)&hash, sizeof(hash));
    return hash;
}
```



Lets move on to implementing the function to find an entry from the hashtable. In the `htFindEntry` helper function, we perform a lookup in the hashtable by hashing the key using the previous function (making sure we modulo with the size of the array so we don't go out of bounds). Then we simply index into the hashtable. We might find the entry on that first try, but there is always the case where two entries might have hashed to the same index (a conflict). We chose our conflict resolution strategy to create a linked-list when this happens, therefore we need to look through the linked list until we find our entry we are looking for. For this, we use the `cmpKey` helper function to compare the two keys.

In the function `htFind`, if we find the entry we are looking for, we will obviously return that value. If we don't find the value, we return a NONE type to indicate so.
```C
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
```
Now lets look at adding a new key-value pair to the hashtable int he 'htAdd' function. Since we use a resizeable array, we might need to increase the size if we're going to be adding more items than we can hold. So, if we need to make the hashtable larger, we call the helper function 'htExpandAndReshash'.

In 'htExpandandReshash', we allocate a larger new table, rehash everything in the old table and insert the old entries in the new table. We still might run into the problem where two items rehash into the same index in the new table, so we need to implement our conflict resolution strategy. You can see in the code that we simply add the entry that has the conflict at the head of the list. Note that we don't need to check if something is already in the spot we're trying to insert into. As an exercise, think of what happens when there already is an item in the spot versus when there isn't (NULL). You'll notice that it works out to be correct in the end.

After we make a larger hashtable (if we needed to), we add the new entry. We hash the key just like in 'htFind', allocate memory and initialize the entry, and then add the entry at the top of the linked list.

```C
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
```

Now that we can add items in the hashtable, we should also be able to remove items. We start out the same as before and we ahsh the key and go to the index into the hashtable. After that, it is the same process as removing a node from a linked list. 

```C
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
```

The final operation is the update/replace function. This is a pretty simple function that finds the entry in the table and updates the value stored there. Not much to say here

```C
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
```

For completeness, here are the functions to create and delete the hashtables. We simply allocate the memory and initialize the properties in `htCreateTable` and remove all the entries in the hashtable in `htDeleteTable`

`HASHTABLE_DEFAULTCAP` is defined as 5.

```C
Hashtable *htCreateTable() {
    Hashtable *ht = malloc(sizeof(Hashtable));
    memset(ht, 0, sizeof(Hashtable));
    if (ht) {
        ht->len = 0;
        ht->exp = HASHTABLE_DEFAULTCAP;
        ht->table = malloc((1 << HASHTABLE_DEFAULTCAP) * sizeof(HashtableEntry));
        memset(ht->table, 0, (1 << HASHTABLE_DEFAULTCAP) * sizeof(HashtableEntry));
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
```

## Database CLI

Now that we've implemented the hashtable for the foundation, we can now implement the command-line interface (CLI) that the user can interact with our program. 

First, lets create the main.c file for hte entrypoint into our program. Then, we can add the structs that we will use to organize and group the data we handle.

```C
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
```

You can see that we have structs for `InputStatement`, `Command`, and `Database`. A user's input to the program will be parsed into an `InputStatement` where the `buffer` is their input and the `buffer_len` is the total length of the buffer while the `input_len` is the length of the user's input. From the `InputStatement`, we parse and construct the `Command` that holds all the information needed to complete a database command with the `query`, `type` of the data, `key`, and `value` to store. Finally, the `Database` struct represents the Database.

Now, lets implement the function to execute an insert command in the database based on the user's input

```C
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
```

This function simply constructs the necessary data types required for the function call to `htAdd` implemented earlier along with a helper function to make sense of the user's `type` input.

The functions to execute the select and delete commands are very simple. Not much explanation needed here. For select, we simply execute the query in the database and print out the result. For delete, we simply delete the entry in the database.

```C
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
```

Finally, to finish off the user-input -> database queries, we have the replace function here.

```C
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
```

Like the ones before, this one is also pretty simple where we construct the data types required for the function call to `htReplace` and let that function do all the heavy lifting.

To put all these function together, here is the function to parse the `InputStatement` and create the `Command` argument to each of the previous functions.

```C
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
```

We use the `strtok` function to parse the input statement and separate it into the `query`, `key`, `type`, and `value`. Not all queries will have all the fields of a `Command`. For example, a `select` input will only specify the query type and key since a value doesn't make sense. Therefore, checking if the previous token was `NULL` before calling `strtok` again handles these cases.

So, a typical input will look something like this:

`insert key1 int 123`

Where `insert` specifies an insert query, `key1` is the key, `int` is the data type, and `123` is the value to store. We can also specify `select`, `delete`, and `replace` queries by following the format and filling in the appropriate values:

`{QUERY TYPE} {KEY} {DATA TYPE} {VALUE}`

After all this, we can move on to the last and final step in this project. We can implement the `main` function and put this all together.

```C
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
        printf("db > ");
        if (readInput(&st) < 0) {
            printf("End of file reached, exiting...\n");
            free(st.buffer);
            htDeleteTable(db.ht);
            break;
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
```

In the `main` function, we infinitely loop and wait for a user's input after creating the database (the hashtable). We print out a nice message `db > ` to prompt the user to input something and then we read their input. We first check if we reached the end of file (this can happen if someone piped cat into the input of the program like `cat text.txt | databaseprogram`) and then we execute the user's input.

And thats it! we now have a simple Redis clone of an in-memory database written in C.
