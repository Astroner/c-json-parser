#include "iterator.h"
#include "json.h"



#if !defined(PARSER_H)
#define PARSER_H

typedef enum Json_internal_ParsingStatus {
    Json_internal_ParsingStatusError = -1,
    Json_internal_ParsingStatusOk = 0,

    Json_internal_ParsingStatusEndedWithComma = 1,
    Json_internal_ParsingStatusCommaOnly = 2,

    Json_internal_ParsingStatusEndedWithCurly = 3,
    Json_internal_ParsingStatusCurlyOnly = 4,

    Json_internal_ParsingStatusEndedWithSquare = 5,
    Json_internal_ParsingStatusSquareOnly = 6,
} Json_internal_ParsingStatus;

typedef enum Json_internal_NestedStructure {
    Json_internal_NestedStructureNone,
    Json_internal_NestedStructureObject,
    Json_internal_NestedStructureArray
} Json_internal_NestedStructure;

typedef struct Json_internal_ParsingCtx {
    Json_internal_NestedStructure nesting;
    size_t namespaceCounter;
    Json_internal_Table* table;
} Json_internal_ParsingCtx;

Json_internal_ParsingStatus Json_internal_parseValue(Json_internal_Iterator* iterator, Json_internal_ParsingCtx* ctx, JsonValue* result);

#endif // PARSER_H
