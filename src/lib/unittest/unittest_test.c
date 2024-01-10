#include "unittest.h"

#include <stddef.h>
#include <string.h>

static int sample_test_pass() { return TEST_PASS; }
static int sample_test_fail() { return TEST_FAIL; }
static int sample_test_skip() { return TEST_SKIP; }
static int sample_test_other() { return 10; }

int test_runtests()
{
    testfn *fns[] = {
            sample_test_pass, sample_test_fail, sample_test_skip,
            sample_test_other, NULL};
    struct testresults rs = {};

    int failed = runtests(fns, &rs);

    if (failed != 1) {
        tfail("expected runtests to return 1, got: %d\n", failed);
    }

    if (rs.passed != 1 || rs.failed != 1 || rs.skipped != 1 || rs.other != 1) {
        tfail("expected pass/fail/skip/other == 1/1/1/1, got: %d/%d/%d/%d\n",
              rs.passed, rs.failed, rs.skipped, rs.other);
    }

    return TEST_PASS;
}

int test_asserteq()
{
    tassert_eq(12345, 12345);                   // int
    tassert_eq((void *) 12345, (void *) 12345); // pointer
    tassert_eq(1.0, 1.0);                       // float

    static const char *s1 = "hello";
    static const char *s2 = "hello";
    tassert_streq(s1, s2); // strings
    return TEST_PASS;
}

