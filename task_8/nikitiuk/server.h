#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Total number of compute threads : 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384

typedef struct {    // Information about each threads interval for integration
    double startX;  // Start of the interval
    double lengthX; // Length of interval
    double minY;    // Minimum value of integrand
    double maxY;    // Maximum value of integrand
    int dots;       // Number of throws for the thread
    int id;         // Id of the thread
} interval;

volatile double integral[16384]; // Value of integral

// drand48 used cause it returns value from 0 to 1 and is supposedly more random
struct drand48_data drand_buf[16384];

double integrand(double x) { return x * sin(1 / x); } // Integrand function

long long get_time() // Returns current time in microseconds
{
    struct timeval time;
    gettimeofday(&time, NULL);
    long long curr = time.tv_sec * 1000000 + time.tv_usec;
    return curr;
}

void* thread_routine(void* args)
{
    interval lmao = *(interval*) args; // Get struct with information from args

    double dotX, dotY; // Each throw's coordinates
    int hits = 0;      // Number of throws that hit per thread

    /*Integrate function using Neumann's generation method:
      Generate a random number x, uniformly distributed between Xmin and Xmax ,
      Generate a second independent random number u uniformly distributed between fmin and fmax
      If u < f(x), then accept x. If not, reject x and repeat. */

    for (int i = 0; i < lmao.dots; i++) // For each throw
    {
        drand48_r(&drand_buf[lmao.id], &dotX);             // Randomise distance from interval's start
        drand48_r(&drand_buf[lmao.id], &dotY);             // Randomise value
        dotX = lmao.startX + dotX * lmao.lengthX;          // Get throw's X coordinate
        dotY = lmao.minY + dotY * (lmao.maxY - lmao.minY); // Get throw's Y coordinate

        if ((dotY > 0) && (integrand(dotX) > dotY)) // Check, whether throw hit above zero
            hits++;
        else if ((dotY < 0) && (integrand(dotX) < dotY)) // Check, whether throw hit below zero
            hits--;
    }

    integral[lmao.id] = (lmao.maxY - lmao.minY) * lmao.lengthX * hits / lmao.dots;
    /*pthread_mutex_lock(&mutex);                              // Add number of hits to total integral
    integral += delta; // Each throw corresponds to some area around it
    pthread_mutex_unlock(&mutex); // Done in critical section because integral is shared between threads
    */
    pthread_exit(NULL);
}
