#include "hash-table.h"


#if !defined(HASH_TABLE_METHODS)
#define HASH_TABLE_METHODS

Json_internal_TableItem* Json_internal_Table_setRoot(Json_internal_Table* table);
Json_internal_TableItem* Json_internal_Table_setByKey(Json_internal_Table* table, char* stringBuffer, size_t start, size_t length, size_t namespaceID);
Json_internal_TableItem* Json_internal_Table_setByIndex(Json_internal_Table* table, size_t index, size_t namespaceID);

Json_internal_TableItem* Json_internal_Table_getByKey(Json_internal_Table* table, char* key, size_t namespaceID);
Json_internal_TableItem* Json_internal_Table_getByIndex(Json_internal_Table* table, size_t index, size_t namespaceID);


void Json_internal_Table_reset(Json_internal_Table* table);

void Json_internal_Table_print(Json_internal_Table* table);

#endif // HASH_TABLE_METHODS
