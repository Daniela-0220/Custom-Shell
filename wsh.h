typedef struct process
{
  char *args[256];                /* for exec */
  pid_t pid;                  /* process ID */
  int status;                 /* reported status value */
} process;

typedef struct job
{             
  int id; /*job id that we print in jobs command*/
  process *process[256];     /* list of processes in this job */
  pid_t pgid;                 /* process group ID */
  char stopped;              /* true if user told about stopped job */
  int procNum;
  int initial_back;        /*to find if we need to type in & after this job in jobs command*/
  int completed;  /*completed = 1*/
} job;
