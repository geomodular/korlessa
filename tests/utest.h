#pragma once

#include <stdbool.h>

struct test {
    int failed;
    int passed;
};

typedef void (*test_fn)(struct test*);

void _fail(struct test *t, const char *fname, char *format, ...);
bool run(const char *name, test_fn tests[]);

#define fail(STRUCT_TEST, FORMAT) _fail(STRUCT_TEST, __func__, FORMAT)
#define failf(STRUCT_TEST, FORMAT, ...) _fail(STRUCT_TEST, __func__, FORMAT, __VA_ARGS__)

