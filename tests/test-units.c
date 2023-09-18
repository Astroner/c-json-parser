#include <assert.h>
#include <stdio.h>

#define JSON_IMPLEMENTATION
#include "../Json.h"

#define it(name, func)\
    printf("It %s ... ", name);\
    func();\
    printf("DONE\n");\

void generalTest(void) {
    Json_create(json, 20);

    Json_setSource(json, "{ \"s\": 22 }");

    assert(Json_parse(json) > -1);

    // Json_internal_Table_print(json->table);

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

void testSmallArray(void) {
    Json_create(json, 20);

    Json_setSource(json, "[22]");

    assert(Json_parse(json) > -1);

    JsonValue* root;

    assert((root = Json_getRoot(json)));


    JsonValue* s;
    assert(s = Json_getIndex(json, root, 0));

    float sVal;
    assert(Json_asNumber(s, &sVal));
    assert(sVal == 22.f);
}

void testAllocate() {
    Json* json = Json_allocate(20);

    Json_setSource(json, "[22]");

    assert(Json_parse(json) >= 0);

    JsonValue* root = Json_getRoot(json);

    assert(Json_asArray(root, NULL));

    JsonValue* el = Json_getIndex(json, root, 0);

    assert(!!el);

    float val;

    assert(Json_asNumber(el, &val));

    assert(val == 22.f);

    Json_free(json);
}

void testReset(void) {
    Json_create(json, 20);

    Json_setSource(json, "[22]");

    assert(Json_parse(json) >= 0);

    JsonValue* root = Json_getRoot(json);

    assert(Json_asArray(root, NULL));

    JsonValue* el = Json_getIndex(json, root, 0);

    float elVal;
    assert(Json_asNumber(el, &elVal));
    assert(elVal == 22.f);



    Json_setSource(json, "{ \"a\": true }");

    assert(Json_parse(json) >= 0);

    root = Json_getRoot(json);
    assert(Json_asObject(root, NULL));

    JsonValue* a = Json_getKey(json, root, "a");

    int aVal;
    assert(Json_asBoolean(a, &aVal));

    assert(aVal == 1);

    Json_reset(json);
}

int main(void) {
    printf("\nTesting Json\n");
    it("generally works", generalTest);
    it("handles small arrays", testSmallArray);
    it("works with dynamic allocation", testAllocate);
    it("resets object correctly", testReset);

    printf("\n");

    return 0;
}
