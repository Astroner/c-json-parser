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

int main1(void) {
    TableItem buffer[4];

    memset(buffer, 0, sizeof(buffer));

    buffer[1].isBusy = 1;
    buffer[1].byIndex = 0;
    buffer[1].typedValue.value.number = 12.f;

    buffer[2].isBusy = 1;
    buffer[2].byIndex = 0;
    buffer[2].typedValue.value.number = 12.f;

    buffer[3].isBusy = 1;
    buffer[3].byIndex = 0;
    buffer[3].typedValue.value.number = 11.f;

    JsonTable table = {
        .buffer = buffer,
        .stringBuffer = "8",
        .maxSize = sizeof(buffer) / sizeof(buffer[0]),
        .size = 0,
    };

    JsonDestination dest = {
        .isIndex = 1,
        .index = 0,
        .ctx = 0
    };

    TableItem* item = JsonTable_set(&table, &dest);
    if(item) {
        item->typedValue.value.number = 1;

    }

    JsonTable_print(&table);

    TableItem* stored = JsonTable_getByIndex(&table, 0, 0);


    if(!stored) {
        printf("Kek\n");
        return 1;
    }

    printf("%f\n", stored->typedValue.value.number);

    return 0;
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