#if !defined(HASH_TABLE_H)
#define HASH_TABLE_H

#include <stddef.h>

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
    int isBusy;
    size_t contextIndex;

    JsonStringRange name;

    int byIndex;
    size_t index;

    JsonValue typedValue;
} Json_internal_TableItem;

typedef struct Json_internal_Table {
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



#include <string.h>

#if !defined(PARSE_H)
#define PARSE_H


typedef struct Json {
    int parsed;
    Json_internal_Table* table;
} Json;

#define Json_createStatic(name, src, elementsNumber)\
    Json_internal_TableItem name##__buffer[elementsNumber];\
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
#if defined(JSON_IMPLEMENTATION)



#include <string.h>
#include <stdio.h>

static unsigned long Json_internal_hashChars(char* str, size_t contextIndex) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashIndex(size_t index, size_t contextIndex) {
    unsigned long hash = 5381;
    
    unsigned char* indexBytes = (void*)&index;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + indexBytes[i];


    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashKey(char* strBuffer, size_t nameStart, size_t nameLength, size_t contextIndex) {
    unsigned long hash = 5381;

    for(size_t i = nameStart; i < nameStart + nameLength; i++)
        hash = ((hash << 5) + hash) + strBuffer[i];

    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];
    
    return hash;
}


Json_internal_TableItem* Json_internal_Table_set(Json_internal_Table* table, Json_internal_Destination* dest) {
    if(dest->isRoot) {
        table->buffer->isBusy = 1;

        table->buffer->name.start = dest->name.start;
        table->buffer->name.length = dest->name.length;
        table->buffer->contextIndex = dest->ctx;

        return table->buffer;
    }

    if(table->size == table->maxSize) return NULL;

    unsigned long hash;
    if(dest->isIndex) {
        hash = Json_internal_hashIndex(dest->index, dest->ctx);
    } else {
        hash = Json_internal_hashKey(table->stringBuffer, dest->name.start, dest->name.length, dest->ctx);
    }


    size_t startIndex = hash % table->maxSize;

    size_t index = startIndex;
    while(table->buffer[index].isBusy) {
        index++;
        if(index == table->maxSize) {
            index = 0;
        }
        if(index == startIndex) {
            return NULL;
        }
    }

    table->buffer[index].isBusy = 1;

    table->buffer[index].contextIndex = dest->ctx;

    if(dest->isIndex) {
        table->buffer[index].index = dest->index;
        table->buffer[index].byIndex = 1;
    } else {
        table->buffer[index].name.start = dest->name.start;
        table->buffer[index].name.length = dest->name.length;
    }
    
    table->size++;

    return table->buffer + index;
}

void Json_internal_Table_print(Json_internal_Table* table) {
    for(size_t i = 0; i < table->maxSize; i++) {
        Json_internal_TableItem* current = table->buffer + i;
        printf("%zu) %s\n", i, current->isBusy ? "BUSY" : "FREE");
    }
}

Json_internal_TableItem* Json_internal_Table_getByKey(Json_internal_Table* table, char* key, size_t contextIndex) {
    unsigned long hash = Json_internal_hashChars(key, contextIndex);

    size_t startIndex = hash % table->maxSize;
    
    size_t index = startIndex;
    Json_internal_TableItem* current = table->buffer + index;

    while(
        !current->isBusy 
        || current->byIndex
        || current->contextIndex != contextIndex 
        || strncmp(key, table->stringBuffer + current->name.start, current->name.length) != 0
    ) {
        index++;
        if(index == table->maxSize) {
            index = 0;
        }
        if(index == startIndex) {
            return NULL;
        }
        current = table->buffer + index;
    }
    return current;
}

Json_internal_TableItem* Json_internal_Table_getByIndex(Json_internal_Table* table, size_t index, size_t contextIndex) {

    unsigned long hash = Json_internal_hashIndex(index, contextIndex);

    size_t startIndex = hash % table->maxSize;

    size_t bufferIndex = startIndex;
    
    Json_internal_TableItem* current = table->buffer + bufferIndex;

    while(
        !current->isBusy 
        || !current->byIndex
        || current->contextIndex != contextIndex 
        || current->index != index
    ) {
        bufferIndex++;
        if(bufferIndex == table->maxSize) {
            bufferIndex = 0;
        }
        if(bufferIndex == startIndex) {
            return NULL;
        }
        current = table->buffer + bufferIndex;
    }
    
    return current;
}

#include <stddef.h>

#if !defined(UTILS_H)
#define UTILS_H

#define Json_internal_isWhiteSpace(name) ((name) == '\n' || (name) == '\r' || (name) == ' ')

void Json_internal_printRange(char* str, size_t start, size_t length);


#endif // UTILS_H




#include <stdio.h>

void Json_internal_printRange(char* str, size_t start, size_t length) {
    for(size_t i = start; i < start + length; i++) {
        putchar(str[i]);
    }
}

#if !defined(LOGS_H)
#define LOGS_H

#include <stdio.h>

#if defined(JSON_DO_LOGS)
    #define LOGS_SCOPE(name)\
        const char* logs__scope__name = #name;\
        if(!logs__scope__name[0]) logs__scope__name = __func__;\
        printf("LOGS '%s'\n", logs__scope__name);\

    #define LOG(...)\
        do {\
            printf("LOGS '%s' ", logs__scope__name);\
            printf(__VA_ARGS__);\
        } while(0);\
    
    #define LOG_LN(...)\
        do {\
            LOG(__VA_ARGS__);\
            printf("\n");\
        } while(0);\

    #define CHECK(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, LOCAL_STATUS);\
                printf(__VA_ARGS__);\
                printf("\n");\
                return LOCAL_STATUS;\
            }\
        } while(0);\

    
    #define CHECK_BOOLEAN(condition, status, ...)\
        do {\
            if(!(condition)) {\
                printf("LOGS '%s' Failed bool check ", logs__scope__name);\
                printf(__VA_ARGS__);\
                printf("\n");\
                return (status);\
            }\
        } while(0);\

    #define LOGS_ONLY(code) code

    #define CHECK_ELSE(condition, elseCode, ...)\
        do {\
            int CONDITION_VALUE = (condition);\
            if(CONDITION_VALUE < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, CONDITION_VALUE);\
                printf(__VA_ARGS__);\
                printf("\n");\
                elseCode;\
            }\
        } while(0);
    
    #define CHECK_RETURN(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) {\
                printf("LOGS '%s' Status: %d  ", logs__scope__name, LOCAL_STATUS);\
                printf(__VA_ARGS__);\
                printf("\n");\
            }\
            return LOCAL_STATUS;\
        } while(0);\

