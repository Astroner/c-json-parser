#include "hash-table.h"
#include "hash-table-methods.h"

#include <string.h>
#include <stdio.h>


static unsigned long Json_internal_hashChars(char* str, size_t namespace) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    unsigned char* ctxBytes = (void*)&namespace;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashIndex(size_t index, size_t namespace) {
    unsigned long hash = 5381;
    
    unsigned char* indexBytes = (void*)&index;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + indexBytes[i];


    unsigned char* ctxBytes = (void*)&namespace;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashKey(char* strBuffer, size_t nameStart, size_t nameLength, size_t namespace) {
    unsigned long hash = 5381;

    for(size_t i = nameStart; i < nameStart + nameLength; i++)
        hash = ((hash << 5) + hash) + strBuffer[i];

    unsigned char* ctxBytes = (void*)&namespace;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];
    
    return hash;
}


Json_internal_TableItem* Json_internal_Table_set(Json_internal_Table* table, Json_internal_Destination* dest) {
    if(dest->isRoot) {
        table->busy[0] = 1;

        table->size++;
        table->buffer->name.start = dest->name.start;
        table->buffer->name.length = dest->name.length;
        table->buffer->namespace = dest->namespace;

        return table->buffer;
    }

    if(table->size == table->maxSize) return NULL;

    unsigned long hash;
    if(dest->isIndex) {
        hash = Json_internal_hashIndex(dest->index, dest->namespace);
    } else {
        hash = Json_internal_hashKey(table->stringBuffer, dest->name.start, dest->name.length, dest->namespace);
    }


    size_t startIndex = hash % table->maxSize;

    size_t index = startIndex;
    while(table->busy[index]) {
        index++;
        if(index == table->maxSize) {
            index = 0;
        }
        if(index == startIndex) {
            return NULL;
        }
    }

    table->busy[index] = 1;

    table->buffer[index].namespace = dest->namespace;

    if(dest->isIndex) {
        table->buffer[index].index = dest->index;
        table->byIndex[index] = 1;
    } else {
        table->buffer[index].name.start = dest->name.start;
        table->buffer[index].name.length = dest->name.length;
    }
    
    table->size++;

    return table->buffer + index;
}

void Json_internal_Table_print(Json_internal_Table* table) {
    for(size_t i = 0; i < table->maxSize; i++) {
        printf("%zu) %s\n", i, table->busy[i] ? "BUSY" : "FREE");
    }
}

Json_internal_TableItem* Json_internal_Table_getByKey(Json_internal_Table* table, char* key, size_t namespace) {
    unsigned long hash = Json_internal_hashChars(key, namespace);

    size_t startIndex = hash % table->maxSize;
    
    size_t index = startIndex;
    Json_internal_TableItem* current = table->buffer + index;
    while(1) {
        if(!table->busy[index]) return NULL;

        if(
            !table->byIndex[index]
            && current->namespace == namespace 
            && strncmp(key, table->stringBuffer + current->name.start, current->name.length) == 0
        ) {
            break;
        }

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

Json_internal_TableItem* Json_internal_Table_getByIndex(Json_internal_Table* table, size_t index, size_t namespace) {

    unsigned long hash = Json_internal_hashIndex(index, namespace);

    size_t startIndex = hash % table->maxSize;

    size_t bufferIndex = startIndex;
    
    Json_internal_TableItem* current = table->buffer + bufferIndex;

    while(1) {
        if(!table->busy[bufferIndex]) return NULL;

        if(
            table->byIndex[bufferIndex] 
            && current->namespace == namespace 
            && current->index == index
        ) {
            break;
        }


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

void Json_internal_Table_reset(Json_internal_Table* table) {
    table->size = 0;
    
    memset(table->busy, 0, sizeof(int) * table->maxSize);
    memset(table->byIndex, 0, sizeof(int) * table->maxSize);
}