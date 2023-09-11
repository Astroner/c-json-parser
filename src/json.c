#include "json.h"
#include "logs.h"
#include "hash-table-methods.h"
#include "iterator.h"
#include "parser.h"
#include "utils.h"


#include <stdio.h>
#include <stdarg.h>


int Json_parse(Json* json) {
    LOGS_SCOPE();

    Json_internal_Iterator iterator = {
        .index = -1,
        .src = json->table->stringBuffer,
        .ended = 0
    };

    Json_internal_ParsingCtx ctx = {
        .nesting = NestedStructureNone,
    };

    Json_internal_Destination dest = {
        .isRoot = 1,
    };

    CHECK(Json_internal_parseValue(&iterator, &ctx, json, &dest), "Parsing basis");

    json->parsed = 1;
    return 0;
}

void Json_reset(Json* json) {
    json->parsed = 0;
    json->table->size = 0;
    memset(json->table->buffer, 0, json->table->maxSize * sizeof(json->table->buffer[0]));
}

void Json_setSource(Json* json, char* src) {
    Json_reset(json);
    json->table->stringBuffer = src;
}

JsonValue* Json_getRoot(Json* json) {
    if(!json->parsed) return NULL;

    return &json->table->buffer->typedValue;
}

int Json_asNumber(JsonValue* item, float* result) {
    if(item->type != Json_internal_TableValueTypeNumber) return 0;

    if(result) *result = item->value.number;

    return 1;
}

int Json_asBoolean(JsonValue* item, int* result) {
    if(!item || item->type != Json_internal_TableValueTypeBoolean) return 0;

    if(result) *result = item->value.boolean;

    return 1;
}

int Json_asString(Json* json, JsonValue* item, char* result, size_t bufferLength, size_t* actualLength) {
    if(!item || item->type != Json_internal_TableValueTypeString) return 0;

    if(actualLength) *actualLength = item->value.string.length + 1;

    if(!result) return 1;

    size_t limit;
    if(bufferLength >= item->value.string.length + 1) {
        limit = item->value.string.length;
    } else {
        limit = bufferLength - 1;
    }

    for(size_t i = 0; i < limit; i++) {
        result[i] = json->table->stringBuffer[i + item->value.string.start];
    }

    result[limit] = '\0';

    return 1;
}

int Json_asArray(JsonValue* item, size_t* length) {
    if(item->type != Json_internal_TableValueTypeArray) return 0;
    if(length) {
        *length = item->value.array.size;
    }

    return 1;
}
int Json_asObject(JsonValue* item, size_t* size) {
    if(item->type != Json_internal_TableValueTypeObject) return 0;
    if(size) {
        *size = item->value.object.size;
    }

    return 1;
}

int Json_isNull(JsonValue* item) {
    return item->type == Json_internal_TableValueTypeNull;
}

JsonValue* Json_getKey(Json* json, JsonValue* item, char* key) {
    if(!json->parsed) return NULL;

    if(item->type != Json_internal_TableValueTypeObject) return NULL;
    Json_internal_TableItem* result = Json_internal_Table_getByKey(json->table, key, item->value.object.contextIndex);
    
    if(!result) return NULL;

    return &result->typedValue;
}

JsonValue* Json_getIndex(Json* json, JsonValue* item, size_t index) {
    if(!json->parsed) return NULL;

    if(item->type != Json_internal_TableValueTypeArray) return NULL;

    Json_internal_TableItem* result = Json_internal_Table_getByIndex(json->table, index, item->value.array.contextIndex);

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
            Json_internal_printRange(json->table->stringBuffer, val->value.string.start, val->value.string.length);
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

            JsonField field;
            while(JsonObjectIterator_next(&iterator, &field)) {
                Json_internal_printSpacer(depth + 1);
                printf("\"");
                Json_internal_printRange(json->table->stringBuffer, field.name.start, field.name.length);
                printf("\": ");
                Json_internal_printValue(json, field.value, depth + 1);
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
    iterator->ctx = obj->value.object.contextIndex;
    iterator->size = obj->value.object.size;
    iterator->index = 0;
    iterator->found = 0;

    return 0;
}

int JsonObjectIterator_next(JsonObjectIterator* iterator, JsonField* field) {
    if(iterator->found == iterator->size || iterator->index == iterator->json->table->maxSize - 1) return 0;

    for(size_t i = iterator->index; i < iterator->json->table->maxSize; i++) {
        if(
            !iterator->json->table->buffer[i].isBusy ||
            iterator->json->table->buffer[i].contextIndex != iterator->ctx
        ) continue;

        iterator->index = i + 1;
        iterator->found++;

        field->name.start = iterator->json->table->buffer[i].name.start;
        field->name.length = iterator->json->table->buffer[i].name.length;
        field->value = &iterator->json->table->buffer[i].typedValue;

        return 1;
    }

    iterator->index = iterator->json->table->maxSize - 1;

    return 0;
}

int JsonObjectIterator_index(JsonObjectIterator* iterator) {
    if(iterator->found == 0) return 0;

    return iterator->found - 1;
}

size_t JsonField_name(Json* json, JsonField* field, char* buffer, size_t bufferSize) {
    size_t limit;
    if(bufferSize >= field->name.length + 1) {
        limit = field->name.length;
    } else {
        limit = bufferSize - 1;
    }

    for(size_t i = 0; i < limit; i++) {
        buffer[i] = json->table->stringBuffer[i + field->name.start];
    }

    buffer[limit] = '\0';

    return field->name.length;
}

void JsonStringRange_print(Json* json, JsonStringRange* range) {
    Json_internal_printRange(json->table->stringBuffer, range->start, range->length);
}

void JsonStringIterator_init(Json* json, JsonStringRange* range, JsonStringIterator* iterator) {
    iterator->index = range->start;
    iterator->end = range->start + range->length;
    iterator->buffer = json->table->stringBuffer;
}

char JsonStringIterator_next(JsonStringIterator* iterator) {
    if(iterator->index < iterator->end) return iterator->buffer[iterator->index++];
    return '\0';
}