#define _XOPEN_SOURCE 700

/* compile line : gd - I../ ds / include / watchdog.c../ ds / src / scheduler.c
../ ds / src / priority_queue.c../ ds / src / sorted_list.c
../ ds / src / dlist.c../ ds / src / uid.c ../ ds / src / task.c
 */
#include <stdlib.h> /*getenv()*/
#include <unistd.h> /*getppid*/
#include <pthread.h>
#include <sys/wait.h>  /*waitpid()*/
#include <signal.h>    /*kill()*/
#include <semaphore.h> /*sem_open*/
#include <fcntl.h>     /* O_CREAT */
#include <assert.h>    /*assert*/
#include <stdio.h>

#include "scheduler.h"
#include "watchdog.h"
#include "WDLogger.h"

/*=================================== macros =================================*/
#define SEM_WRITE "sem_write"
#define SEM_LOAD "sem_load"
#define SEM_PERMS (0644)
#define PID_ARRAY_SIZE 5
#define INT_TO_CHAR_CONVERT_NUMBER 48
#define WD_EXEC_NAME "wd_exec.out"
#define MISS_FACTOR 5
#define LOG_FILE_NAME "logger.txt"

/*=================================== enums =================================*/
enum SchedTask
{
    WD_SEND_SIGNAL,
    WD_CHECK_COUNTER,
    WD_CHECK_FLAG,
    NUM_OF_TASKS
};

enum WD_FLAGS
{
    WD_DOWN,
    WD_UP
};

/*=================================== global variable =================================*/

static ilrd_uid_t uid[NUM_OF_TASKS] = {0};
static pid_t global_pid = 0;
static int global_counter = 0;
static int stop_flag = WD_DOWN;
static int kill_WD = WD_DOWN;
static pthread_t tid = 0;
static pid_t WD_pid = 0;
sched_t *sched = NULL;
static char **user_name_ptr = NULL;
static WDLogger_t *logger = NULL;
static void WriteToBuffer(char *str);

/*TODO: Uniqe semaphore name for each call
    configeration file
    fix vlg
*/

/*=================================== declarations =================================*/

static void *WDRunSched(void *arg);
static sched_t *WDInitTask(pid_t pid);
static int WDInitHandlers();
static int WDSendSignal(void *param);
static int WDCheckCounter(void *param);
static int WDCheckStopFlag(void *param);
static void empty(void *param);
static void WDIncreaseCounter(int signal, siginfo_t *info, void *context);
static void WDRaiseStopFlag(int signal, siginfo_t *info, void *context);
static int StartWithTen(int *num);
static int ReverseInt(int buffer, int num);
static char *Itoa(int num, char *str);
static void CleanUPGlobal();
static size_t WDRevive(char **arg);
static int WDLogWriteToFile(void *param);
static int SigBlock();
static int SigUnBlock();
static void AtExitHandler(void);

/*=================================== API functions =================================*/

