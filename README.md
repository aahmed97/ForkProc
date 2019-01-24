# ForkProc

Compile the files using make command on linux terminal. The command "make clean" in the terminal can be used to clear the executables from the directory

a1jobs is run with no additional command line arguments and has commands:

1) list: lists all the current jobs (processes executing program)
2) run 'program_name': runs the program specified by argument program name
3) terminate jobnum: terminates the job specified by the integer jobnum
4) suspend jobnum: suspends the job specified by the integer jobnum
5) resume jobnum: resumes the job specified by the integer jobnum
6) quit: quits the program and prints relevant timing information before terminating the main program
7) exit: terminates all jobs created during runtime and prints relevant timing information before terminating the main program

*a1jobs will fork a process and attempt to run the specified program name, if it does not exist the forked process is terminated*


a1mon is run in the form a1mon jobnum interval where the process number for a1jobs is jobnum and interval is the time interval in seconds  for printing the following information:
        list of user processes
        all jobs being monitored i.e. that were created using the run command in a running instance of a1jobs

If the a1jobs job exits then a1mon cleans up any remaining processes that were forked from it before exiting.


Design specs credited to Ehab Elmallah at the University of Alberta.
