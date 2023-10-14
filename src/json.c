#include "json.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "logs.h"
#include "hash-table-methods.h"
#include "iterator.h"
#include "parser.h"
#include "utils.h"

Json* Json_allocate(size_t elementsNumber) {
    char* mem = (char*)malloc(
        sizeof(Json) // root structure
        + sizeof(int) * elementsNumber // busy flags
        + sizeof(int) * elementsNumber // isIndex flags
        + sizeof(Json_internal_TableItem) * elementsNumber // table buffer
        + sizeof(Json_internal_Table) // table
    );

    if(!mem) return NULL;

    Json* json = (void*)mem;

    char* busy = mem + sizeof(Json);
    char* byIndex = busy + sizeof(int) * elementsNumber;
    char* buffer = byIndex + sizeof(int) * elementsNumber;
    Json_internal_Table* table = (void*)(buffer + sizeof(Json_internal_TableItem) * elementsNumber);


    table->busy = (void*)busy;
    table->byIndex = (void*)byIndex;
    table->buffer = (void*)buffer;
    table->size = 0;
    table->maxSize = elementsNumber;

    json->parsed = 0;
    json->table = table;
    json->src = NULL;


    return json;
}

void Json_reset(Json* json) {
    json->parsed = 0;
    
    Json_internal_Table_reset(json->table);
}

void Json_setSource(Json* json, char* src) {
    json->parsed = 0;
    json->src = src;
}

void Json_free(Json* json) {
    free(json);
}

int Json_parse(Json* json) {
    LOGS_SCOPE();
    Json_reset(json);

    Json_internal_Iterator iterator = {
        .index = -1,
        .src = json->src,
        .finished = 0
    };

    Json_internal_ParsingCtx ctx = {
        .nesting = Json_internal_NestedStructureNone,
        .table = json->table,
        .namespaceCounter = 0
    };

    Json_internal_TableItem* root = Json_internal_Table_setRoot(json->table);

    CHECK(Json_internal_parseValue(&iterator, &ctx, &root->typedValue), "Parsing basis");

    json->parsed = 1;
    return 0;
}

JsonValue* Json_getRoot(Json* json) {
    if(!json->parsed) return NULL;

    return &json->table->buffer->typedValue;
}

int Json_asNumber(JsonValue* item, float* result) {
    if(!item || item->type != Json_internal_TableValueTypeNumber) return 0;

    if(result) *result = item->value.number;

    return 1;
}

int Json_asBoolean(JsonValue* item, int* result) {
    if(!item || item->type != Json_internal_TableValueTypeBoolean) return 0;

    if(result) *result = item->value.boolean;

    return 1;
}

JsonStringRange* Json_asString(JsonValue* item) {
    if(!item || item->type != Json_internal_TableValueTypeString) return NULL;

    return &item->value.string;
}

int Json_asArray(JsonValue* item, size_t* length) {
    if(!item || item->type != Json_internal_TableValueTypeArray) return 0;
    if(length) {
        *length = item->value.array.size;
    }

    return 1;
}

int Json_asObject(JsonValue* item, size_t* size) {
    if(!item || item->type != Json_internal_TableValueTypeObject) return 0;
    if(size) {
        *size = item->value.object.size;
    }

    return 1;
}

int Json_isNull(JsonValue* item) {
    return item && item->type == Json_internal_TableValueTypeNull;
}

JsonValue* Json_getKey(Json* json, JsonValue* item, char* key) {
    if(!json->parsed || !item) return NULL;

    if(item->type != Json_internal_TableValueTypeObject) return NULL;
    Json_internal_TableItem* result = Json_internal_Table_getByKey(json->table, key, item->value.object.namespaceID);
    
    if(!result) return NULL;

    return &result->typedValue;
}

JsonValue* Json_getIndex(Json* json, JsonValue* item, size_t index) {
    if(!json->parsed || !item) return NULL;

    if(item->type != Json_internal_TableValueTypeArray) return NULL;

    Json_internal_TableItem* result = Json_internal_Table_getByIndex(json->table, index, item->value.array.namespaceID);

    if(!result) return NULL;

    return &result->typedValue;
}


static void Json_internal_printSpacer(size_t times) {
    for(size_t i = 0; i < times; i++) {
        printf("    ");
    }
}