int WDStart(int argc, char **argv)
{
    char *WD_PID_env = NULL;
    pid_t WD_PID_env_target = 0;
    char *USR_PID_env = NULL;
    pid_t USR_PID_env_target = 0;
    char str_pid[PID_ARRAY_SIZE] = {0};
    void *return_value = NULL;

    sem_t *load_sem = sem_open(SEM_LOAD, O_CREAT, SEM_PERMS, 0);
    logger = WDLoggerCreate(LOG_FILE_NAME);

    user_name_ptr = &argv[0];

    if (WD_ERROR == WDInitHandlers())
    {
        return WD_ERROR;
    }

    WD_PID_env = getenv(WD_ENV_NAME);
    if (NULL != WD_PID_env)
    {
        WD_PID_env_target = atoi(WD_PID_env);
    }

    if (WD_PID_env_target == getpid()) /*Im Watchdog*/
    {
        USR_PID_env = getenv(USR_ENV_NAME);
        assert(NULL != USR_PID_env);
        USR_PID_env_target = atoi(USR_PID_env);
        WriteToBuffer("============= Wactch Dog ============");

        sched = WDInitTask(USR_PID_env_target);
        if (NULL == sched)
        {
            return WD_ERROR;
        }

        sem_post(load_sem);
        sem_close(load_sem);
        return_value = WDRunSched((void *)argv);

#ifndef NDEBUG
        printf("WD finish: %d\n", WD_PID_env_target);
#endif

        CleanUPGlobal();
        if (SUCCESS != (size_t)return_value)
        {
            return WD_ERROR;
        }
        return WD_SUCCES;
    }

    if (WD_PID_env_target == getppid()) /*Watchdog is my father*/
    {
        WD_pid = WD_PID_env_target;
#ifndef NDEBUG
        printf("Im son of WD\n");
#endif
    }

    else /*no watchdog started yet*/
    {
        WriteToBuffer("============= USR ============");
        WriteToBuffer("forking new WD");
#ifndef NDEBUG
        printf("forking new WD\n");
#endif
        setenv(USR_ENV_NAME, Itoa(getpid(), str_pid), 1);
        WD_pid = fork();
        if (0 > WD_pid)
        {
            return WD_ERROR;
        }
        if (0 == WD_pid)
        {
            if (0 != execv(WD_EXEC_NAME, argv))
            {
                return WD_ERROR;
            }
        }
    }

    sched = WDInitTask(WD_pid);
    if (NULL == sched)
    {
        return WD_ERROR;
    }

    sem_wait(load_sem);
    if (WD_SUCCES != pthread_create(&tid, NULL, WDRunSched, sched))
    {
        return WD_ERROR;
    }

    sem_close(load_sem);
    /* if (0 != SigBlock())
    {
        perror("couldnt block signals from main thread");
        WriteToBuffer("couldnt block signals from main thread");
    } */
    return WD_SUCCES;
}

int WDStop(void)
{
    size_t i = 0;
    void *return_value = NULL;
    SchedRemove(sched, uid[WD_CHECK_COUNTER]);
    WriteToBuffer("USR stop monitoring WD and Send SIGUSR2 to him");
    /* if (0 != SigUnBlock())
    {
        perror("couldnt UnBlock signals from main thread");
        WriteToBuffer("couldnt Unblock signals from main thread");
    } */
    WDLoggerWriteFile(logger);

    kill_WD = WD_UP;
    for (i = 0; i < MISS_FACTOR; ++i)
    {
        if (stop_flag == WD_UP)
        {
            SchedStop(sched);
            pthread_join(tid, &return_value);
            if (SUCCESS != (size_t)return_value)
            {
                return WD_ERROR;
            }

            return WD_SUCCES;
        }

#ifndef NDEBUG
        printf("send siguser2\n");
#endif
        kill(WD_pid, SIGUSR2);
        sleep(1);
    }
    return WD_ERROR;
}

/*====================================== STATIC ===================================*/
/*==================================================================================*/

static void *WDRunSched(void *arg)
{
    size_t status = SUCCESS;

    while (1)
    {
        status = SchedRun(sched);

        if (kill_WD != WD_UP)
        {
            if (WD_ERROR == WDRevive((char **)arg))
            {
                return (void *)((size_t)WD_ERROR);
            }
        }
        else
        {
#ifndef NDEBUG
            printf("good exit%d \n", getpid());
#endif
            kill(WD_pid, SIGTERM);
            wait(NULL);
            WDLoggerWriteFile(logger);
            sem_unlink(SEM_LOAD);
            sem_unlink(SEM_WRITE);
            unsetenv(USR_ENV_NAME);
            CleanUPGlobal();
            SchedDestroy(sched);
            WDLoggerDestroy(logger);
            return (void *)((size_t)WD_SUCCES);
        }
    }
}

static size_t WDRevive(char **arg)
{
    char str_pid[PID_ARRAY_SIZE] = {0};
    sem_t *load_sem = sem_open(SEM_LOAD, O_CREAT, SEM_PERMS, 0);
    if (NULL != getenv(WD_ENV_NAME) && (int)getpid() == atoi(getenv(WD_ENV_NAME)))
    {
        WriteToBuffer("reviving USR with WD");
#ifndef NDEBUG
        printf("reviving USR with WD %d\n", getpid());
#endif
        setenv(USR_ENV_NAME, Itoa(getpid(), str_pid), 1);
        unsetenv(WD_ENV_NAME);
        WDLoggerWriteFile(logger);
        WDLoggerDestroy(logger);
        if (0 != execv(arg[0], arg))
        {
            WriteToBuffer("reviving USR with WD Failed");
            return ((size_t)WD_ERROR);
        }
    }
    else
    {
        WriteToBuffer("reviving WD with USR");
#ifndef NDEBUG
        printf("reviving WD with USR %d\n", getpid());
#endif
        wait(NULL);
        global_pid = fork();
        if (0 == global_pid)
        {
            sem_close(load_sem);
            if (0 != execv(WD_EXEC_NAME, user_name_ptr))
            {
                WriteToBuffer("reviving WD with USR Failed");
                return ((size_t)WD_ERROR);
            }
        }
        else
        {
            WDLoggerWriteFile(logger);
            kill(WD_pid, SIGTERM);
            WD_pid = global_pid;
        }
        sem_wait(load_sem);
        sem_close(load_sem);
        return (size_t)WD_SUCCES;
    }
    return (size_t)WD_SUCCES;
}

