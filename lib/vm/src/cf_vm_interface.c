/**
 * @brief VM interface implementation file
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include <cf_stack.h>
#include "cf_vm_internal.h"

/**
 * @brief sandbox validness checking function
 * 
 * @param[in] sandbox sandbox to check validness of (is not required to be valid)
 * 
 * @return true if all is ok with sandbox, false otherwise
 */
static bool cfSandboxIsValid( const CfSandbox *sandbox ) {
    return true
        && sandbox != NULL
        && sandbox->initialize != NULL
        && sandbox->terminate != NULL

        && sandbox->getExecutionTime != NULL
        && sandbox->setVideoMode != NULL
        && sandbox->refreshScreen != NULL

        && sandbox->readFloat64 != NULL
        && sandbox->writeFloat64 != NULL
    ;
} // cfSandboxIsValid

/**
 * @brief executable execution function
 * 
 * @param[in] executable  executable to execute
 * @param[in] sandbox execution environment
 * 
 * @return true if execution started, false if not
 */
bool cfExecute( const CfExecutable *executable, const CfSandbox *sandbox ) {
    assert(executable != NULL);
    assert(sandbox != NULL);

    assert(cfSandboxIsValid(sandbox));

    // TODO: validate sandbox

    // perform minimal context setup
    CfVm vm = {
        .executable = executable,
        .sandbox = sandbox,
    };
    bool isOk = true;

    // setup jumpBuffer
    // note: after jumpBuffer setup it's ok to do cleanup and call panic.
    int jmp = setjmp(vm.panicJumpBuffer);
    if (jmp) {
        sandbox->terminate(sandbox->userContext, &vm.termInfo);
        // then go to cleanup
        goto cfExecute__cleanup;
    }

    // allocate memory
    vm.memorySize = 1 << 20;
    vm.memory = (uint8_t *)calloc(vm.memorySize, 1);
    vm.callStack = cfStackCtor(sizeof(uint8_t *));
    vm.operandStack = cfStackCtor(sizeof(uint32_t));

    // here VM is not even initialized
    if (vm.memory == NULL || vm.callStack == NULL || vm.operandStack == NULL) {
        isOk = false;
        goto cfExecute__cleanup;
    }

    vm.instructionCounterBegin = (const uint8_t *)vm.executable->code;
    vm.instructionCounterEnd   = (const uint8_t *)vm.executable->code + vm.executable->codeLength;

    vm.instructionCounter = vm.instructionCounterBegin;

    // try to initialize sandbox
    {
        CfExecContext execContext = {
            .memory = vm.memory,
            .memorySize = vm.memorySize,
        };

        // finish if initialization failed
        // standard termination mechanism is not used, because (by specification?)
        // ANY sandbox function (terminate() too) MUST NOT be called if sandbox initialization failed.
        if (!sandbox->initialize(sandbox->userContext, &execContext)) {
            isOk = false;
            goto cfExecute__cleanup;
        }
    }

    // start execution
    cfVmRun(&vm);

    // perform cleanup
cfExecute__cleanup:
    free(vm.memory);
    cfStackDtor(vm.callStack);
    cfStackDtor(vm.operandStack);
    return isOk;
} // cfExecute

// cf_vm.c
