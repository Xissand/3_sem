typedef struct {    // Information about each threads interval for integration
    double startX;  // Start of the interval
    double lengthX; // Length of interval
    double minY;    // Minimum value of integrand
    double maxY;    // Maximum value of integrand
    int dots;       // Number of throws for the thread
    int id;         // Id of the thread
} interval;
