#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils.h"

static void expect_unescape(const char *input, const char *expected) {
    char *actual = unescape(input, stderr);

    assert(actual != NULL);
    assert(strcmp(actual, expected) == 0);
    free(actual);
}

static void test_count_spaces(void) {
    assert(count_spaces("") == 0);
    assert(count_spaces("one two\tthree\n") == 3);
}

static void test_unescape(void) {
    expect_unescape("hello\\ world", "hello world");
    expect_unescape("\"two words\"", "two words");
    expect_unescape("A\\x42\\101", "ABA");
}

static void test_first_unquoted_space(void) {
    assert(first_unquoted_space("alpha beta") == 5);
    assert(first_unquoted_space("\"alpha beta\" gamma") == 12);
    assert(first_unquoted_space("alpha\\ beta gamma") == 11);

    /* Two backslashes make one literal backslash; they do not escape space. */
    assert(first_unquoted_space("alpha\\\\ beta") == 7);
}

int main(void) {
    test_count_spaces();
    test_unescape();
    test_first_unquoted_space();

    puts("utils tests passed");
    return EXIT_SUCCESS;
}
