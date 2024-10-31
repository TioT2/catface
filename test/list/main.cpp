#include <cstdlib>
#include <deque>

#include <cf_list.h>

uint64_t rand64( void ) {
    return 0
        | ((uint64_t)rand() << 48)
        | ((uint64_t)rand() << 32)
        | ((uint64_t)rand() << 16)
        | ((uint64_t)rand() <<  0)
    ;
}

void dumpUint64( FILE *out, void *u64 ) {
    fprintf(out, "%lu", *(uint64_t *)u64);
}

int main( void ) {
    CfList list = cfListCtor(sizeof(uint64_t), 0);
    std::deque<uint64_t> controlList;

    for (size_t i = 0; i < 10000; i++) {
        uint32_t operation = rand() % 4;

        if (i % 5 == 0)
            volatile int qq = 42;

        switch (operation) {
        case 0: {
            uint64_t value = rand64();

            if (cfListPushBack(&list, &value) != CF_LIST_STATUS_OK) {
                printf("PUSH_BACK failed\n");
                return 1;
            }
            controlList.push_back(value);

            break;
        }

        case 1: {
            uint64_t value = rand64();

            if (cfListPushFront(&list, &value) != CF_LIST_STATUS_OK) {
                printf("PUSH_FRONT failed\n");
                return 1;
            }
            controlList.push_front(value);

            break;
        }

        case 2: {
            uint64_t listDst;

            CfListStatus status = cfListPopFront(&list, &listDst);

            switch (status) {
            case CF_LIST_STATUS_INTERNAL_ERROR: {
                printf("POP_FRONT failed\n");
                return 1;

                break;
            }

            case CF_LIST_STATUS_NO_ELEMENTS: {
                if (!controlList.empty()) {
                    printf("List empty meanwhile control list not.\n");
                    return 1;
                }
                break;
            }

            case CF_LIST_STATUS_OK: {
                if (controlList.empty()) {
                    printf("Pop succeeded meanwhile conrol list empty.\n");
                    return 1;
                }
                uint64_t controlDst = controlList.front();
                controlList.pop_front();

                if (controlDst != listDst) {
                    printf("popped value isn't equal to control one\n");
                    return 1;
                }

                break;
            }
            }

            break;
        }

        case 3: {
            uint64_t listDst;

            CfListStatus status = cfListPopBack(&list, &listDst);

            switch (status) {
            case CF_LIST_STATUS_INTERNAL_ERROR: {
                printf("POP_BACK failed\n");
                return 1;

                break;
            }

            case CF_LIST_STATUS_NO_ELEMENTS: {
                if (!controlList.empty()) {
                    printf("List empty meanwhile control list not.\n");
                    return 1;
                }
                break;
            }

            case CF_LIST_STATUS_OK: {
                if (controlList.empty()) {
                    printf("Pop succeeded meanwhile conrol list empty.\n");
                    return 1;
                }
                uint64_t controlDst = controlList.back();
                controlList.pop_back();

                if (controlDst != listDst) {
                    printf("popped value isn't equal to control one\n");
                    return 1;
                }

                break;
            }
            }

            break;
        }
        }

        cfListDump(stdout, list, dumpUint64);

        if (!cfListDbgCheckPrevNext(list)) {
            printf("ERROR: PN Check failed.\n");
            return 1;
        }

        printf("\n");

        // compare lists
        CfListIterator iter = cfListIter(list);
        uint64_t *listValue = NULL;

        auto controlIter = controlList.begin();
        const auto controlEnd = controlList.end();

        while ((listValue = (uint64_t *)cfListIterNext(&iter)) != NULL) {
            uint64_t value = *listValue;
            if (controlIter == controlEnd) {
                printf("List iterators unmatched.");
                return 1;
            }

            uint64_t controlValue = *controlIter++;

            if (value != controlValue) {
                printf("List iterator values unmatched.");
                return 1;
            }
        }

        if (controlIter != controlEnd) {
            printf("List ends unmatched");
            return 1;
        }
    }

    return 0;
} // gremlinTest

