#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "../../../Json.h"

int main(void) {
    Json_createStatic(json, "\"string\"", 20);

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonStringRange* str = Json_asString(root);

    JsonStringIterator iterator;
    JsonStringIterator_init(json, str, &iterator);

    char ch;
    while((ch = JsonStringIterator_next(&iterator))) {
        putchar(ch);
    }
    putchar('\n');

    return 0;
}
