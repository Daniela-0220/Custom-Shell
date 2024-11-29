#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "wsh.h"

struct job *job_list[256]; // Array of job structures
pid_t shell_pid;
int curfgid;

// Signal handler for SIGINT (Ctrl-C)
void sigint_handler(int signo) {
    if (curfgid != 0) {
        // Send SIGINT to the foreground process group
        kill(-(job_list[curfgid - 1] -> pgid), SIGINT);
        job_list[curfgid - 1] = NULL;
    }
}

// Signal handler for SIGTSTP (Ctrl-Z)
void sigtstp_handler(int signo) {
    if (curfgid != 0) {
        // Send SIGTSTP to the foreground process group
        kill(-(job_list[curfgid - 1] -> pgid), SIGTSTP);
        job_list[curfgid - 1] -> stopped = 1;
    }
}

// Signal handler for SIGCHILD (detect when a background job (child process) has finished)
void sigchld_handler(int signo){
    int status;
    pid_t child_pid;

    // Use waitpid with WNOHANG to check for child processes without blocking
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < 256; i++) {
            if (job_list[i] != NULL && job_list[i]->id == child_pid) {
                if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTSTP){
                    kill(-job_list[i]->pgid, SIGINT);
                    job_list[i]->stopped = 1;
                }
            }
            else if(WIFSIGNALED(status)){
                if(WTERMSIG(status) == SIGINT){
                    kill(-job_list[i]->pgid, SIGINT);
                    int curid = job_list[i]->id;
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                }
                
            }
            else if(WIFEXITED(status)){
                 // Iterate through the job list to find the corresponding job
                    int curid = job_list[i]->id;
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                }
                
                
                break;
            }
        } 
    }
}

//execute jobs command
void execjobs(){
    for(int i=0; i<256; i++){
        if(job_list[i] != NULL){
            printf("%d: ", job_list[i]->id);
            int j=0;
            while(job_list[i]->process[j] != NULL){
                int t = 0;
                while(job_list[i]->process[j]->args[t] != NULL){
                    printf("%s", job_list[i]->process[j]->args[t]);
                    t++;
                    if(job_list[i]->process[j]->args[t] != NULL) printf(" ");
                }
                j++;
                if(job_list[i]->process[j] != NULL) printf("| ");
            }
            if(job_list[i]->initial_back == 1) printf("&\n");
            else printf("\n");
        }
    }
}

//to parse the input
void parseInput(const char* user_input, char* args[], int* argc){
    *argc = 0;
    char *delimiter = " \n\t";
    char *token;

    token = strtok((char*)user_input, delimiter);

    while (token != NULL) {

        args[*argc] = token;
        (*argc)++;
        token = strtok(NULL, delimiter); // Get the next token
    }
    args[*argc] = NULL;
    (*argc)++;
}

