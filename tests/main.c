// SPDX-License-Identifier: MIT
#include <stdlib.h>		// srand
#include <stdio.h>
#include <time.h>		// time
#include "testcase.h"


//================================================================================================================
int
main(
	int		argc,
	char**	argv)
{
	const testcase_t*	testcase;
	(void)argc;
	(void)argv;

	srand((unsigned int)time(0));
	for (testcase=testcases; testcase->name!=NULL; ++testcase)
	{
		int r = 0;
		if (testcase->instruction2>=0)
		{
			printf("%04x %04x = ", testcase->instruction, testcase->instruction2);
		}
		else
		{
			printf("%04x = ", testcase->instruction);
		}
		printf("%s\n", testcase->name);
		r = testcase_run(testcase);
		if (r!=0)
		{
			return 1;
		}
	}
	return 0;
}