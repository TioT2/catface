/**
 * @brief function building stage implementation file
 */

#include "cf_tir_builder.h"

/// @brief function builder internal local variable representation
typedef struct CfTirFunctionBuilderLocal_ {
    CfTirLocalVariableId id;            ///< id
    CfStr                name;          ///< local name
    CfTirType            type;          ///< local variable type
    bool                 isInitialized; ///< do it has value or not
} CfTirFunctionBuilderLocal;

/// @brief function builder
typedef struct CfTirFunctionBuilder_ {
    CfTirBuilder         * tirBuilder; ///< TIR builder
    CfTirBuilderFunction * function;   ///< function is currently building
    CfDeque              * locals;     ///< local variable queue
} CfTirFunctionBuilder;

/**
 * @brief local adding function
 */
static CfTirLocalVariableId cfTirFunctionBuilderAddLocal(
    CfTirFunctionBuilder *const self,
    CfStr                       name,
    CfTirType                   type,
    bool                        isInitialized
) {
    CfTirLocalVariableId id = cfDequeLength(self->locals);
    CfTirFunctionBuilderLocal local = {
        .id            = id,
        .name          = name,
        .type          = type,
        .isInitialized = isInitialized,
    };

    cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(self->locals, &local));

    return id;
} // cfTirFunctionBuilderAddLocal

/**
 * @brief get local by name
 * 
 * @param[in] self function builder
 * @param[in] name name to search
 * 
 * @return local pointer (NULL if there is no local with this name)
 */
static CfTirFunctionBuilderLocal * cfTirFunctionBuilderFindLocal(
    CfTirFunctionBuilder *const self,
    CfStr                       name
) {
    CfDequeCursor cursor;
    if (cfDequeBackCursor(self->locals, &cursor)) {
        do {
            CfTirFunctionBuilderLocal *local = (CfTirFunctionBuilderLocal *)cfDequeCursorGet(&cursor);

            if (cfStrIsSame(local->name, name))
                return local;
        } while (cfDequeCursorAdvance(&cursor, -1));
        cfDequeCursorDtor(&cursor);
    }

    return NULL;
} // cfTirFunctionBuilderAddLocal

/**
 * @brief expression building function
 * 
 * @param[in] self       function builder
 * @param[in] expression expression to build
 * 
 * @return built expression
 */
