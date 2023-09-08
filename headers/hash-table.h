#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H

#include <stdlib.h>

typedef enum TableItemType {
    TableItemTypeString,
    TableItemTypeNumber,
    TableItemTypeObject,
    TableItemTypeBoolean,
    TableItemTypeArray,
    TableItemTypeNull,
} TableItemType;

typedef struct Range {
    size_t start;
    size_t length;
} Range;

typedef struct JsonArray {
    size_t contextIndex;
    size_t size;
} JsonArray;

typedef struct JsonObject {
    size_t contextIndex;
    size_t size;
} JsonObject;

typedef union TableItemValue {
    Range string;
    float number;
    int boolean;
    JsonObject object;
    JsonArray array;
} TableItemValue;

typedef struct JsonValue {
    TableItemType type;
    TableItemValue value;
} JsonValue;

typedef struct TableItem {
    int isBusy;
    size_t contextIndex;

    Range name;

    int byIndex;
    size_t index;

    JsonValue typedValue;
} TableItem;

typedef struct HashTable {
    size_t maxSize;
    size_t size;

    TableItem* buffer;

    char* stringBuffer;
} HashTable;

typedef struct Destination {
    int isRoot;
    Range name;
    size_t ctx;

    int isIndex;
    size_t index;
} Destination;

TableItem* HashTable_set(HashTable* table, Destination* dest);

TableItem* HashTable_get(HashTable* table, Destination* dest);
TableItem* HashTable_getByKey(HashTable* table, char* key, size_t contextIndex);
TableItem* HashTable_getByIndex(HashTable* table, size_t index, size_t contextIndex);

void HashTable_print(HashTable* table);

#endif // HASH_TABLE_H
