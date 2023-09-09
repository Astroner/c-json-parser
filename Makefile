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
	cat headers/hash-table.h > $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +3 headers/json.h >> $(LIB_NAME)

	echo "#if defined(JSON_IMPLEMENTATION)\n" >> $(LIB_NAME)

	tail -n +3 src/hash-table.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	cat headers/utils.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +2 src/utils.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	cat headers/logs.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	cat headers/iterator.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +4 src/iterator.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +4 headers/parser.h >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)
	tail -n +5 src/parser.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	tail -n +7 src/json.c >> $(LIB_NAME)
	echo "\n" >> $(LIB_NAME)

	echo "#endif // JSON_IMPLEMENTATION\n" >> $(LIB_NAME)

.PHONY: build-lib
lib: $(LIB_NAME)
	echo "Done"

.PHONY: test
test: $(LIB_NAME) test.c
	gcc -o ./test -Wall -Wextra -std=c99 -pedantic test.c
	./test
	rm ./test

.PHONY: clean
clean: 
	rm -f $(OBJECTS) $(EXECUTABLE) $(LIB_NAME)