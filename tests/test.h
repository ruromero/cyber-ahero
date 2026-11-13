#ifndef CYBER_TEST_H
#define CYBER_TEST_H

/* Minimal assert-style test helpers, no dependencies.
 *
 * One test executable = one .c file that includes this header, defines
 * static void test functions and a main() like:
 *
 *     int main(void)
 *     {
 *         RUN_TEST(test_something);
 *         return TEST_REPORT();
 *     }
 *
 * CHECK* macros record a failure and keep going; the exit code is non-zero if
 * any check failed. Include this header from a single translation unit per
 * executable (the counters are static). */

#include <stdio.h>
#include <string.h>

static int test_checks_;
static int test_failures_;

#define TEST_FAIL_(...)                                                        \
    do {                                                                       \
        test_failures_++;                                                      \
        fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);                        \
        fprintf(stderr, __VA_ARGS__);                                          \
        fputc('\n', stderr);                                                   \
    } while (0)

#define CHECK(cond)                                                            \
    do {                                                                       \
        test_checks_++;                                                        \
        if (!(cond))                                                           \
            TEST_FAIL_("CHECK(%s) failed", #cond);                             \
    } while (0)

#define CHECK_INT(actual, expected)                                            \
    do {                                                                       \
        long long a_ = (long long)(actual), e_ = (long long)(expected);        \
        test_checks_++;                                                        \
        if (a_ != e_)                                                          \
            TEST_FAIL_("%s == %lld, expected %lld", #actual, a_, e_);          \
    } while (0)

/* NULL-safe string equality; CHECK_STR(x, NULL) asserts x is NULL. */
#define CHECK_STR(actual, expected)                                            \
    do {                                                                       \
        const char *a_ = (actual), *e_ = (expected);                           \
        test_checks_++;                                                        \
        if (!((a_ == NULL && e_ == NULL) ||                                    \
              (a_ != NULL && e_ != NULL && strcmp(a_, e_) == 0)))              \
            TEST_FAIL_("%s == \"%s\", expected \"%s\"", #actual,               \
                       a_ ? a_ : "(null)", e_ ? e_ : "(null)");                \
    } while (0)

#define RUN_TEST(fn)                                                           \
    do {                                                                       \
        int before_ = test_failures_;                                          \
        fn();                                                                  \
        printf("%s %s\n", test_failures_ == before_ ? "ok  " : "FAIL", #fn);   \
    } while (0)

#define TEST_REPORT()                                                          \
    (printf("%d checks, %d failed\n", test_checks_, test_failures_),           \
     test_failures_ ? 1 : 0)

#endif
