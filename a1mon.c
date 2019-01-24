#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

struct monitored_proc{
    char *command;
    pid_t pid;
    int exists;
};

void limit_cpu();
int str_to_i(char * string);
char * is_num(char * string);
void terminate_tree(struct monitored_proc * proc, int num_monitored);
void free_var(int num_monit,struct monitored_proc * proc, char * line);

int main(int argc, char *argv[]) {
    //limit the cpu time and get the pid of a1mon
    limit_cpu();
    pid_t target_pid;
    pid_t pid = getpid();

    //initialize variables
    int interval;
    pid_t current_ppid;
    int index = 0;
    int target_exists = 0;
    int num_monitored = 0;
    int max_num_monitored = 500;
    int interval_exists = 0;
    char *line_split[6];
    pid_t current_pid;
    char *temp;
    int already_monit = 0;
    char * tokens;
    struct monitored_proc *monit_p = malloc(max_num_monitored*sizeof(struct monitored_proc));
    if (monit_p == NULL){
      printf("Allocation error monit_p line 42\n");
      exit(-1);
    }

    //if there are enough command line args for a1mon, convert interval to an integer (if exists)
    //    and convert the argument for target pid to integer. Exit if arguments are not in correct
    //      formats or not enough arguments are supplied
    if(argc>1){
        if(is_num(argv[1])){
            target_pid = str_to_i(argv[1]);
        }
        else{
            printf("An error occurred with command line arguments\n");
            exit(-1);
        }
        if(argc == 3){
            if(is_num(argv[2])){
                interval_exists = 1;
                interval = str_to_i(argv[2]);
              }
            else{
                printf("Non number entered for interval. Exiting...\n");
                exit(-1);
            }
        }
    }
    else{
        printf("Insufficient command line arguments specified. ./a1mon target_pid [interval]\n");
        exit(-1);
    }

    //initialize variables relating to the string lines contained in fp ptr to ps output
    FILE *fp;
    int count = 0;
    int line_num = 0;
    int num_lines = 100;
    int str_length = 300;
    char *line = malloc(300);
    if (line == NULL){
      printf("Allocation error line 82, line\n");
      exit(-1);
    }

    //Loop and add monitored processes while at the same time checking if the target process
    //    still exists
    while(1) {

      //print the headers
      if(interval_exists == 0){
          printf("a1mon   [counter= %d, pid= %d, target_pid= %d]: \n", count, pid, target_pid);
      }
      else{
          printf("a1mon   [counter= %d, pid= %d, target_pid= %d, interval = %d sec]: \n", count, pid, target_pid, interval);
      }

      //open the pipe to ps and if fp is null then terminate since there was an error
      fp = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r");
      if (fp == NULL){
          printf("There was an error opening the stream\n");
          exit(-1);
      }

      //Print the header and get a line from ps. If there is not enough space in the variable then
      //    allocate more
      printf("USER      PID   PPID S STARTED CMD\n");
      printf(".................................\n");
      for(int i = 0; i < num_monitored; i++){
        monit_p[i].exists = 0;
      }
      while(fgets(line, str_length, fp)){
          if(line){
            temp = line;
              if(line[strlen(line)-1]!='\n'){
                  str_length = str_length * 2;
                  line = realloc(line, str_length);
                  if (line == NULL){
                    printf("Error reallocating space, line, line 118\n");
                    exit(-1);
                  }
                  fgets(line, str_length, fp);
                  printf("%s", line);
              }
              printf("%s", line);

              //tokenize the line string
              line_split[index++] = strtok(temp," \t\n");
              while (((tokens = strtok(NULL, "  \t\n"))!=NULL) && (index<6)) {
                      line_split[index++] = tokens;
              }
              if(str_length >= 200){
                str_length = 200;
                line = realloc(line, str_length);
                if(line == NULL){
                  printf("Error reallocating space for line command, line 136\n");
                  exit(-1);
                }
              }
            index = 0;

            //convert the pid and ppid from the line. convert it to an int.
            current_pid = str_to_i(line_split[1]);
            current_ppid = str_to_i(line_split[2]);
          if(num_monitored >= max_num_monitored){
              max_num_monitored*=2;
              monit_p = realloc(monit_p, max_num_monitored);
              if (monit_p == NULL){
                printf("Error reallocating space for monit_p (monitored processes), line 150\n");
                exit(-1);
              }
          }

          //add process to monitor list if its parent id = target id and if it is not already in the list
              if(current_ppid == target_pid){
                  for (int i = 0; i < num_monitored; i++){
                      if(current_pid == monit_p[i].pid){
                          already_monit = 1;
                          monit_p[i].exists = 1;
                      }
                  }
                  if (!(already_monit)){
                      monit_p[num_monitored].command = malloc(strlen(line_split[5])+1);
                      if (monit_p[num_monitored].command == NULL){
                        printf("Error allocating space for command, line 154\n");
                        exit(-1);
                      }
                      strcpy(monit_p[num_monitored].command, line_split[5]);
                      monit_p[num_monitored].pid = current_pid;
                      monit_p[num_monitored++].exists = 1;
                  }
                  already_monit = 0;
              }

              //if the id contained in this line of ps = target pid then we set a flag that it exists
              if(current_pid == target_pid){
                  target_exists = 1;
              }

              int temp = num_monitored;
              //add any child processes of monitored processes to the list of monitored processes
              //    if it does not already exist in that
              for(int i = 0; i<temp; i++){
                  if(current_ppid == monit_p[i].pid){
                      for (int j = 0; j < num_monitored; j++){
                          if(current_pid == monit_p[j].pid){
                              already_monit = 1;
                              monit_p[j].exists = 1;
                              break;
                      }
                  }
                      if(!(already_monit)){
                          monit_p[num_monitored].command = malloc(strlen(line_split[5])+1);
                          if (monit_p[num_monitored].command == NULL){
                            printf("Error allocating space for command, line 184\n");
                            exit(-1);
                          }
                          strcpy(monit_p[num_monitored].command, line_split[5]);
                          monit_p[num_monitored].pid = current_pid;
                          monit_p[num_monitored++].exists = 1;
                          already_monit = 0;
                  }
                  }
                  if (current_pid == monit_p[i].pid){
                    monit_p[i].exists = 1;
                  }
              }
              }
              }
      //reached the end of the ps file we print the monitored processes if the target proc exists
      fclose(fp);
      for(int i = 0; i < num_monitored; i++){
        if(monit_p[i].exists == 0){
          free(monit_p[i].command);
          monit_p[i].command = NULL;
        }
      }
      if(target_exists){
          int count_proc = 0;
          printf("-------------------------------------------------\n");
          printf("List of monitored processes:\n");
          printf("[");
          if(num_monitored>0){
                for(int i = 0; i < num_monitored-1; i++){
                  if(monit_p[i].exists == 1){
                    printf("%d:[%d,%s], ", count_proc, monit_p[i].pid, monit_p[i].command);
                    count_proc++;
                  }
                }
                  if(monit_p[num_monitored-1].exists == 1){
                      printf("%d:[%d,%s]]\n\n", count_proc, monit_p[num_monitored-1].pid,monit_p[num_monitored-1].command);
                    }
                    else
                      printf("]\n\n");
          }
          target_exists = 0;
          }

      //if the target proc no longer exists then we terminate the tree and exit the a1mon program
      else{
          printf("\n");
          printf("a1mon: target appears to have terminated; cleaning up\n");
          terminate_tree(monit_p, num_monitored);
          printf("Exiting a1mon...\n");
          free_var(num_monitored,monit_p,line);
          exit(0);
      }
      //iterate the count and sleep (if applicable) before looping again if the target
      //    process still exists
      if(interval_exists){
          sleep(interval);
      }
      count++;
  }
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

//Converts a string representation of an integer into an int
//   Parameters: string: ptr to string to be converted
//   Returns: num: the integer number the string contained
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

//The head process has been terminated so we terminate all descendant processes
//    Parameters: struct of all monitored proc, num_monitored: the number of proc being monitored
//    Returns: void
void terminate_tree(struct monitored_proc * proc, int num_monitored){
    int i;
    pid_t current_pid;
    for(i = 0; i < num_monitored; i++){
        current_pid = proc[i].pid;
        if(proc[i].command != NULL){
            printf("terminating [%d, %s]\n", current_pid, proc[i].command);
            kill(current_pid, SIGKILL);
      }
    }
}

//Frees the dynamically allocated memory used in the program
//  Parameters: jb: num_monit: number of monitored proc, monitored_proc: array containing monitored proc,
          // line: contains a line from ps
//  Returns: void
void free_var(int num_monit,struct monitored_proc * proc, char * line){
    for(int i = 0; i < num_monit;i++){
        if(proc[i].command != NULL){
            free(proc[i].command);
          }
    }
    free(proc);
    free(line);
}
