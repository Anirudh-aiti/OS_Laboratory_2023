#include <set>
#include <vector>
#include <assert.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <queue>
#include <string.h>
#define BUFF_SIZE 300
using namespace std;


// --- Global Variables ---

// semaphores
sem_t roomsAlloc; // semaphore for the number of rooms to be allocated
sem_t cleanerAlloc; // semaphore to allocate cleaners
sem_t * guestSleep; 

// thread_t
pthread_t * cleaner_t;
pthread_t * guest_t;

// mutexes and cond
pthread_mutex_t roomQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t roomQueueCond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cleanQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cleanQueueCond = PTHREAD_COND_INITIALIZER;

// queues , arrays and other globals
long long X, Y, N;
multiset <pair<int, int>> roomQueue; // multiset of <room_no, priority>
queue <pair<int,bool>> cleanQueue; // queue of <room_no>
vector <time_t> timeStayed;
vector <int> stayedInPast;
vector<bool> available;
vector<int> guest_staying;

// Print a string to a file using fwrite in given mode
// (to print to stdout, use "stdout" as the file name)
void fileWrite(string file, string str, string mode)
{
    if (file == "stdout") {
        char * buff = new char[str.length() + 1];
        strcpy(buff, str.c_str());
        fwrite(buff, sizeof(char), str.length(), stdout);
    }
    else {
        FILE * fp = fopen(file.c_str(), mode.c_str());
        char * buff = new char[str.length() + 1];
        strcpy(buff, str.c_str());
        fwrite(buff, sizeof(char), str.length(), fp);
        fclose(fp);
    }
}

#include "guest.cpp"
#include "cleaner.cpp"


// --- Main ---
signed main()
{
    // Taking input for N, X, Y
    cout << "Enter N (number of rooms): ";
    cin >> N;
    cout << "Enter X (strength of cleaning staff): ";
    cin >> X;
    cout << "Enter Y (number of guests): ";
    cin >> Y;

    ////// CHECK INITS

    // init
    srand(time(NULL));
    setvbuf(stdout, NULL, _IONBF, 0);

    //file open
    fileWrite("hotel.log", "","w");

    // vector and queue inits
    timeStayed = vector <time_t> (N, 0);
    stayedInPast = vector <int> (N, 0);
    available = vector<bool> (N,1);
    guestSleep = new sem_t [Y];
    guest_staying = vector <int> (Y, -1);

    for (int i = 0; i < N; ++i) roomQueue.insert({0, i});
    for (int i = 0; i < N; ++i) cleanQueue.push({i,0});

    // semaphore inits
    for (int i = 0; i < Y; ++i) sem_init(&guestSleep[i], 0, 0);
    sem_init(&roomsAlloc, 0, 2 * N);
    sem_init(&cleanerAlloc, 0, 0);

    
    // Creating threads for cleaner and guests
    cleaner_t = new pthread_t[X];
    guest_t = new pthread_t[Y];
    int indx = 0;
    while(indx < Y)
    {
        // char c[1];
        // c[0] = (char) i;
         pthread_create( & guest_t[indx], NULL, guest_thread,(void*)(&guest_t[indx]) );
         indx++;
    }
    for ( int i = 0; i < X; i++ ) {
        pthread_create( & cleaner_t[i], NULL, cleaner_thread, NULL );

    }   

    //  Waiting for threads to finish
    for ( int i = 0; i < X; i++ ) pthread_join( cleaner_t[i], NULL );
    for ( int i = 0; i < Y; i++ ) pthread_join( guest_t[i], NULL );

    // Destroying semaphores
    for (int i = 0; i < Y; ++i) sem_destroy(&guestSleep[i]);
    sem_destroy(&roomsAlloc);

    // Destroying mutexes and cond
    pthread_mutex_destroy(&roomQueueLock);

    // Destroying threads
    delete [] cleaner_t;
    delete [] guest_t;
    
    return 0;
}
