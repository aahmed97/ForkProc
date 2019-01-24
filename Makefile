all: a1jobs a1mon
	
a1jobs: a1jobs.c
	gcc a1jobs.c -o a1jobs

a1mon: a1mon.c
	gcc a1mon.c -o a1mon

tar: a1jobs.c a1mon.c Makefile ReportA1.pdf
	tar -cvf submit.tar a1jobs.c a1mon.c Makefile ReportA1.pdf

clean:
	rm -rf *.o
