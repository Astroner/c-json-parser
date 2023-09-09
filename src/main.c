#include <stdio.h>
#include <stdlib.h>

#include "json.h"

char* readFile(char* path) {
    FILE* f = fopen(path, "r");

    fseek(f, 0, SEEK_END);

    fpos_t size;

    fgetpos(f, &size);

    char* result = malloc(size + 1);

    fseek(f, 0, SEEK_SET);

    fread(result, 1, size, f);

    result[size] = '\0';

    fclose(f);

    return result;
}

int main(void) {
    char* input = readFile("test.json");

    Json_createStatic(data, input, 150);

    if(Json_parse(data) < 0) return 1;

    JsonValue* root = Json_getRoot(data);
    
    JsonValue* title = Json_chainVA(
        data, 
        root,
        "onResponseReceivedActions",
        "!+0",
        "appendContinuationItemsAction",
        "continuationItems",
        "!+0",
        "richSectionRenderer",
        "content",
        "backgroundPromoRenderer",
        "title",
        "simpleText",
        NULL
    );

    Json_print(data, title);

    return 0;
}