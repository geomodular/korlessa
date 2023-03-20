#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utest.h"
#include "../parser.h"

struct test_case {
    char *source;
    char *expected;
};

typedef struct test_case tc;

char *get_ast(struct parse_result *r);

void test_empty(struct test *t) {
    tc c = {"", "(CRATE (EOF))"};

    struct parser p = new_parser();
    struct parse_result res = parse("<test>", p, c.source);

    char *actual = get_ast(&res);
    if (strcmp(c.expected, actual) != 0)
        failf(t, "expected: %s got: %s", c.expected, actual);

    free(actual);
    free_parse_result(&res);
    free_parser(&p);
}

void test_comment(struct test *t) {
    tc *cases[] = {
        &(tc) {"// comment", "(CRATE (EOF))"},
        &(tc) {"4{// comment\n}", "(CRATE (SHEET l: u:1 d:4 r:1) (EOF))"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);
        char *actual = get_ast(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);
        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

void test_crate(struct test *t) {
    tc *cases[] = {
        &(tc) {"4{} 8{}", "(CRATE (SHEET l: u:1 d:4 r:1) (SHEET l: u:1 d:8 r:1) (EOF))"},
        &(tc) {"{ref1} {ref2}", "(CRATE (REFERENCE l:ref1 r:1) (REFERENCE l:ref2 r:1) (EOF))"},
        &(tc) {"120bpm 160bpm", "(CRATE (BPM v:120) (BPM v:160) (EOF))"},
        &(tc) {"CC0:0 CC90:127", "(CRATE (CC p:0 v:0) (CC p:90 v:127) (EOF))"},
        &(tc) {"pgm0 pgm1", "(CRATE (PGM v:0) (PGM v:1) (EOF))"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);
        char *actual = get_ast(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);
        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

void test_sheet(struct test *t) {
    tc *cases[] = {
        &(tc) {"4{}", "(CRATE (SHEET l: u:1 d:4 r:1) (EOF))"},
        &(tc) {"8{}", "(CRATE (SHEET l: u:1 d:8 r:1) (EOF))"},
        &(tc) {"sheet1:8{}", "(CRATE (SHEET l:sheet1 u:1 d:8 r:1) (EOF))"},
        &(tc) {"2as3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        &(tc) {"2is3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        &(tc) {"2to3{}", "(CRATE (SHEET l: u:2 d:3 r:1) (EOF))"},
        &(tc) {"sheet2:2as3{}", "(CRATE (SHEET l:sheet2 u:2 d:3 r:1) (EOF))"},
        &(tc) {"4{}x1", "(CRATE (SHEET l: u:1 d:4 r:1) (EOF))"},
        &(tc) {"4{}x2", "(CRATE (SHEET l: u:1 d:4 r:2) (EOF))"},
        &(tc) {"4{}loop", "(CRATE (SHEET l: u:1 d:4 r:-1) (EOF))"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);
        char *actual = get_ast(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);
        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

void test_note(struct test *t) {
    tc *cases[] = {
        &(tc) {"4{c}", "(CRATE (SHEET l: u:1 d:4 r:1 (NOTE ch:-1 n:c a: o:-1 v:-1)) (EOF))"},
        &(tc) {"4{c3}", "(CRATE (SHEET l: u:1 d:4 r:1 (NOTE ch:-1 n:c a: o:3 v:-1)) (EOF))"},
        &(tc) {"4{ch1:c}", "(CRATE (SHEET l: u:1 d:4 r:1 (NOTE ch:1 n:c a: o:-1 v:-1)) (EOF))"},
        &(tc) {"4{c!3}", "(CRATE (SHEET l: u:1 d:4 r:1 (NOTE ch:-1 n:c a: o:-1 v:3)) (EOF))"},
        &(tc) {"4{c#}", "(CRATE (SHEET l: u:1 d:4 r:1 (NOTE ch:-1 n:c a:# o:-1 v:-1)) (EOF))"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);
        char *actual = get_ast(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);
        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

int main(int argc, char **argv) {

    static test_fn tests[] = {
        test_empty,
        test_comment,
        test_crate,
        test_sheet,
        test_note,
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