#else
    #define LOGS_SCOPE(name)
    #define LOG(...)
    #define LOG_LN(...)

    #define CHECK(condition, ...)\
        do {\
            int LOCAL_STATUS = (condition);\
            if(LOCAL_STATUS < 0) return LOCAL_STATUS;\
        } while(0);\

    
    #define CHECK_BOOLEAN(condition, status, ...)\
        do {\
            if(!(condition)) return (status);\
        } while(0);\

    #define LOGS_ONLY(code)

    #define CHECK_ELSE(condition, elseCode, ...)\
        do {\
            int CONDITION_VALUE = (condition);\
            if(CONDITION_VALUE < 0) {\
                elseCode;\
            }\
        } while(0);
    
    #define CHECK_RETURN(condition, ...) return (condition);

#endif // DO_LOGS


#endif // LOGS_H


#include <stddef.h>


#if !defined(ITERATOR_H)
#define ITERATOR_H

typedef struct Json_internal_Iterator {
    char* src;
    char current;
    size_t index;
    int ended;
} Json_internal_Iterator;

char Json_internal_Iterator_next(Json_internal_Iterator* iterator);
int Json_internal_Iterator_skipSpaceTo(Json_internal_Iterator* iterator, char stopper);
int Json_internal_Iterator_nextCharIs(Json_internal_Iterator* iterator, char stopper);

#endif // ITERATOR_H




char Json_internal_Iterator_next(Json_internal_Iterator* iterator) {
    if(iterator->ended) return '\0';

    iterator->current = iterator->src[++iterator->index];

    if(iterator->current == '\0') iterator->ended = 1;

    return iterator->current;
}

int Json_internal_Iterator_skipSpaceTo(Json_internal_Iterator* iterator, char stopper) {
    LOGS_SCOPE();
    LOGS_ONLY(
        LOG_LN("To '%c'", stopper);
    )

    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(ch == stopper) return 0;
        if(ch != '\n' && ch != '\r' && ch != ' ') return -1;
    }

    return -1;
}

int Json_internal_Iterator_nextCharIs(Json_internal_Iterator* iterator, char stopper) {
    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        if(Json_internal_isWhiteSpace(ch)) continue;
        if(ch == stopper) return 1;
        return 0;
    }

    return 0;
}



#if !defined(PARSER_H)
#define PARSER_H

