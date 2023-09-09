# Hi there
This is simple C STB-like library for json parsing, that uses static memory.

# Table of Content

# Usage 
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    Json_createStatic(json, "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]", 20); // Creating Json object with macro

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
```
As in any STB-like library you need to once include implementation by defining JSON_IMPLEMENTATION before including the library.
**Json_createStatic** macro takes 3 arguments:
 - **name** - the name of the JSON variable
 - **src** - source string
 - **buffer size** - size of memory buffer to store values. Size is not provided in bytes but in number of referenceable elements. For example:
    ```json
    {
        "a": 33,
        "b": "str",
        "c": [22]
    }
    ```
    In this JSON we only have 5 referenceable elements: root, "a", "b", "c" and 22 as array element.

**Json_parse** parses **Json** object. If there is syntax error or insufficient memory it returns -1 otherwise 0.

**Json_asArray** checks that provided **JsonValue** is an array and if so uses **length** pointer to return array length.

**Json_getIndex** checks that provided **JsonValue** of **Json** is an array and if so returns **JsonValue** from provided index otherwise returns **NULL**

**Json_asNumber** checks that provided **JsonValue** is a number and if so uses **value** pointer to return float value.

Basic algorithm looks like:
 - Create Json object
 - Parse it
 - Use function **Json_getRoot()** to get the root
 - Use functions like **Json_getKey()** or **Json_getIndex()** to traverse through the Json structure
 - Use functions like **Json_asNumber()**, **Json_asString()**, **Json_asArray()** and e.t.c. to get values