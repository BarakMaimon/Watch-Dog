#ifndef __WDLOGGER_H__
#define __WDLOGGER_H__

#include "dvector.h"

/*struct WDLogger
{
    char *file_name;
    d_vector_t *buffer;
};
*/

enum LOGGER_STATUS
{
    LOG_SUCCES,
    LOG_FAILED
};

typedef struct WDLogger WDLogger_t;

/*
 *    Create a logger
 *
 *    Arguments:
 *		file_name - the name of the file to write the logs
 *
 *    Return: NULL if failed otherwise pointer to the WDLogger.
 *
 *    Time complexity: O(1) best/average/worst.
 *    Space complexity: O(1) best/average/worst.
 *
 */
WDLogger_t *WDLoggerCreate(char *file_name);
/*
 *    Destroy a logger
 *
 *    Arguments:
 *		logger - pointer to the logger , cant be NULL.
 *
 *    Return: void.
 *
 *    Time complexity: O(1) best/average/worst.
 *    Space complexity: O(1) best/average/worst.
 *
 */
void WDLoggerDestroy(WDLogger_t *logger);
/*
 *    Write to Buffer the massage
 *
 *    Arguments:
 *		logger - pointer to the logger , cant be NULL.
 *
 *    Return: NULL if failed otherwise pointer to the WDLogger.
 *
 *    Time complexity: O(1) - amortized .
 *    Space complexity: O(1) best/average/worst.
 *
 */
int WDLoggerWriteBuffer(WDLogger_t *logger, char *massage);
/*
 *    Write to File all the messges in the buffer.
 *
 *    Arguments:
 *		logger - pointer to the logger , cant be NULL.
 *
 *    Return: NULL if failed otherwise pointer to the WDLogger.
 *
 *    Time complexity: O(n) - amortized .
 *    Space complexity: O(1) best/average/worst.
 *
 */
int WDLoggerWriteFile(WDLogger_t *logger);

#endif /*__WDLOGGER_H__*/

/*named semaphore between WD and Client for sync the Write File func*/
/*every process will have diffrent WDLogger but will open the same file*/