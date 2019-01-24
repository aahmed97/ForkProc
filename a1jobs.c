#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>


#define MAXJOBS 32
#define RUN_COMMAND "run"
#define LIST_COMMAND "list"
#define RESUME_COMMAND "resume"
#define TERMINATE_COMMAND "terminate"
#define SUSPEND_COMMAND "suspend"
#define QUIT_COMMAND "quit"
#define EXIT_COMMAND "exit"


struct jobs{
    pid_t pid;
    char *command;
};

void limit_cpu(void);
int getTimes(clock_t *results);
void send_sig(char **arguments, int num_jobs, struct jobs * current_jobs);
void print_list(struct jobs *, int num_jobs);
void suspend_job(int job_no, struct jobs * current_jobs);
void resume_job(int job_no, struct jobs * current_jobs);
void terminate_job(int job_no, struct  jobs * current_jobs);
void exit_job(struct  jobs * current_jobs, int num_jobs);
void remove_newline(char * string);
int run_child_prog(char **arguments, int index, struct jobs *current_jobs, int num_jobs, char *original_command);
int str_to_i(char * string);
char * is_num(char * string);
void free_var(struct jobs *jb, int num_jobs);


int main(void){
    //limit the cpu usage
    limit_cpu();

    //initiate array to hold the user,sys,child sys, and child user times
    clock_t start_times[4];
    clock_t end_times[4];

    //get the start times
    int real_start = getTimes(start_times);

    //initiate variables and get the pid of a1jobs process
    pid_t pid_a1jobs = getpid();
    int num_jobs = 0;
    int index = 0;
    struct jobs *current_jobs = malloc(sizeof(struct jobs)*MAXJOBS);
    if(current_jobs == NULL){
      printf("There was an error allocating space for jobs array\n");
      exit(-1);
    }
    char command[1000];
    char * arguments[6];
    char * tokens;
    char delim[2] = " ";
    int new_job_succ = 0;
    char original_command[1000];

    //The main loop of the program. Ask the user for a command and execute the command
    //    with the specified behaviour from user input. Loops until user terminates the program
    //       with a exit or quit.
    while(1){
        printf("a1jobs[%d]:", pid_a1jobs);

        //Get the string from user input containing the command and tokenize it. Store
        //    tokens in argument for determining the proper action. If user inputs a unrecognized
        //      command loop back to the beginning
	       if(fgets(command, 1000, stdin) == NULL){
	          printf("An error has occurred while reading input");
            exit(1);
	    }
        strcpy(original_command, command);
        arguments[index++] = strtok(command, delim);
        while ((tokens = strtok(NULL, delim))!=NULL)
          arguments[index++] = tokens;
        remove_newline(arguments[index-1]);

        //Compare the command with the recognized commands and do the respective behaviour
        if((strcmp(arguments[0],RUN_COMMAND)) == 0){
            new_job_succ = run_child_prog(arguments, index, current_jobs, num_jobs, original_command);
            num_jobs+=new_job_succ;
        }
        else if((strcmp(arguments[0],QUIT_COMMAND)) == 0) {
            break;
        }
        else if((strcmp(arguments[0], LIST_COMMAND)) == 0) {
            print_list(current_jobs, num_jobs);
        }
        else if((strcmp(arguments[0],SUSPEND_COMMAND)==0)||(strcmp(arguments[0],RESUME_COMMAND)==0)||(strcmp(arguments[0],TERMINATE_COMMAND)==0)) {
            send_sig(arguments, num_jobs, current_jobs);
        }
        else if((strcmp(arguments[0],EXIT_COMMAND) == 0)) {
          exit_job(current_jobs, num_jobs);
          break;
        }
        //Print an error mesage if the command is unrecognized
        else{
          printf("The entered command is not recognized\n");
        }

        //Empty the command, original command, and arguments array
        memset(command, 0, sizeof(command));
        memset(original_command, 0, sizeof(original_command));
        memset(arguments, 0, sizeof(arguments));
        index = 0;
    }

    //Get the termination  times of the program
    int real_end = getTimes(end_times);
    static long clktck = 0;
    if (clktck == 0){
        if ((clktck = sysconf(_SC_CLK_TCK))<0){
            printf("sysconf error\n");
            exit(-1);
        }
    }
    //Print the respective elapsed times (end-start)
    printf("real:   %7.2f\n", (real_end-real_start)/(double)clktck);
    printf("user:   %7.2f\n", (end_times[0]-start_times[0])/(double)clktck);
    printf("sys:   %7.2f\n", (end_times[1]-start_times[1])/(double)clktck);
    printf("child user:   %7.2f\n", (end_times[2]-start_times[2])/(double)clktck);
    printf("child_sys:   %7.2f\n", (end_times[3]-start_times[3])/(double)clktck);
  }

