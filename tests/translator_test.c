#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utest.h"
#include "../list.h"
#include "../translator.h"

struct test_case {
    char *source;
    char *expected;
};

typedef struct test_case tc;

char *get_events(struct parse_result *r);

void test_loop(struct test *t) {
    tc *cases[] = {
        &(tc) {"8{.}loop", "(USR0 t:48)"},
        &(tc) {"8{c}loop", "(NOTE L-START-END t:0 ch:0 d:44 n:60 v:127)"},
        &(tc) {"8{c d}loop", "(NOTE L-START t:0 ch:0 d:44 n:60 v:127) (NOTE L-END t:48 ch:0 d:44 n:62 v:127)"},
        &(tc) {"8{c 2{d e}}loop", "(NOTE L-START t:0 ch:0 d:44 n:60 v:127) (NOTE t:48 ch:0 d:20 n:62 v:127) (NOTE L-END t:72 ch:0 d:20 n:64 v:127)"},
        &(tc) {"8{2{c d} 2{e f}}loop", "(NOTE L-START t:0 ch:0 d:20 n:60 v:127) (NOTE t:24 ch:0 d:20 n:62 v:127) (NOTE t:48 ch:0 d:20 n:64 v:127) (NOTE L-END t:72 ch:0 d:20 n:65 v:127)"},
        &(tc) {"8{c}loop 4{d}", "(NOTE L-START-END t:0 ch:0 d:44 n:60 v:127)"},
        &(tc) {"8{c .}loop", "(NOTE L-START-END t:0 ch:0 d:44 n:60 v:127)"},
        &(tc) {"8{c +1}loop", "(NOTE L-START t:0 ch:0 d:44 n:60 v:127) (NOTE L-END t:48 ch:0 d:44 n:61 v:127)"},
        &(tc) {"8{c +1 .}loop", "(NOTE L-START t:0 ch:0 d:44 n:60 v:127) (NOTE L-END t:48 ch:0 d:44 n:61 v:127)"},
        &(tc) {"8{. c .}loop", "(NOTE L-START-END t:48 ch:0 d:44 n:60 v:127)"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);

        char *actual = get_events(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);

        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

void test_interval_and_tie(struct test *t) {
    tc *cases[] = {
        &(tc) {"8{c +1}", "(NOTE t:0 ch:0 d:44 n:60 v:127) (NOTE t:48 ch:0 d:44 n:61 v:127) (USR0 t:96)"},
        &(tc) {"8{c +1 -}", "(NOTE t:0 ch:0 d:44 n:60 v:127) (NOTE t:48 ch:0 d:92 n:61 v:127) (USR0 t:144)"},
        &(tc) {"8{c +1 .}", "(NOTE t:0 ch:0 d:44 n:60 v:127) (NOTE t:48 ch:0 d:44 n:61 v:127) (USR0 t:144)"},
        &(tc) {"8{c . +1}", "(NOTE t:0 ch:0 d:44 n:60 v:127) (NOTE t:96 ch:0 d:44 n:61 v:127) (USR0 t:144)"},
        &(tc) {"8{c -}", "(NOTE t:0 ch:0 d:92 n:60 v:127) (USR0 t:96)"},
        &(tc) {"8{c . -}", "(NOTE t:0 ch:0 d:44 n:60 v:127) (USR0 t:144)"},
        NULL,
    };

    struct parser p = new_parser();

    for (size_t i = 0; cases[i] != NULL; i++) {
        struct parse_result res = parse("<test>", p, cases[i]->source);

        char *actual = get_events(&res);
        if (strcmp(cases[i]->expected, actual) != 0)
            failf(t, "  source: %s\n    expected: %s\n         got: %s", cases[i]->source, cases[i]->expected, actual);

        free(actual);
        free_parse_result(&res);
    }

    free_parser(&p);
}

int main(int argc, char **argv) {

    static test_fn tests[] = {
        test_loop,
        test_interval_and_tie,
        NULL,
    };

    if (run("Translator", tests))
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

char *get_events(struct parse_result *r) {
    char *buffer = NULL;
    size_t size = 0;

    struct event_list *list = translate(*r->n);

    FILE* f = open_memstream(&buffer, &size);
    print_events(list, f);
    fclose(f);

    list_apply(list, free);
    return buffer;
}
