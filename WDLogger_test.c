#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WDLogger.h"

/* // Define test macros for assertions */
#define TEST_ASSERT(condition)                                                  \
	do                                                                          \
	{                                                                           \
		if (!(condition))                                                       \
		{                                                                       \
			printf("Test failed: %s:%d: %s\n", __FILE__, __LINE__, #condition); \
			return 1;                                                           \
		}                                                                       \
	} while (0)

#define TEST_ASSERT_EQUAL(expected, actual)                                                               \
	do                                                                                                    \
	{                                                                                                     \
		if ((expected) != (actual))                                                                       \
		{                                                                                                 \
			printf("Test failed: %s:%d: Expected=%d, Actual=%d\n", __FILE__, __LINE__, expected, actual); \
			return 1;                                                                                     \
		}                                                                                                 \
	} while (0)

#define TEST_FILE_NAME "logger.txt"

/* // Test function prototypes */
int test_WDLoggerCreate();
int test_WDLoggerWriteAndWriteFile();
int test_WDLoggerDestroy();

/* // Test function setup */
int main()
{
	int result = 0;

	/* // Run the test functions */
	result |= test_WDLoggerCreate();
	result |= test_WDLoggerWriteAndWriteFile();
	result |= test_WDLoggerDestroy();

	if (result == 0)
	{
		printf("All tests passed!\n");
	}
	else
	{
		printf("Some tests failed!\n");
	}

	return result;
}

/* // Test functions */

int test_WDLoggerCreate()
{
	WDLogger_t *logger = WDLoggerCreate(TEST_FILE_NAME);
	TEST_ASSERT(logger != NULL);

	/* // Clean up */
	WDLoggerDestroy(logger);
	return 0;
}

int test_WDLoggerWriteAndWriteFile()
{
	/* // Test writing messages to the buffer */
	size_t i = 0;
	const char *messages[] = {
		"Message 1",
		"This is message 2",
		"Another message here",
		"Test message"};

	WDLogger_t *logger = WDLoggerCreate(TEST_FILE_NAME);
	TEST_ASSERT(logger != NULL);

	for (i = 0; i < sizeof(messages) / sizeof(messages[0]); ++i)
	{
		TEST_ASSERT_EQUAL(LOG_SUCCES, WDLoggerWriteBuffer(logger, (char *)messages[i]));
	}

	/* // Test writing buffer to the file */
	TEST_ASSERT_EQUAL(LOG_SUCCES, WDLoggerWriteFile(logger));

	/* // Clean up */
	WDLoggerDestroy(logger);
	return 0;
}

int test_WDLoggerDestroy()
{
	WDLogger_t *logger = WDLoggerCreate(TEST_FILE_NAME);
	TEST_ASSERT(logger != NULL);

	/* // Clean up */
	WDLoggerDestroy(logger);
	return 0;
}
