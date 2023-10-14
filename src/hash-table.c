#include "hash-table.h"
#include "hash-table-methods.h"

#include <string.h>
#include <stdio.h>


static unsigned long Json_internal_hashChars(char* str, size_t namespaceID) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    unsigned char* ctxBytes = (void*)&namespaceID;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashIndex(size_t index, size_t namespaceID) {
    unsigned long hash = 5381;
    
    unsigned char* indexBytes = (void*)&index;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + indexBytes[i];


    unsigned char* ctxBytes = (void*)&namespaceID;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long Json_internal_hashKey(char* strBuffer, size_t nameStart, size_t nameLength, size_t namespaceID) {
    unsigned long hash = 5381;

    for(size_t i = nameStart; i < nameStart + nameLength; i++)
        hash = ((hash << 5) + hash) + strBuffer[i];

    unsigned char* ctxBytes = (void*)&namespaceID;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];
    
    return hash;
}

Json_internal_TableItem* Json_internal_Table_setRoot(Json_internal_Table* table) {
    table->busy[0] = 1;

    table->size++;

    return table->buffer;
}

static Json_internal_TableItem* Json_internal_Table_setByHash(Json_internal_Table* table, unsigned long hash, size_t namespaceID, int byIndex) {
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
    if(byIndex) {
        table->byIndex[index] = 1;
    }

    table->buffer[index].namespaceID = namespaceID;
    table->buffer[index].hash = hash;

    table->size++;

    return table->buffer + index;
}

Json_internal_TableItem* Json_internal_Table_setByKey(Json_internal_Table* table, char* stringBuffer, size_t start, size_t length, size_t namespaceID) {
    if(table->size == table->maxSize) return NULL;

    unsigned long hash = Json_internal_hashKey(stringBuffer, start, length, namespaceID);

    Json_internal_TableItem* item = Json_internal_Table_setByHash(table, hash, namespaceID, 0);

    if(!item) return NULL;

    item->name.start = start;
    item->name.length = length;

    return item;
}

Json_internal_TableItem* Json_internal_Table_setByIndex(Json_internal_Table* table, size_t index, size_t namespaceID) {
    if(table->size == table->maxSize) return NULL;

    unsigned long hash = Json_internal_hashIndex(index, namespaceID);

    Json_internal_TableItem* item = Json_internal_Table_setByHash(table, hash, namespaceID, 1);

    if(!item) return NULL;

    item->arrayIndex = index;

    return item;
}

void Json_internal_Table_print(Json_internal_Table* table) {
    for(size_t i = 0; i < table->maxSize; i++) {
        printf("%zu) %s\n", i, table->busy[i] ? "BUSY" : "FREE");
    }
}

Json_internal_TableItem* Json_internal_Table_getByKey(Json_internal_Table* table, char* key, size_t namespaceID) {
    unsigned long hash = Json_internal_hashChars(key, namespaceID);

    size_t startIndex = hash % table->maxSize;
    
    size_t index = startIndex;
    Json_internal_TableItem* current = table->buffer + index;
    while(1) {
        if(!table->busy[index]) return NULL;

        if(
            !table->byIndex[index]
            && current->namespaceID == namespaceID 
            && current->hash == hash
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

Json_internal_TableItem* Json_internal_Table_getByIndex(Json_internal_Table* table, size_t index, size_t namespaceID) {

    unsigned long hash = Json_internal_hashIndex(index, namespaceID);

    size_t startIndex = hash % table->maxSize;

    size_t bufferIndex = startIndex;
    
    Json_internal_TableItem* current = table->buffer + bufferIndex;

    while(1) {
        if(!table->busy[bufferIndex]) return NULL;

        if(
            table->byIndex[bufferIndex] 
            && current->namespaceID == namespaceID 
            && current->arrayIndex == index
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