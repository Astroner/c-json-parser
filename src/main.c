#include <stdio.h>
#include <stdlib.h>

#include "json.h"

int main(void) {
    Json_create(json, 20); // Creating JSON object with macro

    Json_setSource(json, "{ \"a\": 22, \"n\": 1 }");

    if(Json_parse(json) < 0) { // Parsing values
        printf("Failed to parse JSON");
        return 1;
    }

    JsonValue* root = Json_getRoot(json); // Get root element of the JSON

    JsonObjectIterator iterator;
    JsonObjectIterator_init(json, root, &iterator);

    JsonProperty field;
    while(JsonObjectIterator_next(&iterator, &field)) {
        JsonStringIterator strIter;
        JsonStringIterator_init(json, field.name, &strIter);
        
        char ch;
        while((ch = JsonStringIterator_next(&strIter))) {
            putchar(ch);
        }
        
        float value;
        Json_asNumber(field.value, &value);

        printf(": %.2f\n", value);
    }

    return 0;
}