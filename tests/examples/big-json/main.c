#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

    Json_create(json, 200);

    Json_setSource(json, src);

    assert(Json_parse(json) >= 0);

    Json_print(json, Json_getRoot(json));

    return 0;
}