/*========================================= Init ==================================*/
static sched_t *WDInitTask(pid_t pid)
{
    sched = SchedCreate();
    global_pid = pid;
    if (NULL == sched)
    {
        return NULL;
    }

    uid[WD_SEND_SIGNAL] = SchedAdd(sched, 1, 1, WDSendSignal, &pid, NULL, empty);
    if (UIDIsSame(UIDBadUID, uid[WD_SEND_SIGNAL]))
    {
        return NULL;
    }
    uid[WD_CHECK_COUNTER] = SchedAdd(sched, 6, 5, WDCheckCounter, (void *)sched, NULL, empty);
    if (UIDIsSame(UIDBadUID, uid[WD_CHECK_COUNTER]))
    {
        return NULL;
    }
    if (UIDIsSame(UIDBadUID, SchedAdd(sched, 10, 10, WDLogWriteToFile, (void *)global_pid, NULL, empty)))
    {
        return NULL;
    }

    if (pid != WD_pid)
    {

#ifndef NDEBUG
        printf("init Check stop flag for WD\n");
#endif
        uid[WD_CHECK_FLAG] = SchedAdd(sched, 0, 1, WDCheckStopFlag, (void *)sched, NULL, empty);
        if (UIDIsSame(UIDBadUID, uid[WD_CHECK_FLAG]))
        {
            return NULL;
        }
    }

    return sched;
}

static int WDInitHandlers()
{
    struct sigaction sa2 = {0};
    struct sigaction sa1 = {0};

    if (0 != sigfillset(&sa1.sa_mask))
    {
        return WD_ERROR;
    }
    sa1.sa_flags = SA_SIGINFO;
    sa1.sa_sigaction = WDIncreaseCounter;
    if (0 != sigaction(SIGUSR1, &sa1, NULL))
    {
        return WD_ERROR;
    }

    if (0 != sigfillset(&sa2.sa_mask))
    {
        return WD_ERROR;
    }
    sa2.sa_flags = SA_SIGINFO;
    sa2.sa_sigaction = WDRaiseStopFlag;
    if (0 != sigaction(SIGUSR2, &sa2, NULL))
    {
        return WD_ERROR;
    }

    return WD_SUCCES;
}

/*========================================= Tasks ===================================*/

static void WriteToBuffer(char *str)
{
    WDLoggerWriteBuffer(logger, str);
}

static int WDSendSignal(void *param)
{
    if (0 != kill(global_pid, SIGUSR1))
    {
        return OP_ERROR;
    }

    WriteToBuffer("Sends SIGUSR1 to target");

    return OP_CONTINIUE;
    (void)param;
}

static int WDCheckCounter(void *param)
{
    static int warnings = 0;
    if (0 == global_counter)
    {
        WriteToBuffer("no signals recived");
#ifndef NDEBUG
        printf("no massages\n");
#endif
        SchedStop(sched);
    }

#ifndef NDEBUG
    printf("checked by %d num of signals recived: %d\n", global_pid, global_counter);

    if (global_counter >= 1 && global_counter <= 3)
    {

        WriteToBuffer("WARNING !, receving low signal");

        printf("WARNING !, receving low signal from %d\n", global_pid);
        ++warnings;
    }
#endif

    global_counter = 0;

    return OP_CONTINIUE;
}

