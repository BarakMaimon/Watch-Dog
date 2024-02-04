#define _XOPEN_SOURCE 700
#include <unistd.h> /*getpid*/
#include <stdlib.h> /*setenv()*/
#include <assert.h> /*assert*/
#include <stdio.h>
#include <signal.h> /*kill()*/

#include "watchdog.h"

#define INT_TO_CHAR_CONVERT_NUMBER 48
#define PID_ARRAY_SIZE 5

static char *Itoa(int num, char *str);
static int StartWithTen(int *num);
static int ReverseInt(int buffer, int num);

int main(int argc, char *argv[])
{
    char str_pid[PID_ARRAY_SIZE] = {0};
    int x = 0;

    setenv(WD_ENV_NAME, Itoa(getpid(), str_pid), 1);
    x = WDStart(argc, argv);

    unsetenv(WD_ENV_NAME);

    return x;
}

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
