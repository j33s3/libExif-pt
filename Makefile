CC = gcc
CFLAGS = -Wall -Iinclude

LIB = libExif-pt.a
SRC = src/exif_parser.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean tst

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $^

test: all
	$(CC) tests/test_exif_parser.c -Iinclude -L. ./libExif-pt.a -o test_runner
	./test_runner

clean:
	rm -f src/*.o *.a test_runner
