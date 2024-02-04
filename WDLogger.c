/*
dev: barak
rev:
date:
status:
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "WDLogger.h"

#define MAX_MESSAGE_LEN 60
#define START_CAPACITY 20

static int WDLoggerCleanBuffer(WDLogger_t *logger);

struct WDLogger
{
    char *file_name;
    d_vector_t *buffer;
};

WDLogger_t *WDLoggerCreate(char *file_name)
{
    WDLogger_t *logger = NULL;

    assert(NULL != file_name);

    logger = malloc(sizeof(WDLogger_t));
    if (NULL == logger)
    {
        return NULL;
    }
    logger->file_name = file_name; /*TODO: strcpy/memcpy ?*/
    logger->buffer = DVectorCreate(START_CAPACITY, MAX_MESSAGE_LEN);
    if (NULL == logger->buffer)
    {
        free(logger);
        return NULL;
    }

    return logger;
}

void WDLoggerDestroy(WDLogger_t *logger)
{
    assert(NULL != logger);

    DVectorDestroy(logger->buffer);
    free(logger);
}

int WDLoggerWriteBuffer(WDLogger_t *logger, char *massage)
{
    assert(NULL != logger);
    assert(NULL != massage);

    if (LOG_FAILED == DVectorPushBack(logger->buffer, massage))
    {
        return LOG_FAILED;
    }
    return LOG_SUCCES;
}

int WDLoggerWriteFile(WDLogger_t *logger)
{
    size_t i = 0;
    FILE *file = NULL;

    assert(NULL != logger);

    if (DVectorGetSize(logger->buffer) == 0)
    {
        return LOG_SUCCES;
    }

    file = fopen(logger->file_name, "a");
    if (NULL == file)
    {
        return LOG_FAILED;
    }

    for (i = 0; i < DVectorGetSize(logger->buffer); ++i)
    {
        fprintf(file, "%s\n", (char *)DVectorGetAccess(logger->buffer, i));
    }
    WDLoggerCleanBuffer(logger);

    fclose(file);

    return LOG_SUCCES;
}

static int WDLoggerCleanBuffer(WDLogger_t *logger)
{
    assert(NULL != logger);

    while (0 != DVectorGetSize(logger->buffer))
    {
        if (LOG_FAILED == DVectorPopBack(logger->buffer))
        {
            return LOG_FAILED;
        }
    }

    return LOG_SUCCES;
}