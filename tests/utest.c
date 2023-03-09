#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include "utest.h"


void _fail(struct test *t, const char *fname, char *format, ...) {

    va_list args;
    
    t->failed++;
    
    printf("error: %s\n--- ", fname);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

bool run(const char *name, test_fn tests[]) {

    struct test t = {0};
    size_t i = 0;

    test_fn fn = tests[i++];
    while (fn != NULL) {
        int failed_before = t.failed;
        fn(&t);
        if (t.failed == failed_before)
            t.passed++;
        fn = tests[i++];
    }

    printf("%s: passed %d errors %d\n", name, t.passed, t.failed);

    if (t.failed > 0)
        return false;
    return true;
}

