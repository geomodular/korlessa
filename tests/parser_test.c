#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utest.h"
#include "../parser.h"

char *get_ast(struct parse_result *r);

struct test_case {
    char *source;
    char *expected;
};

void test_empty(struct test *t) {
    struct test_case c = {"", "(CRATE (EOF))"};
    
    struct parser p = new_parser();
    struct parse_result res = parse("<test>", p, c.source);

    char *actual = get_ast(&res);
    if (strcmp(c.expected, actual) != 0)
        failf(t, "expected: %s got: %s", c.expected, actual);

    free(actual);
    free_parse_result(&res);
    free_parser(&p);
}

void test_sheet(struct test *t) {
    struct test_case cases[] = {
        {"4{}", "(CRATE (SHEET l: u:1 d:4 r:1) (EOF))"},
        {"8{}", "(CRATE (SHEET l: u:1 d:8 r:1) (EOF))"},
        {"sheet1:8{}", "(CRATE (SHEET l:sheet1 u:1 d:8 r:1) (EOF))"},
        {"2as3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        {"2is3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        {"2to3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        {"sheet2:2as3{}", "(CRATE (SHEET l:sheet2 u:2 d:3 r:1) (EOF))"},
        {"4{}x1", "(CRATE (SHEET l: u:1 d:4 r:1) (EOF))"},
        {"4{}x2", "(CRATE (SHEET l: u:1 d:4 r:2) (EOF))"},
        {"4{}loop", "(CRATE (SHEET l: u:1 d:4 r:-1) (EOF))"},
        // TODO: NULL
    };

    struct parser p = new_parser();

    for (size_t i = 0; i < 10; i++) {
        struct parse_result res = parse("<test>", p, cases[i].source);
        char *actual = get_ast(&res);
        if (strcmp(cases[i].expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i].source, cases[i].expected, actual);
        free(actual);
        free_parse_result(&res);
    };

    free_parser(&p);
}

int main(int argc, char **argv) {

    static test_fn tests[] = {
        test_empty,
        test_sheet,
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
