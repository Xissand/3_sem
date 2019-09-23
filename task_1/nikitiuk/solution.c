#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static int ARG_MAX = 4096; //Minimum ARG_MAX by POSIX.1 standart

char **parse_cmd(char *cmd) //Parse command's name and argument list
{
	int counter = 0;
	char delim[] =" \n";
	char **list = (char**)malloc(ARG_MAX * sizeof(char*));

	//cmd isn't used anymore, therefore we don't create a copy of it to use in strtok
	for (char *p = strtok(cmd,delim); p != NULL; p = strtok(NULL, delim)) 
	{
    	//puts(p);
    	list[counter] = p;
    	counter = counter +1;
    }

    list[counter] = NULL; //Terminate argument list by NULL for exec

    return list; //Return arguments array, where list[0] is function's name
}

int run_cmd(char *cmd)
{
  const pid_t pid = fork();
  
  if (pid < 0) //Failure of fork
  {
    printf("fork failed!\n");
  }

  if (pid) //Process is a parent
  {
  	int status;
    waitpid(pid, &status, 0); //Wait for child to finish
    return WEXITSTATUS(status); //Returns child's exit status to main
  }

  //Process is a child
  
  char **args=parse_cmd(cmd); //Parse input
  execvp(args[0], args); //Overtake child
  printf("exec* failed\n"); //If child still exists - exec failed, return some unassigned error code
  return 42;
}

int main()
{
	ARG_MAX = sysconf(_SC_ARG_MAX); //Get system's limitation for argument list
	//printf("%d\n", ARG_MAX);

	char *cmd = (char *)malloc(ARG_MAX * sizeof(char));
	while(1)
	{
		fgets(cmd, ARG_MAX, stdin);
		int run_status = run_cmd(cmd); //Run command from input and get it's exit status
		printf("Process finished with exit code:%d\n", run_status);
	}
	return 0;
}
