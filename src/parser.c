#include "parser.h"
#include "hash-table-methods.h"
#include "logs.h"
#include "utils.h"



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