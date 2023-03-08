#pragma once

#define utest(MESSAGE, BLOCK) { \
  int passed = 0; \
  int error = 0; \
  printf("... " MESSAGE); \
  BLOCK \
  if (error > 0) { \
    printf("\n"); \
  } else { \
    printf(" ... ok\n"); \
  } \
  _passed += passed; \
  _error += error; \
}

#define equal(EXPRESSION, MESSAGE) { \
  if (EXPRESSION) { \
    passed++; \
  } else { \
    error++; \
    printf("\n...... ERROR " MESSAGE); \
  } \
}

#define init(MESSAGE) printf(MESSAGE "\n"); int _passed = 0; int _error = 0;
#define print_results() printf("%d passed %d errors\n", _passed, _error);

