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

using namespace std;

// --- Global Variables ---

// semaphores
sem_t roomsAlloc; // semaphore for the number of rooms to be allocated
sem_t * guestSleep;

// thread_t
pthread_t * cleaner_t;
pthread_t * guest_t;

// mutexes and cond
vector <pthread_mutex_t> roomLocks;
pthread_mutex_t roomQueueLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t roomQueueCond = PTHREAD_COND_INITIALIZER;

// queues , arrays and other globals
long long X, Y, N;
multiset <pair<int, int>> roomQueue; // multiset of <room_no, priority>
vector <time_t> timeStayed;
vector <int> stayedInPast;

// --- Threads ---
void * guest_thread ( void * params )
{
    // initialize myPriority to a random number between 1 and Y
    int myPriority = rand() % Y + 1;

    // generate a random ID string for the guest
    string ID = "";
    for (int i = 0; i < 4; ++i) ID += (char)(rand() % 26 + 65);

    while (1)
    {
        // sleep for a random time between 10 and 20 seconds
        int T = rand() % 11 + 10;
        cout << "Guest " << ID << " : sleeping for time " << T << "\n";
        sleep(T);

        // lock the mutex for roomQueue
        pthread_mutex_lock(&roomQueueLock);

        // wait for a room with viable priority to be available
        while (roomQueue.size() > 0 && roomQueue.begin()->first > myPriority)
            pthread_cond_wait(&roomQueueCond, &roomQueueLock);

        // if no room is available, wait for a room to be free
        if (roomQueue.size() == 0) 
        {
            pthread_mutex_unlock(&roomQueueLock);
            continue;
        }

        // get the room with the lowest priority
        pair <int, int> roomInfo;
        roomInfo = *roomQueue.begin();
        roomQueue.erase(roomQueue.begin());
        cout << "Guest " << ID << " : got room <" << roomInfo.first << ", "  << roomInfo.second << "> " << endl;

        // if noone has stayed in the same room in the past,
        // push it back to the set with your priority
        if (stayedInPast[roomInfo.second] == 0) 
        {
            cout << "Guest " << ID << " : staying in the room " << roomInfo.second << " first! Priority : " << roomInfo.first << endl;
            roomQueue.insert({myPriority, roomInfo.second});
            
            // signal guests waiting for the room
            pthread_cond_signal(&roomQueueCond);

            stayedInPast[roomInfo.second]++;
        }

        // if someone is already staying in the room with less priority, make a post to the 
        // guestSleep semaphore so the current person can leave the room
        else if (stayedInPast[roomInfo.second] == 1)
        {
            cout << "Guest " << ID << " : signaling the guest in room " << roomInfo.second << " to leave! Priority : " << roomInfo.first << endl;
            sem_post(&guestSleep[roomInfo.second]);
            stayedInPast[roomInfo.second]++;
        }    

        // two people have stayed, go back to wait
        else 
        {
            cout << "Guest " << ID << " : two guests have stayed in the room " << roomInfo.second << " already! Priority : " << roomInfo.first << endl;
            pthread_mutex_unlock(&roomQueueLock);
            continue;
        }

        // unlock the mutex for roomQueue
        pthread_mutex_unlock(&roomQueueLock);

        // sleep for some random time between 10 to 30 seconds in the room
        struct timespec sleepTime;
        clock_gettime(CLOCK_REALTIME, &sleepTime);
        time_t rand_time = rand() % 21 + 10;
        sleepTime.tv_sec +=  rand_time;
        timeStayed[roomInfo.second] += rand_time;
        int res = sem_timedwait(&guestSleep[roomInfo.second], &sleepTime);

        // if the guest was not interrupted
        if (res == -1)
        {
            // no one woke you up, so you woke up yourself
            cout << "Guest " << ID << " : woke up from room " << roomInfo.second << " after " << rand_time << " seconds" << endl;

            // wake up from sleep
            // if this guest is staying in this room for the first time, 
            // and wasn't interrupted, it would have pushed the room back to the queue
            // update the roomQueue entry with an empty room and priority 0
            pthread_mutex_lock(&roomQueueLock);
            
            if (roomQueue.find({myPriority, roomInfo.second}) != roomQueue.end())
            {
                cout << "Guest " << ID << " : leaving room " << roomInfo.second << " after " << rand_time << " seconds" << endl;
                roomQueue.erase(roomInfo);
                roomQueue.insert({0, roomInfo.second});
            }

            // signal that the room is free
            pthread_cond_signal(&roomQueueCond);

            pthread_mutex_unlock(&roomQueueLock);
        }       

        // if the guest was interrupted 
        else if (res == 0) {
            // use sem_timedwait to check if the guest was interrupted
            cout << "Guest " << ID << " : interrupted from room " << roomInfo.second << " after " <<  sleepTime.tv_sec - time(NULL) << " seconds" << endl;
        }
    }

    pthread_exit(0);
}

void * cleaner_thread ( void * params )
{
    while (1)
    {

    }

    pthread_exit(0);
}

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
    roomLocks = vector <pthread_mutex_t> (N, PTHREAD_MUTEX_INITIALIZER);
    timeStayed = vector <time_t> (N, 0);
    stayedInPast = vector <int> (N, 0);
    guestSleep = new sem_t [N];
    for (int i = 0; i < N; ++i) roomQueue.insert({0, i});

    // semaphore inits
    for (int i = 0; i < N; ++i) sem_init(&guestSleep[i], 0, 0);
    sem_init(&roomsAlloc, 0, 2 * N);

    // mutex and cond inits
    for (int i = 0; i < N; ++i) pthread_mutex_init(&roomLocks[i], NULL);
    pthread_mutex_init(&roomQueueLock, NULL);
    pthread_cond_init(&roomQueueCond, NULL);

    // Creating threads for cleaner and guests
    cleaner_t = new pthread_t[X];
    guest_t = new pthread_t[Y];
    for ( int i = 0; i < Y; i++ ) pthread_create( & guest_t[i], NULL, guest_thread, NULL );
    for ( int i = 0; i < X; i++ ) pthread_create( & cleaner_t[i], NULL, cleaner_thread, NULL );

    //  Waiting for threads to finish
    for ( int i = 0; i < X; i++ ) pthread_join( cleaner_t[i], NULL );
    for ( int i = 0; i < Y; i++ ) pthread_join( guest_t[i], NULL );

    // Destroying semaphores
    for (int i = 0; i < N; ++i) sem_destroy(&guestSleep[i]);
    sem_destroy(&roomsAlloc);

    // Destroying mutexes and cond
    for (int i = 0; i < N; ++i) pthread_mutex_destroy(&roomLocks[i]);
    pthread_mutex_destroy(&roomQueueLock);

    // Destroying threads
    delete [] cleaner_t;
    delete [] guest_t;
    
    return 0;
}
