#include <stddef.h>

#if !defined(UTILS_H)
#define UTILS_H

#define Json_internal_isWhiteSpace(name) ((name) == '\n' || (name) == '\r' || (name) == ' ')

void Json_internal_printRange(char* str, size_t start, size_t length);


#endif // UTILS_H
