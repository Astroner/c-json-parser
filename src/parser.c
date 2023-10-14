#include "parser.h"
#include "hash-table-methods.h"
#include "logs.h"
#include "utils.h"



static Json_internal_ParsingStatus Json_internal_parseArray(
    Json_internal_Iterator* iterator, 
    Json_internal_ParsingCtx* ctx, 
    JsonValue* result
) {
    LOGS_SCOPE();
    
    Json_internal_NestedStructure prevNesting = ctx->nesting;
    ctx->nesting = Json_internal_NestedStructureArray;
    size_t arrayIndex = ++ctx->namespaceCounter;

    result->type = Json_internal_TableValueTypeArray;
    result->value.array.namespace = arrayIndex;
    result->value.array.size = 0;


    size_t arrSize = 0;
    while(1) {
        Json_internal_TableItem* element;

        CHECK_BOOLEAN((element = Json_internal_Table_setByIndex(ctx->table, arrSize, arrayIndex)), Json_internal_ParsingStatusError, "Setting table item");

        Json_internal_ParsingStatus status = Json_internal_parseValue(iterator, ctx, &element->typedValue);

        
        if(status == Json_internal_ParsingStatusOk) {

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
            return Json_internal_ParsingStatusError;
        }

        if(status == Json_internal_ParsingStatusEndedWithComma) {

            arrSize++;
            LOG_LN("Element ended with comma: parsing next element");
            continue;
        }
        if(status == Json_internal_ParsingStatusEndedWithSquare) {
            LOG_LN("Element ended with square bracket: end of array");
            arrSize++;
            break;
        }
        if(status == Json_internal_ParsingStatusCommaOnly) {
            LOG_LN("Unexpected comma");

            return Json_internal_ParsingStatusError;
        }
        if(status == Json_internal_ParsingStatusSquareOnly) {
            if(arrSize > 0) {
                LOG_LN("Unexpected square")
                return Json_internal_ParsingStatusError;
            }
            LOG_LN("Empty array");
            break;
        }
        if(status == Json_internal_ParsingStatusError) {
            LOG_LN("Failed to parse field");

            return Json_internal_ParsingStatusError;
        }
    }

    ctx->nesting = prevNesting;
    result->value.array.size = arrSize;

    return Json_internal_ParsingStatusOk;
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

            return Json_internal_ParsingStatusOk;
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
            return Json_internal_ParsingStatusError;
        } else {
            length++;
        }
    }
    
    LOG_LN("Unexpected EOI");
    return Json_internal_ParsingStatusError;
}

static Json_internal_ParsingStatus Json_internal_parseNumber(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, JsonValue* jsonResult) {
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
            return Json_internal_ParsingStatusError;
        } else if(ctx->nesting != Json_internal_NestedStructureNone && ch == ',') {
            LOG_LN("Ended by comma in nested structure");

            break;
        } else if(ctx->nesting == Json_internal_NestedStructureObject && ch == '}') {
            LOG_LN("Ended as last object field");

            break;
        } else if(ctx->nesting == Json_internal_NestedStructureArray && ch == ']') {
            LOG_LN("Ended as last array element");

            break;
        } else {
            LOG_LN("Unexpected char");
            return Json_internal_ParsingStatusError;
        }
    }

    if(iterator->current == '\0' && ctx->nesting != Json_internal_NestedStructureNone) {
        LOG_LN("Got EOI inside of nested structure");
        return Json_internal_ParsingStatusError;
    }

    if(isNegative) result *= -1;

    LOG_LN("Result: %f", result);

    jsonResult->type = Json_internal_TableValueTypeNumber;
    jsonResult->value.number = result;


    if(ctx->nesting != Json_internal_NestedStructureNone && ch == ',') {
        return Json_internal_ParsingStatusEndedWithComma;
    }

    if(ctx->nesting == Json_internal_NestedStructureObject && ch == '}') {
        return Json_internal_ParsingStatusEndedWithCurly;
    }

    if(ctx->nesting == Json_internal_NestedStructureArray && ch == ']') {
        return Json_internal_ParsingStatusEndedWithSquare;
    }

    return Json_internal_ParsingStatusOk;
}

static Json_internal_ParsingStatus Json_internal_parseTrue(
    Json_internal_Iterator* iterator, 
    JsonValue* result
) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'r') return -1;
    if(Json_internal_Iterator_next(iterator) != 'u') return -1;
    if(Json_internal_Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("true");

    result->type = Json_internal_TableValueTypeBoolean;
    result->value.boolean = 1;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseFalse(Json_internal_Iterator* iterator, JsonValue* result) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'a') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    if(Json_internal_Iterator_next(iterator) != 's') return -1;
    if(Json_internal_Iterator_next(iterator) != 'e') return -1;
    
    LOG_LN("false");

    result->type = Json_internal_TableValueTypeBoolean;
    result->value.boolean = 0;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseNull(Json_internal_Iterator* iterator, JsonValue* result) {
    LOGS_SCOPE();

    if(Json_internal_Iterator_next(iterator) != 'u') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    if(Json_internal_Iterator_next(iterator) != 'l') return -1;
    
    LOG_LN("null");

    result->type = Json_internal_TableValueTypeNull;

    return 0;
}

