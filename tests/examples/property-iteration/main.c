#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    Json_createStatic(
        json,
        "{"
        "\"first\": 1,"
        "\"second\": 2,"
        "\"third\": 3"
        "}",
        20
    );

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonObjectIterator properties;
    JsonObjectIterator_init(json, root, &properties);

    JsonProperty property;
    while(JsonObjectIterator_next(&properties, &property)) {
        JsonStringRange_print(json, property.name);
        
        float value;
        Json_asNumber(property.value, &value);

        printf(": %.1f\n", value);
    }

    return 0;
}