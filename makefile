
all: lib_wd.so client_exec.out wd_exec.out

WD_LIB_SRC = watchdog.c WDLogger.c ../../ds/src/scheduler.c ../../ds/src/priority_queue.c ../../ds/src/uid.c ../../ds/src/task.c ../../ds/src/dlist.c ../../ds/src/sorted_list.c ../../ds/src/dvector.c

lib_wd.so: $(WD_LIB_SRC)
	gcc -ansi -pedantic-errors -Wall -g  -I ./ -I ../../ds/include -fPIC -shared $(WD_LIB_SRC) -o lib_wd.so -lpthread

wd_exec.out: wd_exec.c
	gcc -ansi -pedantic-errors -Wall -g  -I ./ -I ../../ds/include wd_exec.c -o wd_exec.out -L. -l_wd -Wl,-rpath=.

client_exec.out: client_exec.c
	gcc -ansi -pedantic-errors -Wall -g -I ./ -I ../../ds/include client_exec.c -o client_exec.out -L. -l_wd -Wl,-rpath=.

clean:
	rm -f lib_wd.so wd_exec.out client_exec.out
