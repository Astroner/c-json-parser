#include "json.h"

#include <stdio.h>
#include <stdarg.h>

#include "logs.h"
#include "hash-table-methods.h"

typedef struct Iterator {
    char* src;
    char current;
    size_t index;
    int ended;
} Iterator;

typedef enum ParsingStatus {
    ParsingStatusError = -1,
    ParsingStatusOk = 0,

    ParsingStatusEndedWithComma = 1,
    ParsingStatusCommaOnly = 2,

    ParsingStatusEndedWithCurly = 3,
    ParsingStatusCurlyOnly = 4,

    ParsingStatusEndedWithSquare = 5,
    ParsingStatusSquareOnly = 6,
} ParsingStatus;

typedef enum NestedStructure {
    NestedStructureNone,
    NestedStructureObject,
    NestedStructureArray
} NestedStructure;

typedef struct ParsingCtx {
    NestedStructure nesting;
    size_t ctxCounter;
} ParsingCtx;

#define isWhiteSpace(name) ((name) == '\n' || (name) == '\r' || (name) == ' ')

static int parseValue(Iterator* iterator, ParsingCtx* ctx, Json* json, JsonDestination* dest);

static char Iterator_next(Iterator* iterator) {
    if(iterator->ended) return '\0';

    iterator->current = iterator->src[++iterator->index];

    if(iterator->current == '\0') iterator->ended = 1;

    return iterator->current;
}

