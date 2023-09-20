#include "hash-table.h"


#if !defined(PARSE_H)
#define PARSE_H


typedef struct Json {
    int parsed;
    Json_internal_Table* table;
} Json;

#define Json_create(name, elementsNumber)\
    Json_internal_TableItem name##__buffer[elementsNumber];\
    int name##__busy[elementsNumber];\
    int name##__byIndex[elementsNumber];\
    Json_internal_Table name##__table = {\
        .maxSize = elementsNumber,\
        .size = 0,\
        .buffer = name##__buffer,\
        .busy = name##__busy,\
        .byIndex = name##__byIndex,\
        .stringBuffer = NULL,\
    };\
    Json name##__data = {\
        .parsed = 0,\
        .table = &name##__table\
    };\
    Json* name = &name##__data;\

#define Json_createStatic(name, elementsNumber)\
    static Json_internal_TableItem name##__buffer[elementsNumber];\
    static int name##__busy[elementsNumber];\
    static int name##__byIndex[elementsNumber];\
    static Json_internal_Table name##__table = {\
        .maxSize = elementsNumber,\
        .size = 0,\
        .buffer = name##__buffer,\
        .busy = name##__busy,\
        .byIndex = name##__byIndex,\
        .stringBuffer = NULL,\
    };\
    static Json name##__data = {\
        .parsed = 0,\
        .table = &name##__table\
    };\
    static Json* name = &name##__data;\


Json* Json_allocate(size_t elementsNumber);
void Json_free(Json* json);

int Json_parse(Json* json);

void Json_reset(Json* json);

void Json_setSource(Json* json, char* src);

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

typedef struct JsonObjectIterator {
    Json* json;
    size_t ctx;
    size_t size;
    size_t index;
    size_t found;
} JsonObjectIterator;
int JsonObjectIterator_init(Json* json, JsonValue* obj, JsonObjectIterator* iterator);
typedef struct JsonProperty {
    JsonStringRange* name;
    JsonValue* value;
} JsonProperty;
int JsonObjectIterator_next(JsonObjectIterator* iterator, JsonProperty* property);
int JsonObjectIterator_index(JsonObjectIterator* iterator);

void JsonStringRange_copy(Json* json, JsonStringRange* range, char* buffer, size_t bufferSize);
void JsonStringRange_print(Json* json, JsonStringRange* range);

typedef struct JsonStringIterator {
    char* buffer;
    size_t index;
    size_t end;
} JsonStringIterator;

void JsonStringIterator_init(Json* json, JsonStringRange* range, JsonStringIterator* iterator);
char JsonStringIterator_next(JsonStringIterator* iterator);

int Json_asNumber(JsonValue* item, float* value);
int Json_asBoolean(JsonValue* item, int* value);
JsonStringRange* Json_asString(JsonValue* item);
int Json_asArray(JsonValue* item, size_t* length);
int Json_asObject(JsonValue* item, size_t* size);
int Json_isNull(JsonValue* item);

void Json_print(Json* json, JsonValue* root);
void Json_printType(JsonValue* root);

#endif // PARSE_H