static CfTirExpression * cfTirFunctionBuilderBuildExpression(
    CfTirFunctionBuilder  *const self,
    const CfAstExpression *      expression
) {
    CfTirExpression *result = (CfTirExpression *)cfTirBuilderAllocData(
        self->tirBuilder,
        sizeof(CfTirExpression)
    );

    switch (expression->type) {
    case CF_AST_EXPRESSION_TYPE_INTEGER:
    case CF_AST_EXPRESSION_TYPE_FLOATING:
        cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
            .status = CF_TIR_BUILDING_STATUS_CANNOT_DEDUCE_LITERAL_TYPE,
            .cannotDeduceLiteralType = expression,
        });
        break;

    case CF_AST_EXPRESSION_TYPE_IDENTIFIER: {
        CfTirFunctionBuilderLocal *local = cfTirFunctionBuilderFindLocal(self, expression->identifier);

        if (local == NULL)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_UNKNOWN_VARIABLE_REFERENCED,
                .unknownVariableReferenced = expression,
            });

        *result = (CfTirExpression) {
            .type = CF_TIR_EXPRESSION_TYPE_LOCAL,
            .resultingType = local->type,
            .local = local->id,
        };
        break;
    }

    case CF_AST_EXPRESSION_TYPE_CALL: {
        // check if it is actually a function
        if (expression->call.callee->type != CF_AST_EXPRESSION_TYPE_IDENTIFIER)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_EXPRESSION_IS_NOT_CALLABLE,
                .expressionIsNotCallable = {
                    .expression = expression->call.callee,
                },
            });

        // try to find function
        CfTirBuilderFunction *function = cfTirBuilderFindFunction(
            self->tirBuilder,
            expression->call.callee->identifier
        );

        // check for function being found
        if (function == NULL)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_FUNCTION_DOES_NOT_EXIST,
                .functionDoesNotExist = expression->call.callee,
            });

        // compare argument array count
        if (function->function.prototype.inputTypeArrayLength != expression->call.argumentArrayLength)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_UNEXPECTED_ARGUMENT_NUMBER,
                .unexpectedArgumentNumber = {
                    .calledFunction = function->astFunction,
                    .call = expression,
                },
            });

        // allocate expression array
        CfTirExpression **expressionArray = (CfTirExpression **)cfTirBuilderAllocData(
            self->tirBuilder,
            sizeof(CfTirExpression *) * function->function.prototype.inputTypeArrayLength
        );

        for (size_t i = 0; i < expression->call.argumentArrayLength; i++) {
            CfTirType expectedType = function->function.prototype.inputTypeArray[i];
            CfTirExpression *argExpression = cfTirFunctionBuilderBuildExpression(
                self,
                expression->call.argumentArray[i]
            );

            if (argExpression->resultingType != expectedType)
                cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_UNEXPECTED_ARGUMENT_TYPE,
                    .unexpectedArgumentType = {
                        .calledFunction      = function->astFunction,
                        .call                = expression,
                        .parameterExpression = expression->call.argumentArray[i],
                        .parameterIndex      = i,
                        .requiredType        = cfAstTypeFromTirType(expectedType),
                        .actualType          = cfAstTypeFromTirType(argExpression->resultingType),
                    },
                });

            expressionArray[i] = argExpression;
        }

        *result = (CfTirExpression) {
            .type = CF_TIR_EXPRESSION_TYPE_CALL,
            .resultingType = function->function.prototype.outputType,
            .call = {
                .functionId       = function->id,
                .inputArray       = expressionArray,
                .inputArrayLength = function->function.prototype.inputTypeArrayLength,
            },
        };
        break;
    }

    case CF_AST_EXPRESSION_TYPE_CONVERSION: {
        CfTirType dstType = cfTirTypeFromAstType(expression->conversion.type);

        switch (expression->conversion.expr->type) {
        case CF_AST_EXPRESSION_TYPE_INTEGER: {
            // generate literal
            switch (expression->conversion.type) {
            case CF_AST_TYPE_I32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_I32,
                    .resultingType = CF_TIR_TYPE_I32,
                    .constI32 = (int32_t)expression->conversion.expr->integer,
                };
                break;

            case CF_AST_TYPE_U32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_U32,
                    .resultingType = CF_TIR_TYPE_U32,
                    .constU32 = (uint32_t)expression->conversion.expr->integer,
                };
                break;

            case CF_AST_TYPE_F32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_F32,
                    .resultingType = CF_TIR_TYPE_F32,
                    .constF32 = (float)expression->conversion.expr->floating,
                };
                break;

            case CF_AST_TYPE_VOID:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_VOID,
                    .resultingType = CF_TIR_TYPE_VOID,
                };
                break;
            }
            break;
        }

        case CF_AST_EXPRESSION_TYPE_FLOATING: {
            // generate literal
            switch (expression->conversion.type) {
            case CF_AST_TYPE_I32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_I32,
                    .resultingType = CF_TIR_TYPE_I32,
                    .constI32 = (int32_t)expression->conversion.expr->floating,
                };
                break;

            case CF_AST_TYPE_U32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_U32,
                    .resultingType = CF_TIR_TYPE_U32,
                    .constU32 = (uint32_t)expression->conversion.expr->floating,
                };
                break;

            case CF_AST_TYPE_F32:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_CONST_F32,
                    .resultingType = CF_TIR_TYPE_F32,
                    .constF32 = (float)expression->conversion.expr->floating,
                };
                break;

            case CF_AST_TYPE_VOID:
                *result = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_VOID,
                    .resultingType = CF_TIR_TYPE_VOID,
                };
                break;
            }
            break;
        }

        case CF_AST_EXPRESSION_TYPE_CALL:
        case CF_AST_EXPRESSION_TYPE_ASSIGNMENT:
        case CF_AST_EXPRESSION_TYPE_IDENTIFIER:
        case CF_AST_EXPRESSION_TYPE_CONVERSION:
        case CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR: {
            // just normal conversion expression

            CfTirExpression *expr = cfTirFunctionBuilderBuildExpression(
                self,
                expression->conversion.expr
            );

            if (expr->resultingType == CF_TIR_TYPE_VOID && dstType != CF_TIR_TYPE_VOID)
                cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                    .status         = CF_TIR_BUILDING_STATUS_IMPOSSIBLE_CAST,
                    .impossibleCast = {
                        .expression = expression,
                        .srcType    = cfAstTypeFromTirType(expr->resultingType),
                        .dstType    = cfAstTypeFromTirType(dstType),
                    },
                });

            *result = (CfTirExpression) {
                .type = CF_TIR_EXPRESSION_TYPE_CAST,
                .resultingType = dstType,
                .cast = {
                    .expression = expr,
                    .type = dstType,
                },
            };

            break;
        }
        }
        break;
    }

    case CF_AST_EXPRESSION_TYPE_ASSIGNMENT: {
        CfTirFunctionBuilderLocal *local = cfTirFunctionBuilderFindLocal(
            self,
            expression->assignment.destination
        );

        if (local == NULL)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_UNKNOWN_VARIABLE_REFERENCED,
                .unknownVariableReferenced = expression,
            });

        local->isInitialized = true;

        CfAstExpression variableDest;
        CfAstExpression valueDest;
        CfAstExpression *valueExpression;

        if (expression->assignment.op != CF_AST_ASSIGNMENT_OPERATOR_NONE) {
            CfAstBinaryOperator binaryOperator;

            switch (expression->assignment.op) {
            case CF_AST_ASSIGNMENT_OPERATOR_ADD: binaryOperator = CF_AST_BINARY_OPERATOR_ADD; break;
            case CF_AST_ASSIGNMENT_OPERATOR_SUB: binaryOperator = CF_AST_BINARY_OPERATOR_SUB; break;
            case CF_AST_ASSIGNMENT_OPERATOR_MUL: binaryOperator = CF_AST_BINARY_OPERATOR_MUL; break;
            case CF_AST_ASSIGNMENT_OPERATOR_DIV: binaryOperator = CF_AST_BINARY_OPERATOR_DIV; break;

            case CF_AST_ASSIGNMENT_OPERATOR_NONE:
                assert(false && "Unreachable case");
                break;
            }

            variableDest = (CfAstExpression) {
                .type = CF_AST_EXPRESSION_TYPE_IDENTIFIER,
                .identifier = expression->assignment.destination,
            };

            valueDest = (CfAstExpression) {
                .type = CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR,
                .binaryOperator = {
                    .op = binaryOperator,
                    .lhs = &variableDest,
                    .rhs = expression->assignment.value,
                },
            };

            valueExpression = &valueDest;
        } else {
            valueExpression = expression->assignment.value;
        }

        CfTirExpression *value = cfTirFunctionBuilderBuildExpression(self, valueExpression);

        if (value->resultingType != local->type)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_UNEXPECTED_ASSIGNMENT_VALUE_TYPE,

                .unexpectedAssignmentValueType = {
                    .assignmentExpression = expression,
                    .requiredType         = cfAstTypeFromTirType(local->type),
                    .actualType           = cfAstTypeFromTirType(value->resultingType),
                },
            });

        *result = (CfTirExpression) {
            .type          = CF_TIR_EXPRESSION_TYPE_ASSIGNMENT,
            .resultingType = CF_TIR_TYPE_VOID,
            .assignment    = {
                .destination = local->id,
                .value       = value,
            },
        };

        break;
    }

    case CF_AST_EXPRESSION_TYPE_BINARY_OPERATOR: {
        CfTirExpression *lhs = cfTirFunctionBuilderBuildExpression(self, expression->binaryOperator.lhs);
        CfTirExpression *rhs = cfTirFunctionBuilderBuildExpression(self, expression->binaryOperator.rhs);

        if (lhs->resultingType != rhs->resultingType)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_OPERAND_TYPES_UNMATCHED,
                .operandTypesUnmatched = {
                    .expression = expression,
                    .lhsType    = cfAstTypeFromTirType(lhs->resultingType),
                    .rhsType    = cfAstTypeFromTirType(rhs->resultingType),
                },
            });

        if (lhs->resultingType == CF_TIR_TYPE_VOID)
            cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                .status = CF_TIR_BUILDING_STATUS_OPERATOR_IS_NOT_DEFINED,
                .operatorIsNotDefined = {
                    .expression = expression,
                    .type       = cfAstTypeFromTirType(lhs->resultingType),
                },
            });

        CfTirBinaryOperator op = cfTirBinaryOperatorFromAstBinaryOperator(
            expression->binaryOperator.op
        );

        *result = (CfTirExpression) {
            .type          = CF_TIR_EXPRESSION_TYPE_BINARY_OPERATOR,
            .resultingType = cfTirBinaryOperatorIsComparison(op)
                ? CF_TIR_TYPE_U32
                : lhs->resultingType,
            .binaryOperator = {
                .op  = op,
                .lhs = lhs,
                .rhs = rhs,
            },
        };
        break;
    }

    }

    return result;
} // cfTirFunctionBuilderBuildExpression

