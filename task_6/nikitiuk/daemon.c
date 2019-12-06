#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h> //Source for PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

char* get_time() // Returns a string with current system date and time
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char* ret = asctime(timeinfo);
    return ret;
}

// NOTE : code injection possible via filename with &&
char* execute(const char* command) // Executes a bash command, passed to it
{
    static char ret[4096];            // Create a return field for command output
    FILE* pipe = popen(command, "r"); // Popen create a pipe, in which it returns command output
    fgets(ret, sizeof(ret), pipe);    // Get output
    pclose(pipe);                     // Close pipe
    return ret;                       // Return output to caller
}

void daemon_proc(const char* name, const int log) // Daemon routine
{
    DIR* dir = opendir(name); // Open directory from call parameters
    if (dir)                  // If opendir succeed
    {
        char path[PATH_MAX];  // Path to file being checked
        char buf[10000];      // Bit more than PATH_MAX*2 to have enough space for cp and diff commands
        char* end_ptr = path; // Pointer to end of path
        struct stat info;
        struct dirent* e;
        strcpy(path, name);            // Initialize path
        end_ptr = &path[strlen(path)]; // and pointer
        *(end_ptr++) = '/';            // Add backslash at the end, because "name" is a directory

        while ((e = readdir(dir)) != NULL) // Go through all files in the directory
        {
            // NOTE: This skips hidden files, but let's say they're hidden for a reason
            if ((e->d_name[0]) == '.' || !(strcmp((e->d_name), "backup"))) // Skip ../, ./ and backup folders
                continue;

            strcpy(end_ptr, e->d_name); // Append path with new filename

            if (!stat(path, &info)) // Get information about the file, stat returns 0 on success
            {
                if (S_ISDIR(info.st_mode)) // If it's a directory
                {
                    dprintf(log, "directory: %s\n", path); // Log its name
                    sprintf(buf, "./backup/%s", path);     // Add ./backup/ at the start of path
                    dprintf(log, "Attempt to mkdir: %s \n", buf);
                    mkdir(buf, 0755); // Make this directory in backup, if it exists, mkdir does nothing
                    dprintf(log, "created directory: %s. %s \n\n", path, strerror(errno)); // Log mkdir return
                    daemon_proc(path, log); // Start routine in that directory
                }
                else if (S_ISREG(info.st_mode)) // If it's a regular file
                {
                    dprintf(log, "file: %s\n", path);        // Log its name
                    sprintf(buf, "file \"%s\"", path);       // Create "file filename" bash command,
                                                             // file is in quotes to support spaces in filenames
                    char* filetype = execute(buf);           // Execute bash command
                    dprintf(log, "file type: %s", filetype); // Log its return

                    if (!(strstr(filetype, "text"))) // Skip all non-text files
                        continue;

                    sprintf(buf, "diff -q -s -N \"%s\" \"backup/%s\"", path, path);
                    // Create "diff file backed_up_file" bask command
                    // -q formats output for different files
                    // -s formats output for indentical files
                    // -N treats nonexistent files as different
                    char* diff_return = execute(buf);                // Execute bash command
                    dprintf(log, "file checked: %s\n", diff_return); // Log its return

                    if (strstr(diff_return, "identical")) // If backup is up-to-date skip it
                        continue;

                    sprintf(buf, "cp \"%s\" \"backup/%s\"", path, path); // Create "cp file backed_up_file" bash command
                    execute(buf);                                        // Execute it
                    dprintf(log, "backed up file: %s. %s \n", path, strerror(errno)); // Log its return
                    dprintf(log, "time of copy: %s\n", get_time());                   // Log time of operation
                }
            }
        }
    }
    else // If opendir failed, log its error value
    {
        dprintf(log, "opendir: %s\n", strerror(errno));
    }
    return;
}

int main(int argc, char const* argv[])
{
    char* dir = ".";  // Head directory to back up
    int timeout = 60; // Timeout between scans
    if (argc < 2)
    {
        printf("Usage: daemon-file [OPTIONS] [TIMEOUT]\n");
        printf("Daemon should be started in directory it is supposed to back up\n");
        printf("Timeout should be specified in seconds, default is 60\n");
        printf("Options: -f    run in foreground mode\n");
        printf("         -b    run in background mode\n");
    }
    else
    {
        if (argc > 2) // If custom timeout is specified, read it
        {
            sscanf(argv[2], "%d", &timeout);
            printf("Timeout set as %d\n", timeout);
        }
        if (strcmp(argv[1], "-b") == 0) // Run daemon as a daemon
        {
            pid_t pid = fork(); // Fork
            if (!pid)
            {
                setsid(); // Change session
                close(0); // Close standart input
                close(1); // Close standart output
                close(2); // Close standart error

                int log = open("daemon.log", O_WRONLY | O_APPEND | O_CREAT, 0644); // Create or append a log file
                dprintf(log, "---------------------------------------------------------\n");
                dprintf(log, "Daemon initiated in background mode\n");
                dprintf(log, "%s\n", get_time());           // Log starting time
                dprintf(log, "Daemon pid: %d\n", getpid()); // Log daemon pid

                mkdir("backup", 0755); // Create backup folder. If it exists, mkdir does nothing

                while (1) // Run scans until stopped by user
                {
                    dprintf(log, "---------------------------------------------------------\n");
                    dprintf(log, "%s\n", get_time()); // Log start of scan
                    daemon_proc(dir, log);            // Start daemon routine
                    sleep(timeout);                   // Wait until next scan
                }
            }
            else if (pid == -1) // On unsuccessful fork, notify user
            {
                perror("Fork failed");
            }
            else // On successful fork display daemon pid to user
            {
                printf("Daemon pid: %d\n", pid);
            }
        }
        else if (strcmp(argv[1], "-f") == 0) // Run as a normal programm
        {
            int log = open("daemon.log", O_WRONLY | O_APPEND | O_CREAT, 0644); // Create or append a log file
            dprintf(log, "---------------------------------------------------------\n");
            dprintf(log, "Daemon initiated in foreground mode\n"); // Log starting time
            dprintf(log, "%s\n", get_time());                      // Log daemon pid

            mkdir("backup", 0755); // Create backup folder. If it exists, mkdir does nothing

            while (1) // Run scans until stopped by user
            {
                dprintf(log, "---------------------------------------------------------\n");
                dprintf(log, "%s\n", get_time()); // Log start of scan
                daemon_proc(dir, log);            // Start daemon routine
                sleep(timeout);                   // Wait until next scan
            }
        }
    }
    return 0;
}
