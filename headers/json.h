
#include "hash-table.h"

#include <string.h>

#if !defined(PARSE_H)
#define PARSE_H


typedef struct Json {
    int parsed;
    Json_internal_Table* table;
} Json;

#define Json_createStatic(name, src, elementsNumber)\
    Json_internal_TableItem name##__buffer[elementsNumber];\
    memset(name##__buffer, 0, elementsNumber * sizeof(Json_internal_TableItem));\
    Json_internal_Table name##__table = {\
        .maxSize = elementsNumber,\
        .size = 0,\
        .buffer = name##__buffer,\
        .stringBuffer = src\
    };\
    Json name##__data = {\
        .parsed = 0,\
        .table = &name##__table\
    };\
    Json* name = &name##__data;\

int Json_parse(Json* json);

JsonValue* Json_getRoot(Json* json);
JsonValue* Json_getKey(Json* json, JsonValue* item, char* key);
JsonValue* Json_getIndex(Json* json, JsonValue* item, size_t index);

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

JsonValue* Json_chain(Json* json, JsonValue* item, JsonSelector* selectors, size_t selectorsSize);
JsonValue* Json_chainVA(Json* json, JsonValue* item, ...);

#define Json_chainM(resultName, json, root, ...)\
    JsonSelector resultName##__selectors[] = { __VA_ARGS__ };\
    JsonValue* resultName = Json_chain(json, root, resultName##__selectors, sizeof(resultName##__selectors) / sizeof(resultName##__selectors[0]));\

int Json_asNumber(JsonValue* item, float* result);
int Json_asBoolean(JsonValue* item, int* result);
int Json_asString(Json* json, JsonValue* item, char* result, size_t bufferLength, size_t* actualLength);
int Json_asArray(JsonValue* item, JsonArray* array);
int Json_asObject(JsonValue* item, JsonObject* obj);
int Json_isNull(JsonValue* item);

void Json_print(Json* json, JsonValue* root);
void Json_printType(JsonValue* root);

#endif // PARSE_H