static int WDCheckStopFlag(void *param)
{
    int i = 0;
    if (WD_UP == stop_flag)
    {
        WriteToBuffer("============= WD ============sending SIGUSR2 to USR");
        WDLoggerWriteFile(logger);
        WDLoggerDestroy(logger);
        SchedStop(sched);

        for (i = 0; i < MISS_FACTOR; ++i)
        {

#ifndef NDEBUG
            printf("process %d sent SIGUSR2\n", getpid());
#endif
            kill(global_pid, SIGUSR2);
            sleep(1);
        }
        return OP_DONE;
    }

    return OP_CONTINIUE;
}

static int WDLogWriteToFile(void *param)
{
    sem_t *write_sem = sem_open(SEM_WRITE, O_CREAT, SEM_PERMS, 1);
    if ((pid_t)param == WD_pid)
    {
        WriteToBuffer("============= USR ============");
    }
    else
    {
        WriteToBuffer("============= WD ============");
    }

    sem_wait(write_sem);
    if (LOG_FAILED == WDLoggerWriteFile(logger))
    {
        return OP_ERROR;
    }
    sem_post(write_sem);
    sem_close(write_sem);
    return OP_CONTINIUE;
}

static void empty(void *param)
{
    (void)param;
}

/*=========================================== Handlers ================================*/

static void WDIncreaseCounter(int signal, siginfo_t *info, void *context)
{
    assert(NULL != info);

    if (info->si_pid == global_pid)
    {
        ++global_counter;
    }

    (void)(signal);
    (void)(context);
}

static void WDRaiseStopFlag(int signal, siginfo_t *info, void *context)
{
    assert(NULL != info);

    if (info->si_pid == global_pid)
    {
        stop_flag = WD_UP;
    }
    (void)(signal);
    (void)(context);
}

/*============================================= HELPERS ====================================*/

static char *Itoa(int num, char *str)
{
    int buffer = 0;
    char *start = str;
    char ch = 0;
    int flag = 0;

    assert(NULL != str);

    if (0 > num)
    {
        *str = '-';
        ++str;
        num *= -1;
    }
    else if (0 == num)
    {
        *str = '0';
        *(str + 1) = '\0';
        return start;
    }

    flag = StartWithTen(&num);

    buffer = ReverseInt(buffer, num);

    while (0 < buffer) /*init the string*/
    {
        ch = (buffer % 10) + INT_TO_CHAR_CONVERT_NUMBER;
        *str = ch;
        ++str;
        buffer = buffer / 10;
    }
    while (flag) /*adding the zeros*/
    {
        *str = '0';
        ++str;
        --flag;
    }

    *str = '\0';

    return start;
}
static int StartWithTen(int *num)
{
    int flag = 0;
    while (0 == *num % 10) /*dealing if with the first is 0*/
    {
        flag += 1;
        *num = *num / 10;
    }

    return flag;
}
static void CleanUPGlobal()
{
    int i = 0;
    for (i = 0; i < NUM_OF_TASKS; ++i)
    {
        uid[i] = UIDBadUID;
    }

    global_pid = 0;
    global_counter = 0;
    stop_flag = WD_DOWN;
    kill_WD = WD_DOWN;
    tid = 0;
    WD_pid = 0;
}
static int ReverseInt(int buffer, int num)
{
    while (0 < num) /*reverse int*/
    {
        buffer += num % 10;
        buffer *= 10;
        num = num / 10;
    }
    buffer /= 10;

    return buffer;
}
static int SigBlock()
{
    sigset_t set;

    /* Initialize the signal set */
    if (sigemptyset(&set) == -1)
    {
        perror("Failed to initialize the signal set");
        return 1;
    }

    /* Add SIGUSR1 and SIGUSR2 to the set */
    if (sigaddset(&set, SIGUSR1) == -1 || sigaddset(&set, SIGUSR2) == -1)
    {
        perror("Failed to add signals to the set");
        return 1;
    }

    /* Block the signals */
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
    {
        perror("Failed to block signals");
        return 1;
    }

    return 0;
}
static int SigUnBlock()
{
    sigset_t set;

    /* Initialize the signal set */
    if (sigemptyset(&set) == -1)
    {
        perror("Failed to initialize the signal set");
        return 1;
    }

    /* Add SIGUSR1 and SIGUSR2 to the set */
    if (sigaddset(&set, SIGUSR1) == -1 || sigaddset(&set, SIGUSR2) == -1)
    {
        perror("Failed to add signals to the set");
        return 1;
    }

    if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1)
    {
        perror("Failed to unblock signals");
        return 1;
    }

    return 0;
}