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
        size_t namespace;
        size_t size;
    } object;
    
    struct {
        size_t namespace;
        size_t size;
    } array;
} Json_internal_TableValueUnion;

typedef struct JsonValue {
    Json_internal_TableValueType type;
    Json_internal_TableValueUnion value;
} JsonValue;

typedef struct Json_internal_TableItem {
    size_t namespace;

    size_t arrayIndex;
    JsonStringRange name;

    unsigned long hash;

    JsonValue typedValue;
} Json_internal_TableItem;

typedef struct Json_internal_Table {
    int* busy;
    int* byIndex;
    size_t maxSize;
    size_t size;

    Json_internal_TableItem* buffer;
} Json_internal_Table;

#endif // HASH_TABLE_H
