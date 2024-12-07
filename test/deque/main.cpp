/**
 * @brief deque test file
 */

#include <deque>
#include <vector>
#include <iostream>
#include <format>

#include <cstdint>
#include <cstdlib>
#include <cassert>

#include <cf_deque.h>

int testDeque( uint32_t seed, CfDeque deque ) {
    srand(seed);

    std::vector<uint64_t> cDequeData;

    std::deque<uint64_t> correctDeque;

    for (size_t step = 0; step < 400000; step++) {
        uint32_t action = rand() % 4;

        if (step % 100 == 0) {
            volatile int qq = 42;
            qq = 11044;
        }
        if (step % 10 == 0) {
            volatile int qq = 42;
            qq = 11044;
        }

        switch (action) {
        case 0: {
            uint64_t number = 0
                | ((uint64_t)rand() << 32)
                | ((uint64_t)rand() << 16)
                | ((uint64_t)rand() <<  0);

            correctDeque.push_back(number);
            if (!cfDequePushBack(deque, &number)) {
                std::cout << std::format("Push back failed, step: {}\n", step);
                return 1;
            }
            break;
        }

        case 1: {
            uint64_t number = 0
                | ((uint64_t)rand() << 32)
                | ((uint64_t)rand() << 16)
                | ((uint64_t)rand() <<  0);

            correctDeque.push_front(number);
            if (!cfDequePushFront(deque, &number)) {
                std::cout << std::format("Push front failed, step: {}\n", step);
                return 1;
            }
            break;
        }

        case 2: {
            uint64_t number = 0;
            bool popped = cfDequePopBack(deque, &number);

            if (correctDeque.empty() && popped) {
                std::cout << std::format("Popped back while there is no elements in correct deque, step: {}\n", step);
                return 1;
            }

            if (!correctDeque.empty() && !popped) {
                std::cout << std::format("Not popped back while there are elements in correct deque, step: {}\n", step);
                return 1;
            }

            if (!correctDeque.empty()) {
                uint64_t correctNumber = correctDeque.back();

                if (number != correctNumber) {
                    std::cout << std::format("Invalid \"{}\" got, \"{}\" expected while popping back, step: ", number, correctNumber, step);
                    return 1;
                }

                correctDeque.pop_back();
            }

            break;
        }

        case 3: {
            uint64_t number = 0;
            bool popped = cfDequePopFront(deque, &number);

            if (correctDeque.empty() && popped) {
                std::cout << std::format("Popped front while there is no elements in correct deque, step: {}\n", step);
                return 1;
            }

            if (!correctDeque.empty() && !popped) {
                std::cout << std::format("Not popped front while there are elements in correct deque, step: {}\n", step);
                return 1;
            }

            if (!correctDeque.empty()) {
                uint64_t correctNumber = correctDeque.front();

                if (number != correctNumber) {
                    std::cout << std::format("Invalid \"{}\" got, \"{}\" expected while popping front, step: {}", number, correctNumber, step);
                    return 1;
                }

                correctDeque.pop_front();
            }
            break;
        }
        }

        size_t size = cfDequeSize(deque);
        size_t correctSize = correctDeque.size();

        if (size != correctSize) {
            std::cout << std::format("Sizes unmatched ({} expected, got {}), step: {}\n", correctSize, size, step);
            return 1;
        }


        if (size != 0) {
            cDequeData.resize(size);
            cfDequeWrite(deque, cDequeData.data());

            auto correctDequeIter = correctDeque.begin();
            for (size_t i = 0; i < cDequeData.size(); i++) {
                if (cDequeData[i] != *correctDequeIter) {
                    std::cout << std::format("Contents unmatched at element {} ({} expected, got {}), step: {}\n", i, cDequeData[i], *correctDequeIter, step);
                    return 1;
                }
                correctDequeIter++;
            }
        }
    }

    std::cout << "TEST SUCCEEDED!\n";
    return 0;
}

int main( void ) {
    CfArena arena = cfArenaCtor(CF_ARENA_CHUNK_SIZE_UNDEFINED);
    assert(arena != NULL);
    CfDeque arenaDeque = cfDequeCtor(sizeof(uint64_t), CF_DEQUE_CHUNK_SIZE_UNDEFINED, arena);

    testDeque(42, arenaDeque);

    cfDequeDtor(arenaDeque);
    cfArenaDtor(arena);

    CfDeque manualDeque = cfDequeCtor(sizeof(uint64_t), CF_DEQUE_CHUNK_SIZE_UNDEFINED, NULL);

    testDeque(42, manualDeque);

    cfDequeDtor(manualDeque);

    return 0;
} // main

// main.cpp