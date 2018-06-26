HEADERS = rrd_format.h  rrd_snprintf.h

default: rrd2csv

rrd2csv: rrd_csv.o rrd_open.o rrd_snprintf.o rrd_error.o rrd_thread_safe.o rrd_format.o rrd_utils.o optparse.o
	gcc -Wall rrd_csv.o rrd_open.o rrd_snprintf.o rrd_error.o rrd_thread_safe.o rrd_format.o rrd_utils.o optparse.o -pthread -o rrd2csv
       
rrd_csv.o: rrd_csv.c
	gcc -Wall -c rrd_csv.c

rrd_open.o: rrd_open.c
	gcc -Wall -c rrd_open.c

rrd_snprintf.o: rrd_snprintf.c
	gcc -Wall -c rrd_snprintf.c

rrd_utils.o: rrd_utils.c
	gcc -Wall -c rrd_utils.c -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include/

rrd_error.o: rrd_error.c
	gcc -Wall -c rrd_error.c

rrd_thread_safe.o: rrd_thread_safe.c
	gcc -Wall -c rrd_thread_safe.c -pthread

rrd_format.o: rrd_format.c
	gcc -Wall -c rrd_format.c

optparse.o: optparse.c
	gcc -Wall -c optparse.c

clean:
	rm -f *.o
	rm -f rrd2csv

