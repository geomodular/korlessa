#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utest.h"
#include "../parser.h"

char *get_ast(struct parse_result *r);

void test_new_and_free(struct test *t) {
    struct parser p = new_parser();
    free_parser(&p);
}

void test_empty(struct test *t) {
    struct parser p = new_parser();
    const char *in = "";
    struct parse_result res = parse("<test>", p, in);

    char *expected = "(CRATE (EOF))\n";
    char *actual = get_ast(&res);
    if (strcmp(expected, actual) != 0)
        failf(t, "expected: %sgot: %s", expected, actual);

    free(actual);
    free_parse_result(&res);
    free_parser(&p);
}

int main(int argc, char **argv) {

    static test_fn tests[] = {
        test_new_and_free,
        test_empty,
        NULL,
    };

    if (run("Parser", tests))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

char *get_ast(struct parse_result *r) {
    char *buffer = NULL;
    size_t size = 0;
    FILE* f = open_memstream(&buffer, &size);
    print_ast(r->n, f);
    fclose(f);
    return buffer;
}
