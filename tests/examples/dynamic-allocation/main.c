#define JSON_IMPLEMENTATION
#include "../../../Json.h"

int main(void) {
    Json* json = Json_allocate(20);

    Json_setSource(json, "true");

    Json_parse(json);

    Json_print(json, Json_getRoot(json));

    Json_free(json);

    return 0;
}