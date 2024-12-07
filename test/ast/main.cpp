/**
 * @brief AST test file
 */

#include <string.h>

#include <cf_ast.h>

/**
 * @brief main program function
 */
int main( void ) {
    char source[] =
        #include "gen/example.cf"
    ;

    CfAstParseResult result = cfAstParse(CF_STR("example.cf"), CF_STR(source), NULL);

    if (result.status != CF_AST_PARSE_STATUS_OK)
        return 1;

    cfAstDumpJson(stdout, result.ok);

    cfAstDtor(result.ok);

    return 0;
} // main

// main.cpp