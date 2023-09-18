#include <stddef.h>


#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H


typedef enum Json_internal_TableValueType {
    Json_internal_TableValueTypeString,
    Json_internal_TableValueTypeNumber,
    Json_internal_TableValueTypeObject,
    Json_internal_TableValueTypeBoolean,
    Json_internal_TableValueTypeArray,
    Json_internal_TableValueTypeNull,
} Json_internal_TableValueType;

typedef struct JsonStringRange {
    size_t start;
    size_t length;
} JsonStringRange;

typedef union Json_internal_TableValueUnion {
    JsonStringRange string;
    float number;
    int boolean;

    struct {
        size_t contextIndex;
        size_t size;
    } object;
    
    struct {
        size_t contextIndex;
        size_t size;
    } array;
} Json_internal_TableValueUnion;

typedef struct JsonValue {
    Json_internal_TableValueType type;
    Json_internal_TableValueUnion value;
} JsonValue;

typedef struct Json_internal_TableItem {
    size_t contextIndex;

    JsonStringRange name;

    size_t index;

    JsonValue typedValue;
} Json_internal_TableItem;

typedef struct Json_internal_Table {
    int* busy;
    int* byIndex;
    size_t maxSize;
    size_t size;

    Json_internal_TableItem* buffer;

    char* stringBuffer;
} Json_internal_Table;

typedef struct Json_internal_Destination {
    int isRoot;
    JsonStringRange name;
    size_t ctx;

    int isIndex;
    size_t index;
} Json_internal_Destination;

#endif // HASH_TABLE_H
