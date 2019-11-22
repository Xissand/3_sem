#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIGUSR1 10 // used as zero
#define SIGUSR2 12 // used as one

// Some recipient variables.
char* recieved_data = NULL; // array with recieved data
long recieved_amount = 0;   // amount of recieved data
long alocated = 0;          // ammount of allocated space in bytes
pid_t sender_pid;

void add_space() // reallocate a kb if we ran out of bytes
{
    char* buffer = recieved_data;
    recieved_data = malloc(alocated + 1024);
    memcpy(recieved_data, buffer, alocated);
    free(buffer);
    alocated += 1024;
}

void signal_handler(int signo)
{
    // recieved bit is in a byte with number recieved_amount/8 at position recieved_amount%8
    if (signo == SIGUSR1) // change apropriate bit to a zero
    {
        if (!(recieved_amount / 8 < alocated)) // if we have more data than space, add space
            add_space();
        recieved_data[recieved_amount / 8] &= ~(1 << (recieved_amount % 8));
        recieved_amount++;
        kill(sender_pid, SIGUSR1); // send signal back to syncronize the queue
    }
    else if (signo == SIGUSR2) // change apropriate bit to a one
    {
        if (!(recieved_amount / 8 < alocated)) // if we have more data than space, add space
            add_space();
        recieved_data[recieved_amount / 8] |= (1 << (recieved_amount % 8));
        recieved_amount++;
        kill(sender_pid, SIGUSR1); // send signal back to syncronize the queue
    }
    else if (signo == SIGINT) // if somebody tries to terminate recipient, save data to file
    {
        FILE* fp = fopen("output.txt", "wb");
        fwrite(recieved_data, 1, recieved_amount / 8, fp);
        fclose(fp);
        exit(0);
    }
}

int main(int argc, char const* argv[])
{
    if (argc == 1) // Nothing of importance
    {
        printf("usage: -r or -w filename\n");
        printf("Chillin\n");
        return 0;
    }
    if (strcmp(argv[1], "-w") == 0) // Sender take arguments -w and file to be transmitted
    {
        pid_t pid = getpid();
        printf("Sender pid: %d\n", getpid()); // show own pid
        printf("Input it into recipient\n");
        // user has to input sender pid into recipient than recipient pid into sender

        // add SIGUSR1 to signal set and block it to use for syncronization
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGUSR1);
        sigprocmask(SIG_BLOCK, &set, NULL);
        int sig;

        sigwait(&set, &sig); // wait for recipient to be ready

        printf("Input recipient pid:");
        scanf("%d", &pid); // get recipient pid for kill

        if (argc == 2)
        {
            printf("No file set\n");
            return 0;
        }

        // read data from file to data array
        const char* data = argv[2];
        FILE* fileptr;
        char* transmission;
        long size;

        fileptr = fopen(data, "rb"); // Open the file in binary mode
        fseek(fileptr, 0, SEEK_END); // Jump to the end of the file
        size = ftell(fileptr);       // Get the current byte offset in the file
        rewind(fileptr);             // Jump back to the beginning of the file

        transmission = (char*) malloc((size) * sizeof(char)); // Enough memory for file
        fread(transmission, size, 1, fileptr);                // Read in the entire file
        fclose(fileptr);                                      // Close the file

        printf("Size %ld\n", size);

        for (size_t i = 0; i < size; i++) // counter for bytes
        {
            for (size_t bit_i = 0; bit_i < 8; bit_i++) // counter for bits in one byte
            {
                if ((transmission[i]) & (1 << (bit_i))) // compare bit at bit_i with 1
                    kill(pid, SIGUSR2);                 // send 1 if it is 1
                else                                    // if not
                    kill(pid, SIGUSR1);                 // send 0

                sigwait(&set, &sig); // wait for recipient to finish writing the bit
            }
        }

        kill(pid, SIGINT); // terminate the recipient
    }
    else if (strcmp(argv[1], "-r") == 0) // recipient takes -r
    {
        printf("Recipient pid: %d\nInput sender pid:", getpid()); // show own pid
        scanf("%d", &sender_pid);                                 // get sender pid for kill
        kill(sender_pid, SIGUSR1);                                // signal for start of the transmission
        signal(SIGUSR1, signal_handler);                          // assign handler for 0
        signal(SIGUSR2, signal_handler);                          // 1
        signal(SIGINT, signal_handler);                           // and termination
        while (1)
            sleep(1);
    }
    return 0;
}
