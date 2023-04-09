BUILD_DIR := ./build
OBJ_DIR := ${BUILD_DIR}/obj
SRC_DIR := ./src
TEST_DIR := ./tests
CC := gcc
DATABASE_EXEC := $(BUILD_DIR)/db
TEST_EXEC := $(BUILD_DIR)/test

SRCS := src/main.c src/hashtable.c src/siphash.c src/network.c
SRCS_TEST := tests/test.c src/hashtable.c src/siphash.c src/network.c

OBJS := $(SRCS:%.c=$(OBJ_DIR)/%.o)
OBJS_TEST := $(SRCS_TEST:%.c=$(OBJ_DIR)/%.o)

DEPS := $(OBJS:.o=.d)
DEPS_TEST := $(OBJS_TEST:.o=.d)

CFLAGS := -g -MMD -MP

.PHONY: all
all: ${DATABASE_EXEC} ${TEST_EXEC}

$(DATABASE_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(TEST_EXEC): $(OBJS_TEST)
	$(CC) $(OBJS_TEST) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	-rm -r -f $(BUILD_DIR)/*

-include $(DEPS)