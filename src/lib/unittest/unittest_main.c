#include "unittest.h"

#include <stdio.h>

#include <syslib/compiler_compat.h>

#define UNUSED(x) (void) (x) /* suppress unused warning */

extern testfn *discovered_tests[];

ATTR_WEAK int main(int argc, const char *argv[])
{
    const char *progname = argc ? argv[0] : "[unknown test file]";

    struct testresults rs = {};

    int failed = runtests(discovered_tests, &rs);

    printf("%3d passed; %3d failed; %3d skipped; %3d other; %s\n", rs.passed,
           rs.failed, rs.skipped, rs.other, progname);

    return failed;
}