//Takes in the tokenized command, the current number of jobs, and the struct
//    containing the executed jobs. Send the specified signal to the job number
//      taken from the command.
// Returns: None
void send_sig(char **arguments, int num_jobs, struct jobs * current_jobs){
    int job_no;
    if(is_num(arguments[1])){
        job_no = str_to_i(arguments[1]);
    }
    else{
        printf("Invalid job number\n");
        return;
      }
    if(job_no<num_jobs){
        if ((strcmp(arguments[0], SUSPEND_COMMAND))==0){
            suspend_job(job_no, current_jobs);
        }
        else if ((strcmp(arguments[0], TERMINATE_COMMAND))==0){
            terminate_job(job_no, current_jobs);
        }
        else if((strcmp(arguments[0],RESUME_COMMAND)) == 0){
            resume_job(job_no, current_jobs);
        }
    }
    else{
      printf("That job does not exist\n");
    }
    return;
}

//Forks a child process and runs the program specified in the user input within the child process
//    If the specified program does not exist then the child process is killed and the function Returns
//      to the main program.
//  Parameters: arguments:(array containing tokenized string), current jobs:(struct containing executed jobs),
//          num_jobs: number of jobs, original_command:the user input
// Returns: 1 if program executes successfully in child otherwise 0
int run_child_prog(char **arguments, int index, struct jobs *current_jobs, int num_jobs, char *original_command){
    pid_t pid;
    pid_t pid_ch;
    int new_job_success = 0;
    int status = 0;
    int exec_ret = 0;
    int num_arg = index-2;
    char *program_name = arguments[1];
    if(num_jobs<32){
      if ((pid = fork()) < 0){
          printf("Fork error\n");
          exit(-1);
      }
      else if (pid == 0){
          if (num_arg==0){
          exec_ret = execlp(program_name, program_name,(char *)0);
          }
          else if (num_arg == 1){
          exec_ret = execlp(program_name, program_name, arguments[2], (char *)0);
          }
          else if(num_arg==2){
          exec_ret= execlp(program_name, program_name, arguments[2], arguments[3],(char *)0);
          }
          else if (num_arg ==3){
          exec_ret= execlp(program_name, program_name, arguments[2], arguments[3], arguments[4], (char *)0);
          }
          else{
          exec_ret=execlp(program_name, program_name, arguments[2], arguments[3], arguments[4], arguments[5], (char *)0);
          }
          if(exec_ret == -1){
              printf("There was an error with execlp (executing program in child\n)");
              kill(getpid(), SIGKILL);
              exit(-1);
          }
      }
      if((current_jobs[num_jobs].command = (char *)malloc(200)) == NULL){
          printf("Error in allocating space for struct command\n");
          exit(-1);
      }
      sleep(1);
      strcpy(current_jobs[num_jobs].command, original_command);
      current_jobs[num_jobs].pid = pid;
      new_job_success = 1;
      printf("Job added successfully\n");
      }
  else{
    printf("The maximum number of jobs have been admitted\n");
  }
return new_job_success;
}

