#include <hash-table.h>

#include <string.h>
#include <stdio.h>

static unsigned long hashChars(char* str, size_t contextIndex) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long hashIndex(size_t index, size_t contextIndex) {
    unsigned long hash = 5381;
    
    unsigned char* indexBytes = (void*)&index;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + indexBytes[i];


    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];


    return hash;
}

static unsigned long hashKey(char* strBuffer, size_t nameStart, size_t nameLength, size_t contextIndex) {
    unsigned long hash = 5381;

    for(size_t i = nameStart; i < nameStart + nameLength; i++)
        hash = ((hash << 5) + hash) + strBuffer[i];

    unsigned char* ctxBytes = (void*)&contextIndex;

    for(size_t i = 0; i < sizeof(size_t); i++)
        hash = ((hash << 5) + hash) + ctxBytes[i];
    
    return hash;
}


TableItem* HashTable_set(HashTable* table, Destination* dest) {
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
        hash = hashIndex(dest->index, dest->ctx);
    } else {
        hash = hashKey(table->stringBuffer, dest->name.start, dest->name.length, dest->ctx);
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

void HashTable_print(HashTable* table) {
    for(size_t i = 0; i < table->maxSize; i++) {
        TableItem* current = table->buffer + i;
        printf("%zu) %s\n", i, current->isBusy ? "BUSY" : "FREE");
    }
}

// TableItem* HashTable_get(HashTable* table, Destination* dest) {
//     unsigned long hash = hashKey(table->stringBuffer, dest->name.start, dest->name.length, dest->ctx);

//     size_t index = hash % table->maxSize;
    
//     TableItem* current = table->buffer + index;

//     if(
//         !current->isBusy 
//         || current->contextIndex != dest->ctx 
//         || current->name.start != dest->name.start 
//         || current->name.length != dest->name.length
//     ) return NULL;
    
//     return table->buffer + index;
// }

TableItem* HashTable_getByKey(HashTable* table, char* key, size_t contextIndex) {
    unsigned long hash = hashChars(key, contextIndex);

    size_t startIndex = hash % table->maxSize;
    
    size_t index = startIndex;
    TableItem* current = table->buffer + index;

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

TableItem* HashTable_getByIndex(HashTable* table, size_t index, size_t contextIndex) {

    unsigned long hash = hashIndex(index, contextIndex);

    size_t startIndex = hash % table->maxSize;

    size_t bufferIndex = startIndex;
    
    TableItem* current = table->buffer + bufferIndex;

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