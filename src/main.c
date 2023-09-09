#include <stdio.h>
#include <stdlib.h>

#include "json.h"

int main(void) {
    Json_createStatic(json, "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]", 20); // Creating JSON object with macro

    if(Json_parse(json) < 0) { // Parsing values
        printf("Failed to parse JSON");
        return 1;
    }

    JsonValue* root = Json_getRoot(json); // Get root element of the JSON

    size_t length;
    Json_asArray(root, &length);

    printf("Array length: %zu\n", length);
    for(size_t i = 0; i < length; i++) {
        JsonValue* el = Json_getIndex(json, root, i);

        float value;
        Json_asNumber(el, &value);
        printf("[%.2zu]: %.2f\n", i, value);
    }

    return 0;
}