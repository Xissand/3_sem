#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

// Total number of compute threads : 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
int THREADS_COUNT = 1;

const int DOTS = 268435456; // Total number of throws

const double STARTX = 0.0;
const double ENDX = 2.0;
//const double MAXY = 1.0;

typedef struct args { // Information about each threads interval for integration
    double startX;    // Start of the interval
    double lengthX;   // Length of interval
    double minY;      // Minimum value of integrand
    double maxY;      // Maximum value of integrand
    int dots;         // Number of throws for the thread
    int id;
} interval;

double integral[16384]; // Value of integral

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// drand48 used cause it returns value from 0 to 1 and is supposedly more random
struct drand48_data drand_buf;

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
    /*This is supported by comparison in deviation of results from analytical between 1 computatinal thread and 16384 threads.
    Larger amount of threads allows for finer bounds for Y value, which allows to throw points closer to the
    integran's curve increasing precision.*/

    for (int i = 0; i < lmao.dots; i++) // For each throw
    {
        drand48_r(&drand_buf, &dotX);                      // Randomise distance from interval's start
        drand48_r(&drand_buf, &dotY);                      // Randomise value
        dotX = lmao.startX + dotX * lmao.lengthX;          // Get throw's X coordinate
        dotY = lmao.minY + dotY * (lmao.maxY - lmao.minY); // Get throw's Y coordinate

        if ((dotY > 0) && (integrand(dotX) > dotY)) // Check, whether throw hit above zero
            hits++;
        else if ((dotY < 0) && (integrand(dotX) < dotY)) // Check, whether throw hit below zero
            hits--;
    }

    integral[lmao.id]= (lmao.maxY-lmao.minY) * lmao.lengthX * hits / lmao.dots;
    /*pthread_mutex_lock(&mutex);                              // Add number of hits to total integral
    integral += delta; // Each throw coresponds to some area around it
    pthread_mutex_unlock(&mutex); // Done in critical section because integral is shared between threads
*/
    pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
    if(argc != 2)
      printf("Assuming 1 thread\n");
    else
      sscanf(argv[1], "%d", &THREADS_COUNT);

    printf("%d\n", THREADS_COUNT);

    srand48_r(get_time(), &drand_buf); // Initialize random

    pthread_t thread[THREADS_COUNT];
    int dots_per_thread = DOTS / THREADS_COUNT;
    double length_per_thread = (ENDX - STARTX) / THREADS_COUNT;

    //"генерируя в каждом из них N/n точек равномерно по интервалу" could be interpreted in a couple of ways:
    // Each thread generates points in the entire interval or interval is split between threads
    // I believe splitting the interval guarantees more evenly distributed points. However, I could be wrong

    interval a[THREADS_COUNT];
    for (int i = 0; i < THREADS_COUNT; i++) // Initialize each threads interval
    {
        a[i].startX = STARTX + i * length_per_thread;
        a[i].lengthX = length_per_thread;
        a[i].minY = -STARTX - (i + 1) * length_per_thread;
        a[i].maxY = STARTX + (i + 1) * length_per_thread;
        a[i].dots = dots_per_thread;
        a[i].id = i;
    }

    long long time_start = get_time(); // Get starting time

    // Create threads
    for (int i = 0; i < THREADS_COUNT; i++)
        pthread_create(&thread[i], NULL, thread_routine, &a[i]);
    for (int i = 0; i < THREADS_COUNT; i++)
        pthread_join(thread[i], NULL);

    long long time_end = get_time(); // Get ending time

    long long execution_time = (time_end - time_start); // Get time it took to compute integral

    double result = 0;
    for (size_t i = 0; i < THREADS_COUNT; i++) {
      result+=integral[i];
    }

    printf("Integration result: %f\n", result);
    printf("Correct result: 1.29759\n"); // From WolframAlpha
    printf("Computation took: %lld\n", execution_time);

    FILE* log = fopen("data.log", "a");
    fprintf(log, "%d %lld\n", THREADS_COUNT, execution_time); // Add data to log file
    fclose(log);
    return 0;
}
