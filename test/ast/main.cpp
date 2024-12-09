/**
 * @brief AST test file
 */

#include <string.h>
#include <stdlib.h>

#include <cf_ast.h>

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
    char *source = readFile(_TEST_EXAMPLE_DIR"/example.cf");

    CfAstParseResult result = cfAstParse(CF_STR("example.cf"), CF_STR(source), NULL);

    if (result.status != CF_AST_PARSE_STATUS_OK)
        return 1;

    cfAstDumpJson(stdout, result.ok);

    cfAstDtor(result.ok);

    return 0;
} // main

// main.cpp