static int Iterator_skipSpaceTo(Iterator* iterator, char stopper) {
    LOGS_SCOPE();
    LOGS_ONLY(
        LOG_LN("To '%c'", stopper);
    )

    char ch;
    while((ch = Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(ch == stopper) return 0;
        if(ch != '\n' && ch != '\r' && ch != ' ') return -1;
    }

    return -1;
}

static int Iterator_nextCharIs(Iterator* iterator, char stopper) {
    char ch;
    while((ch = Iterator_next(iterator))) {
        if(isWhiteSpace(ch)) continue;
        if(ch == stopper) return 1;
        return 0;
    }

    return 0;
}

static void printRange(char* str, size_t start, size_t length) {
    for(size_t i = start; i < start + length; i++) {
        printf("%c", str[i]);
    }
}


static ParsingStatus parseArray(Iterator* iterator, ParsingCtx* ctx, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();
    
    size_t arrayIndex = ctx->ctxCounter + 1;

    ParsingCtx localCtx = {
        .nesting = NestedStructureArray,
        .ctxCounter = arrayIndex
    };

    TableItem* arr;
    CHECK_BOOLEAN((arr = JsonTable_set(json->table, dest)), -1, "Setting array to the table");
    arr->typedValue.type = JsonValueTypeArray;
    arr->typedValue.value.array.contextIndex = arrayIndex;
    arr->typedValue.value.array.size = 0;


    size_t arrSize = 0;
    while(1) {
        JsonDestination elementDestination = {
            .isIndex = 1,
            .index = arrSize,
            .ctx = arrayIndex
        };

        ParsingStatus status = parseValue(iterator, &localCtx, json, &elementDestination);

        
        if(status == ParsingStatusOk) {

            arrSize++;

            if(Iterator_nextCharIs(iterator, ',')) {
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

static ParsingStatus parseString(Iterator* iterator, JsonStringRange* result) {
    LOGS_SCOPE();

    size_t start = iterator->index + 1;
    size_t length = 0;

    int isLiteral = 0;
    char ch;
    while((ch = Iterator_next(iterator))) {
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
                printRange(iterator->src, start, length);
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

static ParsingStatus parseNumber(Iterator* iterator, ParsingCtx* ctx, Json* json, JsonDestination* dest) {
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
    while((ch = Iterator_next(iterator))) {
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
        } else if(isWhiteSpace(ch)){
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

    TableItem* item;
    CHECK_BOOLEAN(item = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = JsonValueTypeNumber;
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

static ParsingStatus parseTrue(Iterator* iterator, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();

    if(Iterator_next(iterator) != 'r') return -1;
    if(Iterator_next(iterator) != 'u') return -1;
    if(Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("true");


    TableItem* item;
    CHECK_BOOLEAN(item = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = JsonValueTypeBoolean;
    item->typedValue.value.boolean = 1;

    return 0;
}

static ParsingStatus parseFalse(Iterator* iterator, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();

    if(Iterator_next(iterator) != 'a') return -1;
    if(Iterator_next(iterator) != 'l') return -1;
    if(Iterator_next(iterator) != 's') return -1;
    if(Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("false");

    TableItem* item;
    CHECK_BOOLEAN(item = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = JsonValueTypeBoolean;
    item->typedValue.value.boolean = 0;

    return 0;
}

static ParsingStatus parseNull(Iterator* iterator, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();

    if(Iterator_next(iterator) != 'u') return -1;
    if(Iterator_next(iterator) != 'l') return -1;
    if(Iterator_next(iterator) != 'l') return -1;
    
    LOG_LN("null");

    TableItem* item;
    CHECK_BOOLEAN(item = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
    item->typedValue.type = JsonValueTypeNull;

    return 0;
}

static ParsingStatus parseField(Iterator* iterator, size_t objectCtx, ParsingCtx* ctx, Json* json) {
    LOGS_SCOPE();

    int status = Iterator_skipSpaceTo(iterator, '"');

    if(status < 0) {
        if(iterator->current == ',') return ParsingStatusCommaOnly;
        if(iterator->current == '}') return ParsingStatusCurlyOnly;

        LOG_LN("Unexpected char '%c'\n", iterator->current);
        return ParsingStatusError; 
    }


    JsonDestination fieldDest = {
        .isRoot = 0,
        .ctx = objectCtx
    };
    LOG_LN("Parsing key string")
    CHECK(parseString(iterator, &fieldDest.name), "Parse key");

    CHECK(Iterator_skipSpaceTo(iterator, ':'), "Skipping chars to ':'");

    CHECK_RETURN(parseValue(iterator, ctx, json, &fieldDest), "Field value");
}

static ParsingStatus parseObject(Iterator* iterator, ParsingCtx* ctx, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();

    size_t objIndex = ctx->ctxCounter + 1;

    ParsingCtx localCtx = {
        .nesting = NestedStructureObject,
        .ctxCounter = objIndex
    };


    TableItem* obj;
    CHECK_BOOLEAN(obj = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
    obj->typedValue.type = JsonValueTypeObject;
    obj->typedValue.value.object.contextIndex = objIndex;
    obj->typedValue.value.object.size = 0;

    size_t size = 0;
    while(1) {
        ParsingStatus status = parseField(iterator, objIndex, &localCtx, json);

        if(status == ParsingStatusOk) {
            size++;
            if(Iterator_nextCharIs(iterator, ',')) {
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

static ParsingStatus parseValue(Iterator* iterator, ParsingCtx* ctx, Json* json, JsonDestination* dest) {
    LOGS_SCOPE();

    char ch;
    while((ch = Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(isWhiteSpace(ch)) {
            continue;
        } else if(ch == '{') {
            return parseObject(iterator, ctx, json, dest);
        } else if(ch == '[') {
            return parseArray(iterator, ctx, json, dest);
        } else if(ch == '"') {
            JsonStringRange string;
            CHECK(parseString(iterator, &string), "Parsing string");

            TableItem* item;
            CHECK_BOOLEAN(item = JsonTable_set(json->table, dest), -1, "Setting value into the hash table");
            item->typedValue.type = JsonValueTypeString;
            item->typedValue.value.string.start = string.start;
            item->typedValue.value.string.length = string.length;

            return ParsingStatusOk;
        } else if(ch == 't') {
            return parseTrue(iterator, json, dest);
        } else if(ch == 'f') {
            return parseFalse(iterator, json, dest);
        } else if(ch == 'n') {
            return parseNull(iterator, json, dest);
        } else if((ch >= '0' && ch <= '9') || ch == '-') {
            return parseNumber(iterator, ctx, json, dest);
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

int Json_parse(Json* json) {
    LOGS_SCOPE();

    Iterator iterator = {
        .index = -1,
        .src = json->table->stringBuffer,
        .ended = 0
    };

    ParsingCtx ctx = {
        .nesting = NestedStructureNone,
    };

    JsonDestination dest = {
        .isRoot = 1,
    };

    CHECK(parseValue(&iterator, &ctx, json, &dest), "Parsing basis");

    json->parsed = 1;
    return 0;
}

JsonValue* Json_getRoot(Json* json) {
    if(!json->parsed) return NULL;

    return &json->table->buffer->typedValue;
}

int Json_asNumber(JsonValue* item, float* result) {
    if(item->type != JsonValueTypeNumber) return 0;

    if(result) *result = item->value.number;

    return 1;
}

int Json_asBoolean(JsonValue* item, int* result) {
    if(item->type != JsonValueTypeBoolean) return 0;

    if(result) *result = item->value.boolean;

    return 1;
}

int Json_asString(Json* json, JsonValue* item, char* result, size_t bufferLength, size_t* actualLength) {
    if(item->type != JsonValueTypeString) return 0;

    if(actualLength) *actualLength = item->value.string.length;

    if(!result) return 1;

    size_t limit;
    if(bufferLength >= item->value.string.length + 1) {
        limit = item->value.string.length;
    } else {
        limit = item->value.string.length - 1;
    }

    for(size_t i = 0; i < limit; i++) {
        result[i] = json->table->stringBuffer[i + item->value.string.start];
    }

    result[limit] = '\0';

    return 1;
}
int Json_asArray(JsonValue* item, JsonArray* array) {
    if(item->type != JsonValueTypeArray) return 0;
    if(array) {
        array->contextIndex = item->value.array.contextIndex;
        array->size = item->value.array.size;
    }

    return 1;
}

int Json_asObject(JsonValue* item, JsonObject* obj) {
    if(item->type != JsonValueTypeObject) return 0;
    if(obj) {
        obj->contextIndex = item->value.object.contextIndex;
        obj->size = item->value.object.size;
    }

    return 1;
}

int Json_isNull(JsonValue* item) {
    return item->type == JsonValueTypeNull;
}

JsonValue* Json_getKey(Json* json, JsonValue* item, char* key) {
    if(!json->parsed) return NULL;

    if(item->type != JsonValueTypeObject) return NULL;
    TableItem* result = JsonTable_getByKey(json->table, key, item->value.object.contextIndex);
    
    if(!result) return NULL;

    return &result->typedValue;
}

JsonValue* Json_getIndex(Json* json, JsonValue* item, size_t index) {
    if(!json->parsed) return NULL;

    if(item->type != JsonValueTypeArray) return NULL;

    TableItem* result = JsonTable_getByIndex(json->table, index, item->value.array.contextIndex);

    if(!result) return NULL;

    return &result->typedValue;
}


static void printSpacer(size_t times) {
    for(size_t i = 0; i < times; i++) {
        printf("    ");
    }
}

static void printValue(Json* json, JsonValue* val, size_t depth) {
    switch(val->type) {
        case JsonValueTypeNumber:
            printf("%f", val->value.number);
            break;

        case JsonValueTypeBoolean:
            printf("%s", val->value.boolean ? "true" : "false");
            break;

        case JsonValueTypeString: {
            printf("\"");
            printRange(json->table->stringBuffer, val->value.string.start, val->value.string.length);
            printf("\"");
            break;
        }

        case JsonValueTypeNull:
            printf("null");
            break;
        
        case JsonValueTypeArray: 
            if(val->value.array.size == 0) {
                printf("[]");
                break;
            }
            printf("[\n");
            for(size_t i = 0; i < val->value.array.size; i++) {
                printSpacer(depth + 1);
                printValue(json, Json_getIndex(json, val, i), depth + 1);
                if(i != val->value.array.size - 1) printf(",");
                printf("\n");
            }
            printSpacer(depth);
            printf("]");

            break;

        case JsonValueTypeObject: {
            if(val->value.object.size == 0) {
                printf("{}");
                break;
            }
            printf("{\n");

            size_t found = 0;
            for(size_t i = 0 ; i < json->table->maxSize; i++) {
                TableItem* current = json->table->buffer + i;
                if(!current->isBusy || current->contextIndex != val->value.object.contextIndex) continue;
                found++;
                printSpacer(depth + 1);
                printf("\"");
                printRange(json->table->stringBuffer, current->name.start, current->name.length);
                printf("\": ");
                printValue(json, &current->typedValue, depth + 1);
                if(found < val->value.object.size) {
                    printf(",");
                }
                printf("\n");
            }
            printSpacer(depth);
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

    printValue(json, root, 0);

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
        case JsonValueTypeArray:
            printf("Array\n");
            break;

        case JsonValueTypeObject:
            printf("Object\n");
            break;

        case JsonValueTypeBoolean:
            printf("Boolean\n");
            break;

        case JsonValueTypeNull:
            printf("Null\n");
            break;

        case JsonValueTypeNumber:
            printf("Null\n");
            break;

        case JsonValueTypeString:
            printf("String\n");
            break;
    }
}