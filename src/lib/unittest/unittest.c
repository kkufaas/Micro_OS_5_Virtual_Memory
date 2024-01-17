#include "unittest.h"

#include <stdbool.h>
#include <stdint.h>

#include <syslib/compiler_compat.h>

ATTR_WEAK int tprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int count = tvprintf(fmt, args);
    va_end(args);
    return count;
}

int tprintloc_(const char *file, int line, const char *func)
{
    return tprintf("%s:%d: %s: ", file, line, func);
}

ATTR_PRINTFLIKE(4, 5)
int treportf(
        const char *file, int line, const char *func, const char *fmt, ...
)
{
    tprintloc_(file, line, func);
    va_list args;
    va_start(args, fmt);
    int count = tvprintf(fmt, args);
    va_end(args);
    return count;
}

int treportassert(
        const char *file, int line, const char *func, const char *assertion
)
{
    return treportf(file, line, func, "assertion `%s` failed\n", assertion);
}

int treportassert2(
        const char *file,
        int         line,
        const char *func,
        const char *assertion,
        const char *lhs_fmt,
        const char *rhs_fmt,
        ...
)
{
    int count = 0;
    count += treportassert(file, line, func, assertion);
    va_list args;
    va_start(args, rhs_fmt);

    count += tprintf("\tlhs: ");
    count += tvprintf(lhs_fmt, args);
    count += tprintf("\n");

    count += tprintf("\trhs: ");
    count += tvprintf(rhs_fmt, args);
    count += tprintf("\n");

    count += tprintf("\n");
    va_end(args);
    return count;
}

int runtests(testfn **fns, struct testresults *results)
{
    struct testresults rs = {};
    if (!fns) goto return_results;

    for (; *fns; fns++) {
        testfn *test = *fns;

        int result = test();

        switch (result) {
        case TEST_PASS: rs.passed++; break;
        case TEST_FAIL: rs.failed++; break;
        case TEST_SKIP: rs.skipped++; break;
        default: rs.other++;
        }
    }

return_results:
    if (results) *results = rs;
    return rs.failed;
}

