#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int ARG_MAX = 4096; // Minimum ARG_MAX by POSIX.1

char** parse_cmd(char* cmd) // Parse command's name and argument list
{
    int counter = 0;
    char delim[] = " \n";
    char** list = (char**) malloc(ARG_MAX * sizeof(char*));

    // cmd isn't used after this, so we don't create a separate copy of it to use in strtok
    for (char* p = strtok(cmd, delim); p != NULL; p = strtok(NULL, delim))
    {
        list[counter] = p;
        counter = counter + 1;
    }

    list[counter] = NULL; // Terminate argument list by NULL for exec

    return list; // Return arguments array, where list[0] is function's name
}

int run_cmd(char* cmd)
{
    const pid_t pid = fork();

    if (pid < 0) // Check for failure of fork
    {
        printf("fork failed!\n");
    }

    if (pid) // Process is a parent
    {
        int status;
        waitpid(pid, &status, 0);   // Wait for child to finish
        return WEXITSTATUS(status); // Returns child's exit status to main
    }

    // Process is a child

    char** args = parse_cmd(cmd); // Parse input
    execvp(args[0], args);        // Overtake child
    perror("exec");               // If child still exists - exec failed, return some unassigned error code
    return 42;
}

int main()
{
    ARG_MAX = sysconf(_SC_ARG_MAX); // Get system's limitation for argument list

    char* cmd = (char*) malloc(ARG_MAX * sizeof(char));
    while (1)
    {
        /* Read command from user input. Since LINE_MAX is usually much smaller than ARG_MAX (2048 and 2097152 for my
         system), command may be split into multiple lines by \*/
        fgets(cmd, ARG_MAX, stdin);
        int size = strlen(cmd);
        while (cmd[size - 2] == '\\') // If there is a backslash before \n\0, continue to next line
        {
            cmd[size - 2] = ' ';
            size -= 1;
            fgets(&cmd[size], ARG_MAX, stdin);
            size = strlen(cmd);
        }

        int run_status = run_cmd(cmd); // Run command from input and get it's exit status
        printf("Process finished with exit code:%d\n", run_status);
    }
    return 0;
}
