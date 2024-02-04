/*
dev: barak
rev:
date:
status:
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /*getenv()*/
#include "watchdog.h"
#include <semaphore.h> /*sem_open*/

int main(int argc, char *argv[])
{
	int i = 0;
	char *str = "hello";
	int one_time_fault = (NULL == getenv(USR_ENV_NAME));

	WDStart(argc, argv);

	printf("Inside WDStart\n");
	for (i = 0; i < 100; ++i)
	{
		sleep(1);
	}

	/* if (one_time_fault)
	{
		printf("sigmantaion\n");
		str[0] = 't';
		one_time_fault = 0;
	} */

	printf("is exit succed:%d\n", WDStop() == WD_SUCCES);
	for (i = 0; i < 5; ++i)
	{
		sleep(1);
	}

	printf("After WDStop\n");

	WDStart(argc, argv);

	printf("Inside WDStart\n");
	for (i = 0; i < 20; ++i)
	{
		sleep(1);
	}
	printf("is exit succed:%d\n", WDStop() == WD_SUCCES);
	for (i = 0; i < 5; ++i)
	{
		sleep(1);
	}

	printf("After WDStop\n");

	return 0;
}