typedef enum Json_internal_ParsingStatus {
    ParsingStatusError = -1,
    ParsingStatusOk = 0,

    ParsingStatusEndedWithComma = 1,
    ParsingStatusCommaOnly = 2,

    ParsingStatusEndedWithCurly = 3,
    ParsingStatusCurlyOnly = 4,

    ParsingStatusEndedWithSquare = 5,
    ParsingStatusSquareOnly = 6,
} Json_internal_ParsingStatus;

typedef enum Json_internal_NestedStructure {
    NestedStructureNone,
    NestedStructureObject,
    NestedStructureArray
} Json_internal_NestedStructure;

typedef struct Json_internal_ParsingCtx {
    Json_internal_NestedStructure nesting;
    size_t ctxCounter;
} Json_internal_ParsingCtx;

Json_internal_ParsingStatus Json_internal_parseValue(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, Json* json, Json_internal_Destination* dest);

#endif // PARSER_H





static Json_internal_ParsingStatus Json_internal_parseArray(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();
    
    size_t arrayIndex = ctx->ctxCounter + 1;

    Json_internal_ParsingCtx localCtx = {
        .nesting = NestedStructureArray,
        .ctxCounter = arrayIndex
    };

    Json_internal_TableItem* arr;
    CHECK_BOOLEAN((arr = Json_internal_Table_set(json->table, dest)), -1, "Setting array to the table");
    arr->typedValue.type = Json_internal_TableValueTypeArray;
    arr->typedValue.value.array.contextIndex = arrayIndex;
    arr->typedValue.value.array.size = 0;


    size_t arrSize = 0;
    while(1) {
        Json_internal_Destination elementDestination = {
            .isIndex = 1,
            .index = arrSize,
            .ctx = arrayIndex
        };

        Json_internal_ParsingStatus status = Json_internal_parseValue(iterator, &localCtx, json, &elementDestination);

        
        if(status == ParsingStatusOk) {

            arrSize++;

            if(Json_internal_Iterator_nextCharIs(iterator, ',')) {
                LOG_LN("Next char is comma: Parsing next element");

                continue;
            }
            if(iterator->current == ']') {
                LOG_LN("Next char is square bracket: end of array")

                break;
            }

            LOG_LN("Unexpected char '%c'", iterator->current);
            return ParsingStatusError;
        }

        if(status == ParsingStatusEndedWithComma) {

            arrSize++;
            LOG_LN("Element ended with comma: parsing next element");
            continue;
        }
        if(status == ParsingStatusEndedWithSquare) {
            LOG_LN("Element ended with square bracket: end of array");
            arrSize++;
            break;
        }
        if(status == ParsingStatusCommaOnly) {
            LOG_LN("Unexpected comma");

            return ParsingStatusError;
        }
        if(status == ParsingStatusSquareOnly) {
            if(arrSize > 0) {
                LOG_LN("Unexpected square")
                return ParsingStatusError;
            }
            LOG_LN("Empty array");
            break;
        }
        if(status == ParsingStatusError) {
            LOG_LN("Failed to parse field");

            return ParsingStatusError;
        }
    }

    ctx->ctxCounter = localCtx.ctxCounter;
    arr->typedValue.value.array.size = arrSize;

    return ParsingStatusOk;
}

static Json_internal_ParsingStatus Json_internal_parseString(Json_internal_Iterator* iterator, JsonStringRange* result) {
    LOGS_SCOPE();

    size_t start = iterator->index + 1;
    size_t length = 0;

    int isLiteral = 0;
    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(ch == '"') {
            if(isLiteral) {
                isLiteral = 0;
                length++;
                continue;
            }
            LOG_LN("String start: %zu, Length: %zu", start, length);
            LOG("Content: \"");
            LOGS_ONLY(
                Json_internal_printRange(iterator->src, start, length);
                printf("\"\n");
            )

            result->start = start;
            result->length = length;

            return ParsingStatusOk;
        } else if(ch == '\\') {
            if(isLiteral) {
                isLiteral = 0;
            } else {
                LOG_LN("Make next literal");
                isLiteral = 1;
            }
            length++;
        } else if(ch == '\n') {
            LOG_LN("Error: new line inside of a string");
            return ParsingStatusError;
        } else {
            length++;
        }
    }
    
    LOG_LN("Unexpected EOI");
    return ParsingStatusError;
}

