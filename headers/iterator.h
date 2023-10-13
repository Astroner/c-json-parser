#include <stddef.h>


#if !defined(ITERATOR_H)
#define ITERATOR_H

typedef struct Json_internal_Iterator {
    char* src;
    char current;
    size_t index;
    int finished;
} Json_internal_Iterator;

char Json_internal_Iterator_next(Json_internal_Iterator* iterator);
int Json_internal_Iterator_skipSpaceTo(Json_internal_Iterator* iterator, char stopper);
int Json_internal_Iterator_nextCharIs(Json_internal_Iterator* iterator, char stopper);

#endif // ITERATOR_H
