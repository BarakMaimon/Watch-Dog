#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

#define WD_ENV_NAME "wd_env"
#define USR_ENV_NAME "usr_env"

enum WD_STATUS
{
    WD_SUCCES,
    WD_ERROR
};

int WDStart(int argc, char *argv[]);

int WDStop(void);

#endif /*__WATCHDOG_H__*/