static Json_internal_ParsingStatus Json_internal_parseField(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, size_t objectCtx) {
    LOGS_SCOPE();

    int status = Json_internal_Iterator_skipSpaceTo(iterator, '"');

    if(status < 0) {
        if(iterator->current == ',') return Json_internal_ParsingStatusCommaOnly;
        if(iterator->current == '}') return Json_internal_ParsingStatusCurlyOnly;

        LOG_LN("Unexpected char '%c'\n", iterator->current);
        return Json_internal_ParsingStatusError; 
    }

    JsonStringRange name;

    CHECK(Json_internal_parseString(iterator, &name), "Parsing key string");

    CHECK(Json_internal_Iterator_skipSpaceTo(iterator, ':'), "Skipping chars to ':'");

    Json_internal_TableItem* field;
    CHECK_BOOLEAN((field = Json_internal_Table_setByKey(
        ctx->table, 
        iterator->src, 
        name.start, 
        name.length, 
        objectCtx
    )), Json_internal_ParsingStatusError, "Setting table item");

    CHECK_RETURN(Json_internal_parseValue(iterator, ctx, &field->typedValue), "Field value");
}

static Json_internal_ParsingStatus Json_internal_parseObject(
    Json_internal_Iterator* iterator, 
    Json_internal_ParsingCtx* ctx, 
    JsonValue* result
) {
    LOGS_SCOPE();

    Json_internal_NestedStructure prevNesting = ctx->nesting;
    ctx->nesting = Json_internal_NestedStructureObject;
    size_t objIndex = ++ctx->namespaceCounter;

    result->type = Json_internal_TableValueTypeObject;
    result->value.object.namespace = objIndex;
    result->value.object.size = 0;

    size_t size = 0;
    while(1) {
        Json_internal_ParsingStatus status = Json_internal_parseField(iterator, ctx, objIndex);

        if(status == Json_internal_ParsingStatusOk) {
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
        if(status == Json_internal_ParsingStatusEndedWithComma) {
            size++;
            LOG_LN("Field ended with comma: parsing next field");
            continue;
        }
        if(status == Json_internal_ParsingStatusEndedWithCurly) {
            LOG_LN("Field ended with curly: end of object");
            size++;
            break;
        }
        if(status == Json_internal_ParsingStatusCommaOnly) {
            LOG_LN("Unexpected comma");

            return Json_internal_ParsingStatusError;
        }
        if(status == Json_internal_ParsingStatusCurlyOnly) {
            if(size > 0) {
                LOG_LN("Unexpected curly")
                return Json_internal_ParsingStatusError;
            }
            LOG_LN("Empty object");
            break;
        }
        if(status == Json_internal_ParsingStatusError) {
            LOG_LN("Failed to parse field");

            return Json_internal_ParsingStatusError;
        }
    }

    ctx->nesting = prevNesting;
    result->value.object.size = size;

    return Json_internal_ParsingStatusOk;
}

Json_internal_ParsingStatus Json_internal_parseValue(
    Json_internal_Iterator* iterator, 
    Json_internal_ParsingCtx* ctx, 
    JsonValue* result
) {
    LOGS_SCOPE();

    char ch;
    while((ch = Json_internal_Iterator_next(iterator))) {
        LOG_LN("CHAR: '%c'", ch);
        if(Json_internal_isWhiteSpace(ch)) continue;
        else if(ch == '{') return Json_internal_parseObject(iterator, ctx, result);
        else if(ch == '[') return Json_internal_parseArray(iterator, ctx, result);
        else if(ch == '"') {
            JsonStringRange string;
            CHECK(Json_internal_parseString(iterator, &string), "Parsing string");

            result->type = Json_internal_TableValueTypeString;
            result->value.string.start = string.start;
            result->value.string.length = string.length;

            return Json_internal_ParsingStatusOk;
        } else if(ch == 't') return Json_internal_parseTrue(iterator, result);
        else if(ch == 'f') return Json_internal_parseFalse(iterator, result);
        else if(ch == 'n') return Json_internal_parseNull(iterator, result);
        else if((ch >= '0' && ch <= '9') || ch == '-') return Json_internal_parseNumber(iterator, ctx, result);
        else if(ctx->nesting == Json_internal_NestedStructureArray && ch == ']') return Json_internal_ParsingStatusSquareOnly;
        else {
            LOG_LN("Unexpected char '%c'", ch);
            return Json_internal_ParsingStatusError;
        }
    }

    LOG_LN("Unexpected EOI");
    return Json_internal_ParsingStatusError;
}