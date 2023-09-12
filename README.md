# Hi there
This is simple C STB-like library for json parsing, that uses static memory.

# Table of Content
 - [Usage](#usage)
 - [API Reference](#api-reference)
     - [Working with types](#working-with-types)
         - [Null](#null)
         - [Number](#number)
         - [Boolean](#boolean)
         - [String](#string)
             - [JsonStringRange](#jsonstringrange)
                 - [Copy](#copy)
                 - [Print](#print)
                 - [Iterate](#iterate)
         - [Array](#array)
             - [Accessing element](#accessing-elements)
         - [Object](#object)
             - [Accessing properties](#accessing-properties)
             - [Iterating over properties](#iterating-over-properties)
     - [Chaining](#chaining)
        - [Json_chain](#json_chain)
        - [Json_chainM](#json_chainm)
        - [Json_chainVA](#json_chainva)
     - [Printing](#printing)
         - [Json_print](#json_print)
         - [Json_printType](#json_printtype)

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

Let's take a closer look at used functions:
 - **Json_createStatic** macro takes 3 arguments:
    - **name** - the name of the Json variable
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
 - **Json_parse** - parses **Json** object. If there is syntax error or insufficient memory it returns -1 otherwise 0.
 - **Json_asArray** checks that provided **JsonValue** is an array and if so uses **length** pointer to return array length.
 - **Json_getIndex** checks that provided **JsonValue** of **Json** is an array and if so returns **JsonValue** from provided index otherwise returns **NULL**
 - **Json_asNumber** checks that provided **JsonValue** is a number and if so uses **value** pointer to return float value.

Overall, basic usage algorithm looks like:
 - Create Json object
 - Parse it with **Json_parse()**
 - Use **Json_getRoot()** to get the root
 - Use **Json_getKey()** or **Json_getIndex()** to traverse through the Json structure
 - Use **Json_asNumber()**, **Json_asString()**, **Json_asArray()** and e.t.c. to get values

# API Reference
## Working with types
Here is the list of functions to work with data types.

### Null
Use function **Json_isNull** to interpret **JsonValue** as null.
```c
int Json_isNull(JsonValue* item);
```
 - **returns** - **1** if provided **JsonValue** is null else it returns **0**
 - **item** - **JsonValue** to parse

### Number
Use function **Json_asNumber** to interpret **JsonValue** as number.
```c
int Json_asNumber(JsonValue* item, float* value);
```
 - **returns** - **1** if provided **JsonValue** is a number else it returns **0**
 - **item** - **JsonValue** to parse
 - **value** - **NULLABLE** - pointer to memory location where number value will be placed

### Boolean
Use function **Json_asBoolean** to interpret **JsonValue** as boolean.
```c
int Json_asBoolean(JsonValue* item, int* value);
```
 - **returns** - **1** if provided **JsonValue** is a boolean else it returns **0**
 - **item** - **JsonValue** to parse
 - **value** - **NULLABLE** - pointer to memory location where boolean value will be placed

### String
Use function **Json_asString** to interpret **JsonValue** as string.
```c
JsonStringRange* Json_asString(JsonValue* item);
```
 - **returns** - **NULLABLE** - pointer to **JsonStringRange** if **JsonValue** is a string and **NULL** if it is not. 
 - **item** - **JsonValue** to parse

In general, string are represented in format of **JsonStringRange**:
```c
struct JsonStringRange {
    size_t start;
    size_t length; // not including null-terminator
};
```
It represents piece of json source string.

#### JsonStringRange
This library provides several methods to interact with string ranges:

##### Copy
**JsonStringRange_copy()** copies piece of source string into provided buffer
```c
void JsonStringRange_copy(Json* json, JsonStringRange* range, char* buffer, size_t bufferSize);
```
 - **json** - **Json** object
 - **range** - string range to copy
 - **buffer** - pointer to memory locations where the string will be copied and **null-terminated**
 - **bufferSize** - buffer size

If **bufferSize** is insufficient the function will copy only piece of string range.

##### Print
**JsonStringRange_print()** prints string range into stdout.
```c
void JsonStringRange_print(Json* json, JsonStringRange* range);
```
 - **json** - **Json** object
 - **range** - string range to copy

##### Iterate
**JsonStringIterator** structure allows string iteration
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    Json_createStatic(json, "\"string\"");

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonStringRange* str = Json_asString(root);

    JsonStringIterator iterator;
    JsonStringIterator_init(json, str, &iterator);

    char ch;
    while((ch = JsonStringIterator_next(&iterator))) {
        putchar(ch);
    }
    putchar('\n');

    return 0;
}
```
**JsonStringIterator_init()** initiates iterator with provided **Json** and **JsonValue** and **JsonStringIterator_next()** just iterates each char of the string.

### Array
Use function **Json_asArray** to interpret **JsonValue** as array.
```c
int Json_asArray(JsonValue* item, size_t* length);
```
 - **returns** - **1** if provided **JsonValue** is an array else it returns **0**
 - **item** - **JsonValue** to parse
 - **length** - **NULLABLE** - pointer to memory location where array length will be placed

#### Accessing elements
Use function **Json_getIndex** to access specific array element
```c
JsonValue* Json_getIndex(Json* json, JsonValue* item, size_t index);
```
 - **returns** - **NULLABLE** - **JsonValue** if provided **JsonValue** is an array and it has element with provided **index** and **NULL** if it is not.
 - **json** - **Json** structure
 - **item** - **JsonValue** to traverse
 - **index** - element index

### Object
Use function **Json_asObject** to interpret **JsonValue** as object.
```c
int Json_asObject(JsonValue* item, size_t* size);
```
 - **returns** - **1** if provided **JsonValue** is an object else it returns **0**
 - **item** - **JsonValue** to parse
 - **length** - **NULLABLE** - pointer to memory location where number of object fields will be placed

#### Accessing properties
Use function **Json_getKey** to access specific object property
```c
JsonValue* Json_getKey(Json* json, JsonValue* item, char* key);
```
 - **returns** - **NULLABLE** - **JsonValue** if provided **JsonValue** is an object and it has property with provided **key** and **NULL** if it is not.
 - **json** - **Json** structure
 - **item** - **JsonValue** to traverse
 - **key** - property key

#### Iterating over properties
Use **JsonObjectIterator** to iterate over object properties:
```c
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

    /* 
    stdout:
        third: 3.0
        second: 2.0
        first: 1.0
    */

    return 0;
}
```
> As you can see, iterated properties are not in the same order as in the source due to hashing limitations.

Use **JsonObjectIterator_init()** to initiate iterator with provided **Json** and **JsonValue** and then simply use **JsonObjectIterator_next()** to iterate over the object.
**JsonObjectIterator_next()** uses **JsonProperty** structure with following definition:
```c
struct JsonProperty {
    JsonStringRange* name;
    JsonValue* value;
};
```
It is just a pair of **key** and **value** on which you can use related to these structures methods: **JsonStringRange_copy()**, **Json_asNumber()** and e.t.c.

## Chaining
This section describes methods for key chaining to simplify traversal through nested structures.
Let's say you have this nested structure:
```json
{
    "entities": [
        {
            "name": {
                "text": "Train"
            }
        }
    ]
}
```
And you just want to get this **"Train"** string. To do so with basic traversing functions you would need to write something like this:
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    char* src = readFile("example.json");
    Json_createStatic(json, src, 5);

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
```
So here you can see that nested json will require nested function calls to reach specific property, which is not really convenient.
To simplify this process the library provides several functions, which are described below.

### Json_chain
This function is a straight way to chain several selectors:
```c
JsonValue* Json_chain(Json* json, JsonValue* item, JsonSelector* selectors, size_t selectorsSize);
```
 - **returns** - **NULLABLE** - **JsonValue** if element matching **selectors** exists and **NULL** if it is not
 - **json** - parsed **Json** structure
 - **item** - item to traverse 
 - **selectors** - array of selectors
 - **selectorsSize** - number of selectors

Selector is represented in format of **JsonSelector**:
```c
typedef enum JsonSelectorType {
    JsonKey,
    JsonIndex
} JsonSelectorType;

typedef struct JsonSelector {
    JsonSelectorType type;
    union {
        char* key;
        size_t index;
    } q;
} JsonSelector;
```

Basic usage example:
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

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

    Json_print(json, string); // stdout: "Train"

    free(src);

    return 0;
}
```
So here we are just specifying list of **selectors** and then passing it to **Json_chain()** to get exactly the same result as in the initial code example.

### Json_chainM
It is just a simplification macro for [Json_chain()](#json_chain)
```c
#define Json_chainM(resultName, json, root, ...)
```
 - **resultName** - variable name which will be defined to store the result
 - **json** - **Json** object
 - **root** - starting **JsonValue**
 - **VA_ARGS** - list of selectors

Example:
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    char* src = readFile("example.json");
    Json_createStatic(json, src, 5);

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    Json_chainM(string, json, root, 
        { .type = JsonKey, .q.key = "entities" },
        { .type = JsonIndex, .q.index = 0 },
        { .type = JsonKey, .q.key = "name" },
        { .type = JsonKey, .q.key = "text" }
    );

    Json_print(json, string); // stdout: "Train"

    free(src);

    return 0;
}
```

### Json_chainVA
This function uses variadic arguments for selectors
```c
JsonValue* Json_chainVA(Json* json, JsonValue* item, ...);
```
 - **returns** - **NULLABLE** - **JsonValue** if element matching **selectors** exists and **NULL** if it is not
 - **json** - parsed **Json** structure
 - **item** - item to traverse 
 - **VA_ARGS** - list of string selectors terminated with **NULL**
   This selectors has the following format:
     - If the selector begins with "!+" then if will be interpreted as index
     - Everything else will be interpreted as key

Example:
```c
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "Json.h"

int main(void) {
    char* src = readFile("example.json");
    Json_createStatic(json, src, 5);

    Json_parse(json);

    JsonValue* root = Json_getRoot(json);

    JsonValue* string = Json_chainVA(
        json, 
        root,
        "entities",
        "!+0",
        "name",
        "text",
        NULL
    );

    Json_print(json, string);

    free(src);

    return 0;
}
```
> Of Course this function is slower than Json_chain() due to selectors parsing

## Printing
This section describes set of function to print **Json** info into console:

### Json_print
This function prints given **JsonValue** into stdout
```c
void Json_print(Json* json, JsonValue* root);
```

### Json_printType
This function prints type of the given **JsonValue** into stdout
```c
void Json_printType(JsonValue* root);
```