static Json_internal_ParsingStatus Json_internal_parseNumber(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    int isNegative = iterator->current == '-';

    LOGS_ONLY(
        if(isNegative) {
            LOG_LN("Negative");
        }
    );

    float result = isNegative ? 0.f : (float)(iterator->current - '0');

    LOG_LN("Initial value: %f", result);

    int started = 0;
    float afterPointPower = 0;
    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);

        if(ch >= '0' && ch <= '9') {
            started = 1;
            if(afterPointPower) {
                result += (float)(ch - '0') / afterPointPower;
                afterPointPower *= 10;
            } else {
                result *= 10;
                result += (float)(ch - '0');
            }
        } else if(ch == '.' && !afterPointPower) {
            LOG_LN("Got point");
            afterPointPower = 10;
        } else if(Json_internal_isWhiteSpace(ch)){
            LOG_LN("Number ended by whitespace");
            break;
        } else if(!started && isNegative) { // "{ "a": - }" scenario handling
            LOG_LN("Unexpected char");
            return ParsingStatusError;
        } else if(ctx->nesting != NestedStructureNone && ch == ',') {
            LOG_LN("Ended by comma in nested structure");

            break;
        } else if(ctx->nesting == NestedStructureObject && ch == '}') {
            LOG_LN("Ended as last object field");

            break;
        } else if(ctx->nesting == NestedStructureArray && ch == ']') {
            LOG_LN("Ended as last array element");

            break;
        } else {
            LOG_LN("Unexpected char");
            return ParsingStatusError;
        }
    }

    if(iterator->current == '\0' && ctx->nesting != NestedStructureNone) {
        LOG_LN("Got EOI inside of nested structure");
        return ParsingStatusError;
    }

    if(isNegative) result *= -1;

    LOG_LN("Result: %f", result);

    Json_internal_TableItem* item;
    CHECK_BOOLEAN(item = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = Json_internal_TableValueTypeNumber;
    item->typedValue.value.number = result;


    if(ctx->nesting != NestedStructureNone && ch == ',') {
        return ParsingStatusEndedWithComma;
    }

    if(ctx->nesting == NestedStructureObject && ch == '}') {
        return ParsingStatusEndedWithCurly;
    }

    if(ctx->nesting == NestedStructureArray && ch == ']') {
        return ParsingStatusEndedWithSquare;
    }

    return ParsingStatusOk;
}

