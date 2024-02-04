Requierments:

- Fault tolerance
- Thread safe
- Watchdog per process

Files:
watchdog.h - User API
whatchdog_utils.h - Internal API
whatchdog_utils.c
watchdog.c - fork and excec watchdog.out
make watchdog
watchdog_process.c: Main for the watchdog process. includes libwhatchdog.so
Out Files:
libwhatchdog.so:
wtachdog.o: includes watchdog.h, whatchdog_utils.o
whatchdog_utils.o: includes whatchdog_utils.h, scheduler.h
watchdog_process.exe: compiled from watchdog_process.c using a makefile

==========================================================
User process:
global_counter = 0;
stop_flag = 0;

WDStart():
Create Signal Handler (SIG USR 1)
Create Signal Handler (SIG USR 2)
if(env_variable != ppid())
fork and exec WD
semwait(for WD finish upload)
RunScheduler(SendSignal(1 sec interval),
CheckCounter(5 sec interval),
CheckStopFlag(2 sec interval));

---

SigUsr1Handler():
++global_counter
SigUsr2Handler():
flag = 1;

==========================================================
WD Process:
global_counter = 0;
stop_flag = 0;

ExecWD():

    Create Signal Handler (SIG USR 1);
    Create Signal Handler (SIG USR 2);

    sempost(Tell User that upload complete);

    RunScheduler(SendSignal(1 sec interval),
    			 CheckCounter(5 sec  interval),
    			 CheckStopFlag(2 sec interval));

---

SigUsr1Handler():
++global_counter
SigUsr2Handler():
flag = 1;
============================================================
shared library:

SendSignal(pid_t pid):
kill(pid,SIG USR 1);

CheckCounter(global_counter, struct what_to_revive):
if(global_counter <= 3 && global_counter >= 2)
warning;
if(global_counter >= 0 && global_counter <= 1)
Reviving(what_to_revive);
global_counter = 0;

CheckStopFlag(pid_t pid):
if(flag == 1)
while(Response from WD)
kill(pid,SIG USR 2);
StopScheduler();
