#include "utils.h"


#include <stdio.h>

void Json_internal_printRange(char* str, size_t start, size_t length) {
    for(size_t i = start; i < start + length; i++) {
        printf("%c", str[i]);
    }
}