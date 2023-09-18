CC=gcc-12

SOURCES=$(wildcard ./src/*.c)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=./parse
LIB_NAME=Json.h

.PHONY: all
all: $(EXECUTABLE)
	$(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $(EXECUTABLE) $^

$(OBJECTS): %.o: %.c
	$(CC) -o $@ -c -Wall -Wextra -std=c99 -pedantic -Iheaders $(CFLAGS) $<

$(LIB_NAME): $(wildcard headers/*.h) $(wildcard src/*.c)
	echo "#include <stddef.h>" > $(LIB_NAME)
	tail -n +2 headers/hash-table.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +2 headers/json.h >> $(LIB_NAME)

	echo "\n#if defined(JSON_IMPLEMENTATION)\n" >> $(LIB_NAME)
	echo "#include <string.h>" >> $(LIB_NAME)
	echo "#include <stdio.h>" >> $(LIB_NAME)
	echo "#include <stdarg.h>" >> $(LIB_NAME)
	echo "#include <stdlib.h>" >> $(LIB_NAME)

	tail -n +6 src/hash-table.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +2 headers/utils.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +4 src/utils.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +2 headers/logs.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +2 headers/iterator.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +4 src/iterator.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +4 headers/parser.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +5 src/parser.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +12 src/json.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	echo "#endif // JSON_IMPLEMENTATION\n" >> $(LIB_NAME)

.PHONY: build-lib
lib: $(LIB_NAME)
	echo "Done"

.PHONY: test-units
test-units: $(LIB_NAME) tests/test-units.c
	$(CC) -o ./test-units -Wall -Wextra -std=c99 -pedantic ./tests/test-units.c
	./test-units
	rm ./test-units

.PHONY: test-examples
test-examples: $(LIB_NAME)
	$(CC) -o ./test-examples -D COMPILER='"$(CC)"' tests/test-examples.c
	./test-examples
	rm ./test-examples

.PHONY: tests
tests: $(LIB_NAME) test-units test-examples

.PHONY: clean
clean: 
	rm -f $(OBJECTS) $(EXECUTABLE) $(LIB_NAME) $(wildcard tests/examples/*/*.gen.txt tests/examples/*/*.gen)