/**
 * @brief AST test file
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <cf_ast.h>
#include <cf_tir.h>

// just to shut up IntelliSence
#ifndef _TEST_EXAMPLE_DIR
#define _TEST_EXAMPLE_DIR
#endif

char * readFile( const char *path ) {
    FILE *file = fopen(path, "rt");

    if (file == NULL)
        return NULL;

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = (char *)calloc(size, 1);

    if (data == NULL) {
        fclose(file);
        return NULL;
    }

    fread(data, 1, size, file);

    fclose(file);

    return data;
} // readFile

/**
 * @brief main program function
 */
int main( void ) {
    // 'example.cf' file is processed to be C++ raw string and added to /include/gen build subdirectory
    char  * text = NULL;
    CfAst * ast  = NULL;
    CfTir * tir  = NULL;

    CfArena tempArena = cfArenaCtor(1024);
    assert(tempArena != NULL);

    {
        text = readFile(_TEST_EXAMPLE_DIR"/square_equation_solver.cf");

        if (text == NULL)
            return 1;
    }

    {
        CfAstParseResult result = cfAstParse(CF_STR("square_equation_solver.cf"), CF_STR(text), tempArena);

        if (result.status != CF_AST_PARSE_STATUS_OK)
            return 1;

        ast = result.ok;

        cfAstDumpJson(stdout, ast);
        cfArenaFree(tempArena);
    }

    {
        CfTirBuildingResult result = cfTirBuild(ast, tempArena);

        if (result.status != CF_TIR_BUILDING_STATUS_OK)
            return 1;
        cfArenaFree(tempArena);
    }

    cfArenaDtor(tempArena);
    cfTirDtor(tir);
    cfAstDtor(ast);
    free(text);

    return 0;
} // main

// main.cpp
