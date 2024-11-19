#include <cf_list.h>

void dumpFn( FILE *out, const void *data ) {
    fprintf(out, "%ld", *(uint64_t *)data);
}

int main( void ) {
    CfList list = cfListCtor(sizeof(uint64_t), 0);
    uint64_t temp = 0;
    temp = 3; cfListPushBack(&list, &temp);
    temp = 1; cfListPushBack(&list, &temp);
    temp = 2; cfListPushBack(&list, &temp);
    temp = 4; cfListPushBack(&list, &temp);
    temp = 5; cfListPushBack(&list, &temp);

    CfListIter iter = cfListIterStart(list);

    cfListIterNext(&iter);
    cfListIterNext(&iter);
    temp = 11044; cfListIterInsertAfter(&iter, &temp);

    list = cfListIterFinish(&iter);

    cfListPrintDot(stdout, list, dumpFn);

    cfListDtor(list);

    return 0;
} // main

// main.cpp