/**
 * @brief 
 */
static CfTirBlock * cfTirFunctionBuilderBuildBlock(
    CfTirFunctionBuilder *const self,
    const CfAstBlock     *block
) {
    // build block local array
    CfDeque *stmtDeque = cfDequeCtor(
        sizeof(CfTirStatement),
        CF_DEQUE_CHUNK_SIZE_UNDEFINED,
        self->tirBuilder->tempArena
    );

    // local deque
    CfDeque *localDeque = cfDequeCtor(
        sizeof(CfTirLocalVariable),
        CF_DEQUE_CHUNK_SIZE_UNDEFINED,
        self->tirBuilder->tempArena
    );

    // declare block locals
    for (size_t i = 0; i < block->statementCount; i++) {
        const CfAstStatement *stmt = &block->statements[i];

        switch (stmt->type) {
        case CF_AST_STATEMENT_TYPE_EXPRESSION: {
            CfTirStatement expr = {
                .type = CF_TIR_STATEMENT_TYPE_EXPRESSION,
                .expression = cfTirFunctionBuilderBuildExpression(
                    self,
                    stmt->expression
                )
            };

            cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(stmtDeque, &expr));
            break;
        }

        case CF_AST_STATEMENT_TYPE_DECLARATION: {
            // 
            switch (stmt->declaration.type) {
            case CF_AST_DECLARATION_TYPE_FN:
                cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_LOCAL_FUNCTIONS_NOT_ALLOWED,
                    .localFunctionsNotAllowed = {
                        .function = &stmt->declaration.fn,
                        .externalFunction = self->function->astFunction,
                    },
                });
                break;

            case CF_AST_DECLARATION_TYPE_LET:
                break;
            }

            CfTirType type = cfTirTypeFromAstType(stmt->declaration.let.type);

            // add local variable to local stack
            CfTirLocalVariableId id = cfTirFunctionBuilderAddLocal(
                self,
                stmt->declaration.let.name,
                type,
                stmt->declaration.let.init != NULL
            );

            // add local variable to block internal array
            CfTirLocalVariable local = {
                .type = type,
                .name = stmt->declaration.let.name,
            };

            cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(localDeque, &local));

            // add initializer expression if there's initializer, actually
            if (stmt->declaration.let.init != NULL) {

                // build initializer expression
                CfTirExpression *value = cfTirFunctionBuilderBuildExpression(
                    self,
                    stmt->declaration.let.init
                );

                // perform value type-check
                if (local.type != value->resultingType)
                    cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                        .status = CF_TIR_BUILDING_STATUS_UNEXPECTED_INITIALIZER_TYPE,
                        .unexpectedInitializerType = {
                            .variableDeclaration = &stmt->declaration.let,
                            .expectedType        = stmt->declaration.let.type,
                            .actualType          = cfAstTypeFromTirType(value->resultingType),
                        },
                    });


                CfTirExpression *assignment = (CfTirExpression *)cfTirBuilderAllocData(
                    self->tirBuilder,
                    sizeof(CfTirExpression)
                );

                *assignment = (CfTirExpression) {
                    .type = CF_TIR_EXPRESSION_TYPE_ASSIGNMENT,
                    .resultingType = CF_TIR_TYPE_VOID,
                    .assignment = {
                        .destination = id,
                        .value = value,
                    },
                };

                CfTirStatement initStatement = {
                    .type = CF_TIR_STATEMENT_TYPE_EXPRESSION,
                    .expression = assignment
                };

                // add initializer statement
                cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(stmtDeque, &initStatement));
            }
            break;
        }

        case CF_AST_STATEMENT_TYPE_BLOCK: {
            CfTirStatement statement = {
                .type = CF_TIR_STATEMENT_BLOCK,
                .block = cfTirFunctionBuilderBuildBlock(self, stmt->block),
            };

            cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(stmtDeque, &statement));
            break;
        }

        case CF_AST_STATEMENT_TYPE_IF: {
            CfTirExpression *condition = cfTirFunctionBuilderBuildExpression(self, stmt->if_.condition);

            if (condition->resultingType != CF_TIR_TYPE_U32)
                cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_IF_CONDITION_TYPE_MUST_BE_U32,

                    .ifConditionMustBeU32 = {
                        .condition = stmt->if_.condition,
                        .actualType = cfAstTypeFromTirType(condition->resultingType),
                    },
                });

            // build expression
            CfTirStatement statement = {
                .type = CF_TIR_STATEMENT_TYPE_IF,
                .if_ = {
                    .condition = condition,
                    .blockThen = cfTirFunctionBuilderBuildBlock(self, stmt->if_.blockThen),
                    .blockElse = cfTirFunctionBuilderBuildBlock(self, stmt->if_.blockElse),
                },
            };

            cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(stmtDeque, &statement));
            break;
        }

        case CF_AST_STATEMENT_TYPE_WHILE: {
            CfTirExpression *condition = cfTirFunctionBuilderBuildExpression(
                self,
                stmt->while_.condition
            );

            // build expression
            if (condition->resultingType != CF_TIR_TYPE_U32)
                cfTirBuilderFinish(self->tirBuilder, (CfTirBuildingResult) {
                    .status = CF_TIR_BUILDING_STATUS_WHILE_CONDITION_TYPE_MUST_BE_U32,

                    .whileConditionMustBeU32 = {
                        .condition = stmt->while_.condition,
                        .actualType = cfAstTypeFromTirType(condition->resultingType),
                    },
                });

            CfTirStatement statement = {
                .type = CF_TIR_STATEMENT_TYPE_LOOP,
                .loop = {
                    .condition = condition,
                    .block     = cfTirFunctionBuilderBuildBlock(self, stmt->while_.code),
                },
            };
            cfTirBuilderAssert(self->tirBuilder, cfDequePushBack(stmtDeque, &statement));
            break;
        }
        }
    }

    // pop current level locals from local stack
    for (size_t i = 0; i < cfDequeLength(localDeque); i++)
        cfTirBuilderAssert(self->tirBuilder, cfDequePopBack(self->locals, NULL));

    // build local array
    CfTirLocalVariable *localArray = (CfTirLocalVariable *)cfTirBuilderAllocData(
        self->tirBuilder,
        sizeof(CfTirLocalVariable) * cfDequeLength(localDeque)
    );
    cfDequeWrite(localDeque, localArray);

    // build statement array
    CfTirStatement *statementArray = (CfTirStatement *)cfTirBuilderAllocData(
        self->tirBuilder,
        sizeof(CfTirStatement) * cfDequeLength(stmtDeque)
    );
    cfDequeWrite(stmtDeque, statementArray);

    // build resulting block
    CfTirBlock *result = (CfTirBlock *)cfTirBuilderAllocData(self->tirBuilder, sizeof(CfTirBlock));
    *result = (CfTirBlock) {
        .locals = localArray,
        .localCount = cfDequeLength(localDeque),
        .statements = statementArray,
        .statementCount = cfDequeLength(stmtDeque),
    };

    return result;
} // cfTirFunctionBuilderBuildBlock