//Limits the cpu time of the program
//  Parameters: void
//  Returns: void
void limit_cpu(){
    struct rlimit limit;
    limit.rlim_cur = 600;
    limit.rlim_max = 700;
    setrlimit(RLIMIT_CPU, &limit);
    return;
}

// stores the sys, user, child sys, child user in an array results
//  Parameters: clock_t results array: to store times
//  Returns: elapsed real time
int getTimes(clock_t *results){
    struct tms current;
    struct timespec tsp;
    clock_t time_;
    time_ = times(&current);
    results[0] = current.tms_utime;
    results[1] = current.tms_stime;
    results[2] = current.tms_cutime;
    results[3] = current.tms_cstime;
    return time_;
}

// Prints the list of non-terminated jobs
//  Parameters: current_jobs: struct containing executed jobs, num_jobs: the number of jobs executed successfully
//  Returns: void
void print_list(struct jobs * current_jobs, int num_jobs){
    for(int i = 0; i<num_jobs; i++){
      if((current_jobs[i].command)!=NULL)
            printf("%d: PID=    %d, cmd= %s", i, current_jobs[i].pid, current_jobs[i].command);
    }
    return;
}

//Suspends the job specified by the user
//  Parameters: job_no: the num of the job to suspend, current_jobs: struct containing executed jobs
//  Returns: void
void suspend_job(int job_no, struct jobs * current_jobs){
    pid_t pid = current_jobs[job_no].pid;
    kill(pid, SIGSTOP);
    return;
}

//Resumes the job specified by the user
//  Parameters: job_no: the num of the job to resume, current_jobs: struct containing executed jobs
//  Returns: void
void resume_job(int job_no, struct jobs * current_jobs){
    pid_t pid = current_jobs[job_no].pid;
    kill(pid, SIGCONT);
    return;
}

//terminates the job specified by the user
//  Parameters: job_no: the num of the job to terminate, current_jobs: struct containing executed jobs
//  Returns: void
void terminate_job(int job_no, struct jobs * current_jobs){
    pid_t pid = current_jobs[job_no].pid;
    kill(pid, SIGKILL);
    current_jobs[job_no].command = NULL;
    return;
}

//terminates all the head processes that have not been terminated already
//  Parameters: current_jobs: struct containing executed jobs, num_jobs: the current number of jobs
//  Returns: void
void exit_job(struct  jobs * current_jobs, int num_jobs){
    for (int i = 0; i < num_jobs; i++){
      if(current_jobs[i].command != NULL){
        printf("    Job %d terminated\n", current_jobs[i].pid);
        terminate_job(i, current_jobs);
    }
  }
    return;
}

//Removes new line from the end of a string
//  Parameters: string: ptr to string for newline to be removed from
//  Returns: void
void remove_newline(char * string){
  char * ptr = string;
  while(*ptr != '\0'){
    if (*ptr == '\n')
      *(ptr++) = '\0';
    else
      ptr++;
  }
  return;
}

//Converts a string representation of an integer into an int
// Parameters: string: ptr to string to be converted
// Returns: num: the integer number the string contained
int str_to_i(char * string){
    int num = 0;
    char *ptr = string;
    while(*ptr != '\0'){
        num = (num*10) + (*ptr-'0');
        ptr++;
    }
    return num;
}

// Checks if a string contains only digit characters
//   Parameters: string: ptr to string to be parsed
//   Returns: string: ptr to input string if contains digit char or null if there are other
//        non digit chars
char * is_num(char * string){
    char *ptr = string;
    while(*ptr != '\0'){
        if((*ptr<'0')||(*ptr>'9')){
            ptr = NULL;
            return ptr;
        }
        ptr++;
    }
    return string;
}

//Frees the dynamically allocated memory used in the program
//  Parameters: jb: array of executed jobs, num_jobs: the number of executed jobs
//  Returns: void
void free_var(struct jobs *jb, int num_jobs){
    for(int i = 0; i < num_jobs; i++){
        free(jb[i].command);
    }
    free(jb);
}
