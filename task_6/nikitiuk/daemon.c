#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int foreground = 1;
int daemonic = 0;
int verbose = 0;

void parse_args(char* args) {}

void logprintf(const int fd, const char* text, const char* param)
{
    if (verbose)
        dprintf(fd, text, param);
}

char* get_time()
{
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char* ret = asctime(timeinfo);
    return ret;
}

// NOTE : code injection possible via filename with &&

char* execute(const char* command)
{
    static char ret[4096];
    FILE* pipe = popen(command, "r");
    fgets(ret, sizeof(ret), pipe);
    pclose(pipe);
    return ret;
}

void daemon_proc(const char* name, const int log)
{
    DIR* dir = opendir(name);
    if (dir)
    {
        char path[PATH_MAX];
        char buf[10000]; // bit more than PATH_MAX*2
        char* end_ptr = path;
        struct stat info;
        struct dirent* e;
        strcpy(path, name);
        end_ptr = &path[strlen(path)];
        *(end_ptr++) = '/';

        while ((e = readdir(dir)) != NULL)
        {
            if ((e->d_name[0]) == '.' || !(strcmp((e->d_name), "backup")))
                continue;
            strcpy(end_ptr, e->d_name);

            if (!stat(path, &info))
            {
                if (S_ISDIR(info.st_mode))
                {
                    dprintf(log, "directory: %s\n", path);
                    sprintf(buf, "./backup/%s", path);
                    dprintf(log, "attempt to mkdir: %s \n", buf);
                    mkdir(buf, 0755);
                    dprintf(log, "created directory: %s. %s \n\n", path, strerror(errno));
                    daemon_proc(path, log);
                }
                else if (S_ISREG(info.st_mode))
                {
                    dprintf(log, "file: %s\n", path);
                    sprintf(buf, "file \"%s\"", path);
                    char* filetype = execute(buf);
                    dprintf(log, "file type: %s", filetype);

                    if (!(strstr(filetype, "text")))
                        continue;

                    sprintf(buf, "diff -q -s -N \"%s\" \"backup/%s\"", path, path);
                    char* diff_return = execute(buf);
                    dprintf(log, "file checked: %s\n", diff_return);

                    if (strstr(diff_return, "identical"))
                        continue;

                    sprintf(buf, "cp \"%s\" \"backup/%s\"", path, path);
                    execute(buf);
                    dprintf(log, "backed up file: %s. %s \n", path, strerror(errno));
                    dprintf(log, "time of copy: %s\n", get_time());
                }
            }
        }
    }
    else
    {
        dprintf(log, "opendir: %s\n", strerror(errno));
    }
    return;
}

int main(int argc, char const* argv[])
{
    char* dir = "3_sem";
    int timeout = 60;
    if (argc < 2)
    {
        printf("Usage: daemon-file [DIRECTORY TO MONITOR] [TIMEOUT] [OPTIONS]\n");
        printf("Path should be RELATIVE, timeout is in seconds\n");
        printf("Options: -f    run in foreground mode\n");
        printf("         -b    run in background mode\n");
        // printf("Is this what you wanted\n");
    }
    else if (strcmp(argv[1], "-b") == 0)
    {
        pid_t pid = fork();
        if (!pid)
        {
            setsid();
            close(0);
            close(1);
            close(2);

            int log = open("daemon.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
            dprintf(log, "---------------------------------------------------------\n");
            dprintf(log, "Daemon initiated in background mode\n");
            dprintf(log, "%s\n", get_time());

            mkdir("backup", 0755);
            char temp[8192];
            sprintf(temp, "backup/%s", dir);
            mkdir(temp, 0755);

            while (1)
            {
                dprintf(log, "---------------------------------------------------------\n");
                dprintf(log, "%s\n", get_time());
                daemon_proc(dir, log);
                sleep(timeout);
            }
        }
        else if (pid == -1)
        {
            perror("lmao");
        }
        else
        {
            printf("daemon pid%d\n", pid);
        }
    }
    else if (strcmp(argv[1], "-f") == 0) // run in forground
    {
        int log = open("daemon.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
        dprintf(log, "---------------------------------------------------------\n");
        dprintf(log, "Daemon initiated in foreground mode\n");
        dprintf(log, "%s\n", get_time());

        mkdir("backup", 0755);
        char temp[8192];
        sprintf(temp, "backup/%s", dir);
        mkdir(temp, 0755);

        while (1)
        {
            dprintf(log, "---------------------------------------------------------\n");
            dprintf(log, "%s\n", get_time());
            daemon_proc(dir, log);
            sleep(timeout);
        }
    }
    return 0;
}
