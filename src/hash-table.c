#include "hash-table.h"
#include "hash-table-methods.h"


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