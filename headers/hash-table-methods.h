#if !defined(HASH_TABLE_METHODS)
#define HASH_TABLE_METHODS

TableItem* JsonTable_set(JsonTable* table, JsonDestination* dest);

TableItem* JsonTable_get(JsonTable* table, JsonDestination* dest);
TableItem* JsonTable_getByKey(JsonTable* table, char* key, size_t contextIndex);
TableItem* JsonTable_getByIndex(JsonTable* table, size_t index, size_t contextIndex);

void JsonTable_print(JsonTable* table);

#endif // HASH_TABLE_METHODS
