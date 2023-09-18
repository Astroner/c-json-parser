#define JSON_IMPLEMENTATION
#include "../../../Json.h"

Json_create(json, 20)

int main() {
    Json_setSource(json, "[22]");

    Json_parse(json);

    Json_print(json, Json_getRoot(json));

    return 0;
}