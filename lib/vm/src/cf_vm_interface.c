/**
 * @brief VM user interface implementation file
 */

#include <assert.h>
#include <stdlib.h>
#include <math.h>

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

bool cfExecute( const CfExecuteInfo *execInfo ) {
    assert(execInfo != NULL);

    assert(execInfo->executable != NULL);
    assert(execInfo->sandbox != NULL);
    assert(cfSandboxIsValid(execInfo->sandbox));

    // TODO: validate sandbox

    // perform minimal context setup
    CfVm vm = {
        .executable = execInfo->executable,
        .sandbox = execInfo->sandbox,
    };
    bool isOk = true;

    // setup jumpBuffer
    // note: after jumpBuffer setup it's ok to do cleanup and call panic.
    int jmp = setjmp(vm.panicJumpBuffer);
    if (jmp) {
        execInfo->sandbox->terminate(execInfo->sandbox->userContext, &vm.termInfo);
        // then go to cleanup
        goto cfExecute__cleanup;
    }

    // allocate memory
    vm.ramSize = execInfo->ramSize;
    vm.ram = (uint8_t *)calloc(vm.ramSize, 1);
    vm.callStack = cfDarrCtor(sizeof(uint8_t *));
    vm.operandStack = cfDarrCtor(sizeof(uint32_t));

    // here VM is not even initialized
    if (vm.ram == NULL || vm.callStack == NULL || vm.operandStack == NULL) {
        isOk = false;
        goto cfExecute__cleanup;
    }

    vm.instructionCounterBegin = (const uint8_t *)vm.executable->code;
    vm.instructionCounterEnd   = (const uint8_t *)vm.executable->code + vm.executable->codeLength;

    vm.instructionCounter = vm.instructionCounterBegin;

    // try to initialize sandbox
    {
        CfExecContext execContext = {
            .memory = vm.ram,
            .memorySize = vm.ramSize,
        };

        // finish if initialization failed
        // standard termination mechanism is not used, because (by specification?)
        // ANY sandbox function (terminate() too) MUST NOT be called if sandbox initialization failed.
        if (!execInfo->sandbox->initialize(execInfo->sandbox->userContext, &execContext)) {
            isOk = false;
            goto cfExecute__cleanup;
        }
    }

    // start execution
    cfVmRun(&vm);

    assert(false && "vm fatal internal error occured");

    // perform cleanup
cfExecute__cleanup:
    free(vm.ram);
    cfDarrDtor(vm.callStack);
    cfDarrDtor(vm.operandStack);
    return isOk;
} // cfExecute

// cf_vm.c
