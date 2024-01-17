#ifndef UNITTEST_H
#define UNITTEST_H

#include <stdarg.h>

#include <syslib/compiler_compat.h>

ATTR_PRINTFLIKE(1, 2) int tprintf(const char *fmt, ...);
int tvprintf(const char *fmt, va_list args);

typedef int testfn();

enum testresult {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
};

struct testresults {
    int passed;
    int failed;
    int skipped;
    int other;
};

int runtests(testfn **fns, struct testresults *results);

ATTR_PRINTFLIKE(4, 5)
int treportf(
        const char *file, int line, const char *func, const char *fmt, ...
);
int treportassert(
        const char *file, int line, const char *func, const char *assertion
);
int treportassert2(
        const char *file,
        int         line,
        const char *func,
        const char *assertion,
        const char *lhs_fmt,
        const char *rhs_fmt,
        ...
);

#define tfail(fmt, ...) \
    do { \
        treportf(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
        return TEST_FAIL; \
    } while (0)

#define tassertf(assertion, fmt, ...) \
    do { \
        if (assertion) { /* pass */ \
        } else { \
            treportf(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__); \
            return TEST_FAIL; \
        } \
    } while (0)

#define tassert(assertion) \
    do { \
        if (assertion) { /* pass */ \
        } else { \
            treportassert(__FILE__, __LINE__, __func__, #assertion); \
            return TEST_FAIL; \
        } \
    } while (0)

#define tassert2f(assertion, lhs_fmt, rhs_fmt, lhs, rhs) \
    do { \
        if (assertion) { /* pass */ \
        } else { \
            treportassert2( \
                    __FILE__, __LINE__, __func__, #assertion, lhs_fmt, \
                    rhs_fmt, lhs, rhs \
            ); \
            return TEST_FAIL; \
        } \
    } while (0)

#define tfmtspec(val) \
    _Generic(val, \
            unsigned char : "%#02x", \
            signed char : "%c", \
            char : "%c", \
            int : "%8d", \
            unsigned short: "%#4x", \
            char * : "%s", \
            const char * : "%s", \
            void *: "%8p", \
            double: "%f" \
    )

#define tassert2(assertion, lhs, rhs) \
    do { \
        if (assertion) { /* pass */ \
        } else { \
            treportassert2( \
                    __FILE__, __LINE__, __func__, #assertion, tfmtspec(lhs), \
                    tfmtspec(rhs), lhs, rhs \
            ); \
            return TEST_FAIL; \
        } \
    } while (0)

#define tassert_eq(lhs, rhs)    tassert2(lhs == rhs, lhs, rhs)
#define tassert_lt(lhs, rhs)    tassert2(lhs < rhs, lhs, rhs)
#define tassert_gt(lhs, rhs)    tassert2(lhs > rhs, lhs, rhs)
#define tassert_streq(lhs, rhs) tassert2(strcmp(lhs, rhs) == 0, lhs, rhs)

#endif /* UNITTEST_H */
