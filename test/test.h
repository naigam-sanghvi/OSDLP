#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include "osdlp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

void
test_tm(void **state);

void
test_tc(void **state);

#endif /* TEST_TEST_H_ */