//a function that execute command
//return -1 if there is an error in execute 
int execomd(const char* user_input, char* args[], int background){
    //exit
    if (strcmp(args[0], "exit") == 0) {
        if(args[1] != NULL){
            exit(-1);
        }
        for(int i=0; i<256; i++){//free the job_list
            if(job_list[i] != NULL){
                for(int j=0; job_list[i]->process[j] != NULL; j++){
                    for(int t=0; job_list[i]->process[j]->args[t] != NULL; t++){
                        free(job_list[i]->process[j]->args[t]);
                    }
                    free(job_list[i]->process[j]);
                }
                free(job_list[i]);
            }   
        }
        exit(0);
    }

    //cd
    if(strcmp(args[0], "cd") == 0){ 
        if(args[2] != NULL || args[1] == NULL){
            exit(-1);
        }
        if (chdir(args[1]) != 0) {
            perror("chdir"); 
            exit(-1);
        }
        return 0;
    }

    //jobs
    if(strcmp(args[0], "jobs") == 0){
        execjobs();
        return 0;
    }

    //fg
    if(strcmp(args[0], "fg") == 0){
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
        int job_id;
        if(args[1] != NULL){
            job_id = atoi(args[1]);
        }
        else{
            for(int i=256; i>=0; i--){
                if(job_list[i] != NULL) job_id = i+1;
            }
        }
        curfgid = job_id;
        struct job *curjob = job_list[job_id - 1];
        if(curjob == NULL){//point to the position
            exit(-1);
        }
        
        //printf("%d", curjob->pgid);
            if((tcsetpgrp(STDOUT_FILENO, curjob->pgid)) < 0) exit(-1);
        
        if(curjob->stopped == 1){//first transfer it to bg if stopped
            curjob->stopped = 0;
            if (kill (-curjob->pgid, SIGCONT) < 0)
                perror ("kill (SIGCONT)");
        }
        //else{
            int status;
            int j=0;
            while(curjob->process[j] != NULL){
                waitpid(curjob->process[j]->pid, &status, WUNTRACED);
                if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTSTP){
                    kill(-curjob->pgid, SIGTSTP);
                    curjob->stopped = 1;
                    break;
                }
            }
            else if(WIFSIGNALED(status)){
                if(WTERMSIG(status) == SIGINT){
                    kill(-curjob->pgid, SIGINT);
                    int curid = curjob->id;
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                    break;
                }
                
            }
            else if(WIFEXITED(status)){
                 // Iterate through the job list to find the corresponding job
                    int curid = curjob->id;
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                    break;
                }
                j++;
                curjob->process[j]->status = status;//set status
            }
        //}
        if((tcsetpgrp(STDOUT_FILENO, shell_pid)) < 0) exit(-1);
        curfgid = 0;
        return 0;
    }

    //bg
    if(strcmp(args[0], "bg") == 0){
        int job_id;
        if(args[1] != NULL){
            job_id = atoi(args[1]);
        }
        else{
            for(int i=256; i>=0; i--){
                if(job_list[i] != NULL) job_id = i;
            }
        }
        struct job *curjob = job_list[job_id - 1];
        if(curjob == NULL){//point to the position
            exit(-1);
        }
        curjob->stopped = 0;
        if((tcsetpgrp(STDOUT_FILENO, shell_pid)) < 0) exit(-1);
        if (kill (-curjob->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
        return 0;
    }

    //set jobs
    struct process *curproc = malloc(sizeof(struct process));
    for(int i=0; i<256; i++){
        curproc->args[i] = NULL;
    }
    for(int i=0; args[i] != NULL; i++){
        curproc->args[i] = malloc(sizeof(char) * 256);
        strcpy(curproc->args[i], args[i]);
    }
    
    struct job *curjob = malloc(sizeof(struct job));
    curjob->stopped = 0;
    for(int i=0; i<256; i++){
        curjob->process[i] = NULL;
    }
    curjob->process[0] = curproc;
    curjob->procNum = 1;
    for(int i=0; i<256; i++){
        if(job_list[i] == NULL){
            curjob->id = i+1;
            job_list[i] = curjob;
            break;
        }
    }
    if(background) curjob->initial_back = 1;
    else {curjob->initial_back = 0;
    curfgid = curjob->id;}
    //not build-in
    pid_t pid;
    int status;
    // Create a child process
    pid = fork();
    curproc->pid = pid;//set pid and pgid
    curjob->pgid = curproc->pid;
    //printf("%d", pid);
    if (pid < 0) {
        exit(-1);
    } else if (pid == 0) {// Child process
        // Execute the command
        if(getpid() != getsid(0)){
            setpgid(pid, pid); 
        }
        if(background == 0){
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
        }
        if (execvp(args[0], args) == -1) {
            perror("execvp"); // Handle execution error
            exit(-1);
        }
    } else {// Parent process
        // Wait for the child process to complete
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        if(background == 0){ // tcsetpgrp
            if((tcsetpgrp(STDOUT_FILENO, pid)) < 0) exit(-1);
            waitpid(pid, &status, WUNTRACED);
            if(WIFSTOPPED(status)){
                if(WSTOPSIG(status) == SIGTSTP){
                    kill(-curjob->pgid, SIGTSTP);
                    curjob->stopped = 1;
                }
            }
            else if(WIFSIGNALED(status)){
                if(WTERMSIG(status) == SIGINT){
                    kill(-curjob->pgid, SIGINT);
                    /*for(int j=0; curjob->process[0]->args[j] != NULL; j++){
                        free(curjob->process[0]->args[j]);
                    }
                    free(curjob->process[0]);*/
                    int curid = curjob->id;
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                }
                
            }
            else if(WIFEXITED(status)){
                 // Iterate through the job list to find the corresponding job
                    int curid = curjob->id;
                    for(int j=0; job_list[curid - 1]->process[j] != NULL; j++){
                        for(int t=0; job_list[curid - 1]->process[j]->args[t] != NULL; t++){
                            free(job_list[curid - 1]->process[j]->args[t]);
                        }
                        free(job_list[curid - 1]->process[j]);
                    }
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                }
                
            curproc->status = status;//set status
            if((tcsetpgrp(STDOUT_FILENO, shell_pid)) < 0) exit(-1);
            curfgid = 0;
        }
    }
    return 0;

}

//execute pipeline commands
int execpipe(char* input, int background){
    //split input into multiple commands
    char *cmdLine[256];
    for(int i=0; i<256; i++){
        cmdLine[i] = NULL;
    }
    int argc = 0; // Set this to the number of programs in the pipeline
    char *token;
    token = strtok((char*)input, "|");
    while (token != NULL) {
        cmdLine[argc] = token;
        argc++;
        token = strtok(NULL, "|"); // Get the next token
    }
    //int prev_pipe_read = 0; // File descriptor for reading from the previous command's output

    //create a job for this command line 
    struct job *curjob = malloc(sizeof(struct job));
    curjob->stopped = 0;
    for(int i=0; i<256; i++){
        curjob->process[i] = NULL;
    }
    curjob->procNum = argc;
    for(int i=0; i<256; i++){
        if(job_list[i] == NULL){
            curjob->id = i+1;
            job_list[i] = curjob;
            break;
        }
    }
    if(background) curjob->initial_back = 1;
    else {curjob->initial_back = 0;
    curfgid = curjob->id;}
    int pipe_fds[argc - 1][2];
    for(int i = 0; i<argc-1; i++){
        pipe_fds[i][0] = -1;
        pipe_fds[i][1] = -1;
    }
    pid_t child_pid;
    int status;
    pid_t pgid = -1;
    for (int i = 0; i < argc; i++) {
        //seperate into arguments
        char *args[256];
        for(int j=0; j<256; j++){
            args[j] = NULL;
        } 
        int argcount = 0;
        parseInput(cmdLine[i], args, &argcount);
         
        //create each process
        struct process *curproc = malloc(sizeof(struct process));
        for(int j=0; j<256; j++){
            curproc->args[j] = NULL;
        }
        for(int j=0; args[j] != NULL; j++){
            curproc->args[j] = malloc(sizeof(char) * 256);
            strcpy(curproc->args[j], args[j]);
        }
        curjob->process[i] = curproc;
        if(i < argc - 1){
            if (pipe(pipe_fds[i]) == -1) {//get fd[0] and fd[1], each is read end and write end
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        child_pid = fork();
        if ((child_pid) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0) { // Child process
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            if (i != 0) {
                dup2(pipe_fds[i - 1][0], STDIN_FILENO); // Redirect input from the previous pipe
                close(pipe_fds[i - 1][0]);
            }
            if (i < argc - 1) {
                dup2(pipe_fds[i][1], STDOUT_FILENO); // Redirect output to the next pipe
                close(pipe_fds[i][1]);
            }
            // Execute the command
            if(execvp(args[0], args) == -1){
                perror("execvp");
                exit(EXIT_FAILURE);
            }
        } 
        else{
            if (pgid == -1) {
                pgid = child_pid;
            }
            setpgid(child_pid, pgid);
            curproc->pid = child_pid;
            curjob->pgid = curjob->process[0]->pid;
            if(i != 0){
                close(pipe_fds[i - 1][0]);
            }
            if(i < argc - 1){
                close(pipe_fds[i][1]);
            }
            if(background == 0){ // tcsetpgrp
                //printf("foreground\n");
                if((tcsetpgrp(STDOUT_FILENO, child_pid)) < 0) exit(-1);
                waitpid(child_pid, &status, WUNTRACED);
                if(WIFSTOPPED(status)){
                    if(i < argc - 1){
                        continue;
                    }
                    if(WSTOPSIG(status) == SIGTSTP){
                        curjob->stopped = 1;
                    }
                }
                else if(WIFSIGNALED(status)){
                    if(i < argc - 1){
                        continue;
                    }
                     if(WTERMSIG(status) == SIGINT){
                        int curid = curjob->id;
                        for(int j=0; job_list[curid - 1]->process[j] != NULL; j++){
                            for(int t=0; job_list[curid - 1]->process[j]->args[t] != NULL; t++){
                                free(job_list[curid - 1]->process[j]->args[t]);
                            }
                            free(job_list[curid - 1]->process[j]);
                        }
                        free(job_list[curid - 1]);
                        job_list[curid - 1] = NULL;
                    }
                }
                else if(WIFEXITED(status)){
                    if(i < argc - 1){
                        continue;
                    }
                    int curid = curjob->id;
                    for(int j=0; job_list[curid - 1]->process[j] != NULL; j++){
                        for(int t=0; job_list[curid - 1]->process[j]->args[t] != NULL; t++){
                            free(job_list[curid - 1]->process[j]->args[t]);
                        }
                        free(job_list[curid - 1]->process[j]);
                    }
                    free(job_list[curid - 1]);
                    job_list[curid - 1] = NULL;
                }
                if((tcsetpgrp(STDOUT_FILENO, shell_pid)) < 0) exit(-1);
                curfgid = 0;
            }
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]){
    //signal(SIGCHLD, sigchld_handler);
    //signal(SIGINT, sigint_handler);
    //signal(SIGTSTP, sigtstp_handler);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);//ignore all the signals
    for(int i=0; i<256; i++){
        job_list[i] = NULL;
    }//initialize job_list
    if((shell_pid = tcgetpgrp(STDOUT_FILENO)) < 0) exit(-1);
    //find if it's interactive mode or batch mode
    if(argc == 1){ // interactive mode
        //loop through prompt wsh untill exit appear
        while(1){
            char *user_input = NULL;
            size_t input_size = 0;
            //print prompt
            printf("wsh> ");
            //get user input
            if (getline(&user_input, &input_size, stdin) == -1) {
                // Error or EOF
                exit(-1);
            }
            if (input_size > 0 && strcmp(&user_input[input_size - 1], "\n")) {
                user_input[input_size - 1] = '\0';
            }
            //parse the input
            char *args[256]; 
            int argsc = 0;
            int background = 0;
            if (user_input[strlen(user_input) - 2] == '&') {
                background = 1;
                user_input[strlen(user_input) - 2] = '\0'; // Remove the '&' character
            }
            char *input = malloc(sizeof(char) * 256);
            strcpy(input, user_input);
            //find if this is an pipeline situation
            if(strchr(user_input, '|')){
                execpipe(user_input, background);
            }
            else{
                parseInput(user_input, args, &argsc);
                //execute command
                if(execomd(input,args, background) < 0){
                    exit(-1);
                }
            }

            free(input);
        }
    }
    else{ //batch mode
        //get into the batch file
        FILE *fp; // File pointer
        if ((fp = fopen(argv[1], "r")) == NULL) {
            exit(-1);
        }
        char *fileLine;
        size_t linelen = 0;
        while(getline(&fileLine, &linelen, fp) != -1){
            char *args[256]; 
            for(int i=0; i<256; i++){
                args[i] = NULL;
            } 
            fileLine[strcspn(fileLine, "\n")] = '\0';
            //find 
            int background = 0;
            if (fileLine[strlen(fileLine) - 2] == '&') {
                background = 1;
                fileLine[strlen(fileLine) - 2] = '\0'; // Remove the '&' character
            }
            char *input = malloc(sizeof(char) * 256);
            strcpy(input, fileLine);
            //find if this is an pipeline situation
            if(strchr(fileLine, '|')){
                execpipe(fileLine, background);
            }
            else{
                int argcount = 0;
                parseInput(fileLine, args, &argcount); //parse the input
                if(execomd(input, args, background) == -1){
                    exit(-1);
                }
            }

        }
        
        fclose(fp);
    }

    return 0;

}
