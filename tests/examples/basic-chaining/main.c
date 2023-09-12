#include <stdio.h>
#include <stdlib.h>

#define JSON_IMPLEMENTATION
#include "../../../Json.h"

char* readFile(char* path) {
    FILE* f = fopen(path, "r");

    if(!f) return NULL;

    fseek(f, 0, SEEK_END);

    fpos_t size;

    fgetpos(f, &size);

    fseek(f, 0, SEEK_SET);

    char* result = (char*)malloc(size + 1);

    fread(result, 1, size, f);

    result[size] = '\0';

    fclose(f);

    return result;
}

int main(void) {
    char* src = readFile("example.json");
    Json_createStatic(json, src, 5);

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonSelector selectors[] = {
        { .type = JsonKey, .q.key = "entities" },
        { .type = JsonIndex, .q.index = 0 },
        { .type = JsonKey, .q.key = "name" },
        { .type = JsonKey, .q.key = "text" },
    };

    JsonValue* string = Json_chain(json, root, selectors, sizeof(selectors) / sizeof(selectors[0]));

    Json_print(json, string);

    free(src);

    return 0;
}
