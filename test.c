#define JSON_IMPLEMENTATION
#include "Json.h"
#include <assert.h>
#include <stdio.h>

#define it(name, func)\
    printf("It %s ... ", name);\
    func();\
    printf("DONE\n");\

void generalTest() {
    Json_createStatic(json, "{ \"s\": 22 }", 20);

    assert(Json_parse(json) > -1);

    JsonValue* root;

    assert((root = Json_getRoot(json)));

    size_t size;
    assert(Json_asObject(root, &size));
    assert(size == 1);

    JsonValue* s;
    assert(s = Json_getKey(json, root, "s"));

    float sVal;
    assert(Json_asNumber(s, &sVal));
    assert(sVal == 22.f);
}

int main(void) {
    printf("\nTesting Json\n");
    it("generally works", generalTest);

    printf("\n");

    return 0;
}
