#include "test.h"

int
main()
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_tm),
		cmocka_unit_test(test_tc),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