static Json_internal_ParsingStatus Json_internal_parseTrue(Json_internal_Iterator* iterator, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'r') return -1;
    if(Json_internal_Iterator_next(iterator) != 'u') return -1;
    if(Json_internal_Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("true");


    Json_internal_TableItem* item;
    CHECK_BOOLEAN(item = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = Json_internal_TableValueTypeBoolean;
    item->typedValue.value.boolean = 1;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseFalse(Json_internal_Iterator* iterator, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'a') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    if(Json_internal_Iterator_next(iterator) != 's') return -1;
    if(Json_internal_Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("false");

    Json_internal_TableItem* item;
    CHECK_BOOLEAN(item = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = Json_internal_TableValueTypeBoolean;
    item->typedValue.value.boolean = 0;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseNull(Json_internal_Iterator* iterator, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'u') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    
    LOG_LN("null");

    Json_internal_TableItem* item;
    CHECK_BOOLEAN(item = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = Json_internal_TableValueTypeNull;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseField(Json_internal_Iterator* iterator, size_t objectCtx, Json_internal_ParsingCtx* ctx, Json* json) {
    LOGS_SCOPE();

    int status = Json_internal_Iterator_skipSpaceTo(iterator, '"');

    if(status < 0) {
        if(iterator->current == ',') return ParsingStatusCommaOnly;
        if(iterator->current == '}') return ParsingStatusCurlyOnly;

        LOG_LN("Unexpected char '%c'\n", iterator->current);
        return ParsingStatusError; 
    }


    Json_internal_Destination fieldDest = {
        .isRoot = 0,
        .ctx = objectCtx
    };
    LOG_LN("Parsing key string")
    CHECK(Json_internal_parseString(iterator, &fieldDest.name), "Parse key");

    CHECK(Json_internal_Iterator_skipSpaceTo(iterator, ':'), "Skipping chars to ':'");

    CHECK_RETURN(Json_internal_parseValue(iterator, ctx, json, &fieldDest), "Field value");
}

static Json_internal_ParsingStatus Json_internal_parseObject(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    size_t objIndex = ctx->ctxCounter + 1;

    Json_internal_ParsingCtx localCtx = {
        .nesting = NestedStructureObject,
        .ctxCounter = objIndex
    };


    Json_internal_TableItem* obj;
    CHECK_BOOLEAN(obj = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
    obj->typedValue.type = Json_internal_TableValueTypeObject;
    obj->typedValue.value.object.contextIndex = objIndex;
    obj->typedValue.value.object.size = 0;

    size_t size = 0;
    while(1) {
        Json_internal_ParsingStatus status = Json_internal_parseField(iterator, objIndex, &localCtx, json);

        if(status == ParsingStatusOk) {
            size++;
            if(Json_internal_Iterator_nextCharIs(iterator, ',')) {
                LOG_LN("Next char is comma: Parsing next field");
                continue;
            }
            if(iterator->current == '}') {
                LOG_LN("Next char is curly: End of object");
                break;
            }

            LOG_LN("Unexpected char '%c', expected comma\n", iterator->current);

            return -1;
        }
        if(status == ParsingStatusEndedWithComma) {
            size++;
            LOG_LN("Field ended with comma: parsing next field");
            continue;
        }
        if(status == ParsingStatusEndedWithCurly) {
            LOG_LN("Field ended with curly: end of object");
            size++;
            break;
        }
        if(status == ParsingStatusCommaOnly) {
            LOG_LN("Unexpected comma");

            return ParsingStatusError;
        }
        if(status == ParsingStatusCurlyOnly) {
            if(size > 0) {
                LOG_LN("Unexpected curly")
                return ParsingStatusError;
            }
            LOG_LN("Empty object");
            break;
        }
        if(status == ParsingStatusError) {
            LOG_LN("Failed to parse field");

            return ParsingStatusError;
        }
    }

    ctx->ctxCounter = localCtx.ctxCounter;
    obj->typedValue.value.object.size = size;

    return ParsingStatusOk;
}

Json_internal_ParsingStatus Json_internal_parseValue(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, Json* json, Json_internal_Destination* dest) {
    LOGS_SCOPE();

    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(Json_internal_isWhiteSpace(ch)) {
            continue;
        } else if(ch == '{') {
            return Json_internal_parseObject(iterator, ctx, json, dest);
        } else if(ch == '[') {
            return Json_internal_parseArray(iterator, ctx, json, dest);
        } else if(ch == '"') {
            JsonStringRange string;
            CHECK(Json_internal_parseString(iterator, &string), "Parsing string");

            Json_internal_TableItem* item;
            CHECK_BOOLEAN(item = Json_internal_Table_set(json->table, dest), -1, "Setting value into the hash table");
            item->typedValue.type = Json_internal_TableValueTypeString;
            item->typedValue.value.string.start = string.start;
            item->typedValue.value.string.length = string.length;

            return ParsingStatusOk;
        } else if(ch == 't') {
            return Json_internal_parseTrue(iterator, json, dest);
        } else if(ch == 'f') {
            return Json_internal_parseFalse(iterator, json, dest);
        } else if(ch == 'n') {
            return Json_internal_parseNull(iterator, json, dest);
        } else if((ch >= '0' && ch <= '9') || ch == '-') {
            return Json_internal_parseNumber(iterator, ctx, json, dest);
        } else if(ctx->nesting == NestedStructureArray && ch == ']') {
            return ParsingStatusSquareOnly;
        } else {
            LOG_LN("Unexpected char '%c'", ch);
            return ParsingStatusError;
        }
    }

    LOG_LN("Unexpected EOI");
    return ParsingStatusError;
}



#include <stdio.h>
#include <stdarg.h>


int Json_parse(Json* json) {
    LOGS_SCOPE();
    Json_reset(json);

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

JsonStringRange* Json_asString(JsonValue* item) {
    if(!item || item->type != Json_internal_TableValueTypeString) return NULL;

    return &item->value.string;
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
    iterator->ctx = obj->value.object.contextIndex;
    iterator->size = obj->value.object.size;
    iterator->index = 0;
    iterator->found = 0;

    return 0;
}

int JsonObjectIterator_next(JsonObjectIterator* iterator, JsonProperty* property) {
    if(iterator->found == iterator->size || iterator->index == iterator->json->table->maxSize - 1) return 0;

    for(size_t i = iterator->index; i < iterator->json->table->maxSize; i++) {
        if(
            !iterator->json->table->buffer[i].isBusy ||
            iterator->json->table->buffer[i].contextIndex != iterator->ctx
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
        buffer[i] = json->table->stringBuffer[i + range->start];
    }

    buffer[limit] = '\0';
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

#endif // JSON_IMPLEMENTATION

