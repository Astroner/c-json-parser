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
    
    Json_create(json, 5);

    Json_setSource(json, src);

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonValue* string = Json_getKey(
        json,
        Json_getKey(
            json,
            Json_getIndex(
                json,
                Json_getKey(
                    json,
                    root,
                    "entities"
                ),
                0
            ),
            "name"
        ),
        "text"
    );

    Json_print(json, string); // stdout: "Train"

    free(src);

    return 0;
}
