#include "string.h"

#include <unittest/unittest.h>

int test_memcpy()
{
    char src[]  = "0123456789";
    char dest[] = "          ";

    void *ret = memcpy(dest, src, 3);

    tassert_eq((char) '0', dest[0]);
    tassert_eq((char) '1', dest[1]);
    tassert_eq((char) '2', dest[2]);
    tassert_eq((char) ' ', dest[3]);
    tassert_eq(dest, ret);

    return TEST_PASS;
}

int test_memset()
{
    char dest[] = "          ";

    void *ret = memset(dest, 'a', 3);

    tassert_eq((char) 'a', dest[0]);
    tassert_eq((char) 'a', dest[1]);
    tassert_eq((char) 'a', dest[2]);
    tassert_eq((char) ' ', dest[3]);
    tassert_eq(dest, ret);

    return TEST_PASS;
}

int test_strcpy()
{
    char buf[256];

    char *ret = strcpy(buf, "0123456789");
    tassert_streq("0123456789", buf);
    tassert_eq(buf, ret);

    /* strncopy should limit the number of characters written. */
    strncpy(buf, "abcdefghijklmnop", 5);
    tassert_streq("abcde56789", buf);

    /* strncopy should write nulls until count is finished. */
    strncpy(buf, "foo", 6);
    tassert_streq("foo", buf);
    tassert_eq(0, buf[3]);
    tassert_eq(0, buf[4]);
    tassert_eq(0, buf[5]);
    tassert_eq('6', buf[6]);

    return TEST_PASS;
}

int test_strcat()
{
    char  buf[256] = "[existing]";
    char *ret;

    ret = strcat(buf, "[new]");
    tassert_streq("[existing][new]", buf);
    tassert_eq(buf, ret);

    ret = strncat(buf, "12345", 3);
    tassert_streq("[existing][new]123", buf);
    tassert_eq(buf, ret);

    return TEST_PASS;
}

int test_memcmp()
{
    tassert_eq(memcmp("he\0lo", "he\0lo", 5), 0);
    tassert_lt(memcmp("hello1", "hello2", 6), 0);
    tassert_gt(memcmp("hello3", "hello2", 6), 0);
    return TEST_PASS;
}

int test_strcmp()
{
    tassert_eq(strcmp("hello", "hello"), 0);
    tassert_lt(strcmp("hello1", "hello2"), 0);
    tassert_gt(strcmp("hello3", "hello2"), 0);

    tassert_lt(strcmp("hello", "hello world"), 0);
    tassert_gt(strcmp("hello world", "hello"), 0);

    tassert_eq(strncmp("hello world", "hello", 5), 0);
    tassert_eq(strncmp("hello", "world", 0), 0);
    return TEST_PASS;
}

int test_strlen()
{
    tassert_eq(0, strlen(""));
    tassert_eq(1, strlen("s"));
    tassert_eq(5, strlen("hello"));
    tassert_eq(0, strlen(NULL));

    return TEST_PASS;
}

