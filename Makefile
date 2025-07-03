CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests
LIB_NAME = lib/libExif-pt.a


SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(TEST_SRCS:$(TEST_DIR)/%.c=$(BUILD_DIR)/tests/%)

.PHONY: all clean


all: $(LIB_NAME) $(TEST_BINS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_NAME): $(OBJS)
	@mkdir -p lib
	ar rcs $@ $^

$(BUILD_DIR)/tests/%: $(TEST_DIR)/%.c $(LIB_NAME)
	@mkdir -p $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $< -Llib -lExif-pt -o $@


test: all
	./build/tests/test_exif_parser

clean:
	rm -rf $(BUILD_DIR) lib
