#include "iterator.h"
#include "logs.h"
#include "utils.h"


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