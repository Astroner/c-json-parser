#include "hash-table.h"


#if !defined(HASH_TABLE_METHODS)
#define HASH_TABLE_METHODS

Json_internal_TableItem* Json_internal_Table_set(Json_internal_Table* table, Json_internal_Destination* dest);

Json_internal_TableItem* Json_internal_Table_get(Json_internal_Table* table, Json_internal_Destination* dest);
Json_internal_TableItem* Json_internal_Table_getByKey(Json_internal_Table* table, char* key, size_t contextIndex);
Json_internal_TableItem* Json_internal_Table_getByIndex(Json_internal_Table* table, size_t index, size_t contextIndex);

void Json_internal_Table_print(Json_internal_Table* table);

#endif // HASH_TABLE_METHODS
