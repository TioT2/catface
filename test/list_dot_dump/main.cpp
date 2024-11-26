#include <cassert>

#include <cf_list.h>

void dumpFn( FILE *out, const void *data ) {
    fprintf(out, "%ld", *(uint64_t *)data);
}

int main( void ) {
    CfList list = cfListCtor(sizeof(uint64_t), 0);

    for (uint32_t i = 0; i < 10; i++) {
        uint64_t temp = i;

        CfListStatus status = cfListPushBack(&list, &temp);
        assert(status == CF_LIST_STATUS_OK);
    }

    CfListIter iter = cfListIterStart(list);

    for (uint32_t i = 0; i < 5; i++)
        cfListIterNext(&iter);

    CfListStatus status = cfListIterRemoveAt(&iter, NULL);
    assert(status == CF_LIST_STATUS_OK);

    list = cfListIterFinish(&iter);

    cfDbgListPrintDot(stdout, list, dumpFn);

    cfListDtor(list);

    return 0;
} // main

// main.cpp