static void Json_internal_printValue(Json* json, JsonValue* val, size_t depth) {
    switch(val->type) {
        case Json_internal_TableValueTypeNumber:
            printf("%f", val->value.number);
            break;

        case Json_internal_TableValueTypeBoolean:
            printf("%s", val->value.boolean ? "true" : "false");
            break;

        case Json_internal_TableValueTypeString: {
            printf("\"");
            Json_internal_printRange(json->src, val->value.string.start, val->value.string.length);
            printf("\"");
            break;
        }

        case Json_internal_TableValueTypeNull:
            printf("null");
            break;
        
        case Json_internal_TableValueTypeArray: 
            if(val->value.array.size == 0) {
                printf("[]");
                break;
            }
            printf("[\n");
            for(size_t i = 0; i < val->value.array.size; i++) {
                Json_internal_printSpacer(depth + 1);
                Json_internal_printValue(json, Json_getIndex(json, val, i), depth + 1);
                if(i != val->value.array.size - 1) printf(",");
                printf("\n");
            }
            Json_internal_printSpacer(depth);
            printf("]");

            break;

        case Json_internal_TableValueTypeObject: {
            if(val->value.object.size == 0) {
                printf("{}");
                break;
            }
            printf("{\n");

            JsonObjectIterator iterator;
            JsonObjectIterator_init(json, val, &iterator);

            JsonProperty property;
            while(JsonObjectIterator_next(&iterator, &property)) {
                Json_internal_printSpacer(depth + 1);
                printf("\"");
                JsonStringRange_print(json, property.name);
                printf("\": ");
                Json_internal_printValue(json, property.value, depth + 1);
                if(iterator.found < val->value.object.size) {
                    printf(",");
                }
                printf("\n");
            }
            Json_internal_printSpacer(depth);
            printf("}");

            break;
        }
    }
}

void Json_print(Json* json, JsonValue* root) {
    if(!json->parsed) return;
    if(!root) {
        printf("NULL\n");
        return;
    }

    Json_internal_printValue(json, root, 0);

    printf("\n");
}

JsonValue* Json_chain(Json* json, JsonValue* item, JsonSelector* selectors, size_t selectorsSize) {
    JsonValue* current = item;
    for(size_t i = 0; i < selectorsSize; i++) {
        if(selectors[i].type == JsonKey) {
            current = Json_getKey(json, current, selectors[i].q.key);
        } else {
            current = Json_getIndex(json, current, selectors[i].q.index);
        }

        if(!current) {
            return NULL;
        }
    }

    return current;
}

JsonValue* Json_chainVA(Json* json, JsonValue* item, ...) {
    va_list args;

    va_start(args, item);

    JsonValue* current = item;
    char* selector;
    while((selector = va_arg(args, char*))) {
        if(selector[0] == '!' && selector[1] == '+') {
            selector += 2;
            size_t index = 0;
            char ch;
            while((ch = *selector++)) {
                index *= 10;
                index += ch - '0';
            }
            current = Json_getIndex(json, current, index);
        } else {
            current = Json_getKey(json, current, selector);
        }

        if(!current) {
            va_end(args);
            
            return NULL;
        }
    }

    va_end(args);

    return current;
}

void Json_printType(JsonValue* root) {
    if(!root) {
        printf("Empty pointer\n");
        return;
    }

    switch(root->type) {
        case Json_internal_TableValueTypeArray:
            printf("Array\n");
            break;

        case Json_internal_TableValueTypeObject:
            printf("Object\n");
            break;

        case Json_internal_TableValueTypeBoolean:
            printf("Boolean\n");
            break;

        case Json_internal_TableValueTypeNull:
            printf("Null\n");
            break;

        case Json_internal_TableValueTypeNumber:
            printf("Null\n");
            break;

        case Json_internal_TableValueTypeString:
            printf("String\n");
            break;
    }
}

int JsonObjectIterator_init(Json* json, JsonValue* obj, JsonObjectIterator* iterator) {
    if(obj->type != Json_internal_TableValueTypeObject) {
        iterator->found = obj->value.object.size;
        return -1;
    }

    iterator->json = json;
    iterator->namespaceID = obj->value.object.namespaceID;
    iterator->size = obj->value.object.size;
    iterator->index = 0;
    iterator->found = 0;

    return 0;
}

int JsonObjectIterator_next(JsonObjectIterator* iterator, JsonProperty* property) {
    if(iterator->found == iterator->size || iterator->index == iterator->json->table->maxSize - 1) return 0;

    for(size_t i = iterator->index; i < iterator->json->table->maxSize; i++) {
        if(
            !iterator->json->table->busy[i] ||
            iterator->json->table->buffer[i].namespaceID != iterator->namespaceID
        ) continue;

        iterator->index = i + 1;
        iterator->found++;

        property->name = &iterator->json->table->buffer[i].name;
        property->value = &iterator->json->table->buffer[i].typedValue;

        return 1;
    }

    iterator->index = iterator->json->table->maxSize - 1;

    return 0;
}

int JsonObjectIterator_index(JsonObjectIterator* iterator) {
    if(iterator->found == 0) return 0;

    return iterator->found - 1;
}

void JsonStringRange_copy(Json* json, JsonStringRange* range, char* buffer, size_t bufferSize) {
    size_t limit;
    if(bufferSize >= range->length + 1) {
        limit = range->length;
    } else {
        limit = bufferSize - 1;
    }

    for(size_t i = 0; i < limit; i++) {
        buffer[i] = json->src[i + range->start];
    }

    buffer[limit] = '\0';
}

void JsonStringRange_print(Json* json, JsonStringRange* range) {
    Json_internal_printRange(json->src, range->start, range->length);
}

void JsonStringIterator_init(Json* json, JsonStringRange* range, JsonStringIterator* iterator) {
    iterator->index = range->start;
    iterator->end = range->start + range->length;
    iterator->buffer = json->src;
}

char JsonStringIterator_next(JsonStringIterator* iterator) {
    if(iterator->index < iterator->end) return iterator->buffer[iterator->index++];
    return '\0';
}