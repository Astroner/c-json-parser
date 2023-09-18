#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "../../../Json.h"

int main(void) {
    Json_create(json, 20); // Creating Json object with macro

    Json_setSource(json, "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]"); // setting string source

    if(Json_parse(json) < 0) { // Parsing values
        printf("Failed to parse JSON");
        return 1;
    }

    JsonValue* root = Json_getRoot(json); // Get root element of the JSON

    size_t length;
    Json_asArray(root, &length); // Get array length

    printf("Array length: %zu\n", length);
    for(size_t i = 0; i < length; i++) {
        JsonValue* el = Json_getIndex(json, root, i); // Get array element with index i

        float value;
        Json_asNumber(el, &value); // Get el value as float
        printf("[%.2zu]: %.2f\n", i, value);
    }
    /*
    stdout:
    Array length: 11
    [00]: 1.00
    [01]: 2.00
    [02]: 3.00
    [03]: 4.00
    [04]: 5.00
    [05]: 6.00
    [06]: 7.00
    [07]: 8.00
    [08]: 9.00
    [09]: 10.00
    [10]: 11.00
    */
    return 0;
}