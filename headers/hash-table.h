#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H

#include <stdlib.h>

typedef enum JsonValueType {
    JsonValueTypeString,
    JsonValueTypeNumber,
    JsonValueTypeObject,
    JsonValueTypeBoolean,
    JsonValueTypeArray,
    JsonValueTypeNull,
} JsonValueType;

typedef struct JsonStringRange {
    size_t start;
    size_t length;
} JsonStringRange;

typedef struct JsonArray {
    size_t contextIndex;
    size_t size;
} JsonArray;

typedef struct JsonObject {
    size_t contextIndex;
    size_t size;
} JsonObject;

typedef union JsonValueUnion {
    JsonStringRange string;
    float number;
    int boolean;
    JsonObject object;
    JsonArray array;
} JsonValueUnion;

typedef struct JsonValue {
    JsonValueType type;
    JsonValueUnion value;
} JsonValue;

typedef struct TableItem {
    int isBusy;
    size_t contextIndex;

    JsonStringRange name;

    int byIndex;
    size_t index;

    JsonValue typedValue;
} TableItem;

typedef struct JsonTable {
    size_t maxSize;
    size_t size;

    TableItem* buffer;

    char* stringBuffer;
} JsonTable;

typedef struct JsonDestination {
    int isRoot;
    JsonStringRange name;
    size_t ctx;

    int isIndex;
    size_t index;
} JsonDestination;

#endif // HASH_TABLE_H
