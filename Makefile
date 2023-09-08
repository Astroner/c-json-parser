CC=gcc-12

SOURCES=$(wildcard ./src/*.c)
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=./parse

.PHONY: all
all: $(EXECUTABLE)
	$(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $(EXECUTABLE) $^

$(OBJECTS): %.o: %.c
	$(CC) -o $@ -c -Wall -Wextra -std=c99 -pedantic -Iheaders $(CFLAGS) $<

.PHONY: clean
clean: 
	rm -f $(OBJECTS) $(EXECUTABLE)