/**
 * @brief single function building function
 * 
 * @param[in] self     tir builder pointer
 * @param[in] function function to build
 */
static void cfTirBuildFunction( CfTirBuilder *const self, CfTirBuilderFunction *function ) {
    if (function->astFunction->impl == NULL)
        return;

    CfTirFunctionBuilder builder = {
        .tirBuilder = self,
        .function   = function,
        .locals     = cfDequeCtor(
            sizeof(CfTirFunctionBuilderLocal),
            CF_DEQUE_CHUNK_SIZE_UNDEFINED,
            self->tempArena
        ),
    };
    cfTirBuilderAssert(self, builder.locals != NULL);

    // insert locals
    for (size_t i = 0; i < function->function.prototype.inputTypeArrayLength; i++) {
        CfTirType type = function->function.prototype.inputTypeArray[i];
        CfStr name = function->astFunction->inputs[i].name;

        cfTirFunctionBuilderAddLocal(&builder, name, type, true);
    }

    function->function.impl = cfTirFunctionBuilderBuildBlock(&builder, function->astFunction->impl);
} // cfTirBuildFunction

void cfTirBuildFunctions( CfTirBuilder *const self ) {
    CfDequeCursor fnCursor;

    if (cfDequeFrontCursor(self->functions, &fnCursor)) {
        // do-while usecase!
        do {
            cfTirBuildFunction(
                self,
                (CfTirBuilderFunction *)cfDequeCursorGet(&fnCursor)
            );
        } while (cfDequeCursorAdvance(&fnCursor, 1));

        cfDequeCursorDtor(&fnCursor);
    }
} // cfTirBuildFunction

// cf_tir_builder.c
