#include "iterator.h"
#include "json.h"



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
