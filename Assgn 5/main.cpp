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
vector <pthread_mutex_t> roomLocks;
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

// --- Threads ---
void * guest_thread ( void * params )
{
    // initialize myPriority to a random number between 1 and Y
    int myPriority = rand() % Y + 1;

    // generate a random ID string for the guest
    string ID = "";
    for (int i = 0; i < 4; ++i) ID += (char)(rand() % 26 + 65);
    cout << "Guest " << ID << " : my priority is " << myPriority << "\n";

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

        // if this guest is the last guest i.e. the 2 * Nth guest
        // then post on the cleanerAlloc semaphore
        if (roomQueue.size() == 0)
        {
            pthread_mutex_unlock(&roomQueueLock);
            cout << "Guest " << ID << " : last guest with room " << roomInfo.second << endl;
            for (int i = 0; i < N; ++i) sem_post(&cleanerAlloc);
            continue;
        }

        // if noone has stayed in the same room in the past,
        // push it back to the set with your priority
        if (stayedInPast[roomInfo.second] == 0 ) 
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

        // if someone has already stayed in the room and has vacated it
        else if(( stayedInPast[roomInfo.second]==1 && roomInfo.first==0 ))
        {
            stayedInPast[roomInfo.second]++;
            cout << "Guest " << ID << " : staying in the room " << roomInfo.second << " second! Priority : " << roomInfo.first << endl;
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
            
            if (roomQueue.find({myPriority, roomInfo.second}) != roomQueue.end() && stayedInPast[roomInfo.second] == 1)
            {
                cout << "Guest " << ID << " : leaving room " << roomInfo.second << " after " << rand_time << " seconds" << endl;
                roomQueue.erase({myPriority, roomInfo.second});
                roomQueue.insert({0, roomInfo.second});
                // cout<<"Guest " << ID << " : I inserted room " << roomInfo.second <<endl;
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
    // generate a random ID string for the cleaner
    string ID = "";
    for (int i = 0; i < 4; ++i) ID += (char)(rand() % 26 + 65);

    while (1)
    {
        // wait for the cleanerAlloc semaphore to be posted
        sem_wait(&cleanerAlloc);
        
        cout << "Cleaner " << ID << " : started" << endl;

        // lock the cleanQueueLock
        pthread_mutex_lock(&cleanQueueLock);

        // pop a room from the cleanQueue
        int room = cleanQueue.front().first;
        bool clean = cleanQueue.front().second;

        cout << "Cleaner " << ID << " : cleaning room " << room << endl;
        cleanQueue.pop();


        // unlock the cleanQueueLock
        pthread_mutex_unlock(&cleanQueueLock);

        // clean the room : wait for (lambda = 1.250) * timeStayed[room] seconds
        int clean_time = 1.250 * (float)timeStayed[room];
        sleep(clean_time);
        cout << "Cleaner " << ID << " : cleaned room " << room <<",time taken  "<< clean_time<< endl;

        // lock the clean Queue
        pthread_mutex_lock(&cleanQueueLock);

        // push the room back to the cleanQueue
        clean =1;
        cleanQueue.push(make_pair(room,clean));


        // check if it is the last cleaning thread
        if(cleanQueue.size() == N && cleanQueue.front().second)
        {

            pthread_mutex_lock(&roomQueueLock);

            cout<< "I am the last cleaner!!\n";

            timeStayed = vector <time_t> (N, 0);
            stayedInPast = vector <int> (N, 0);

            // push rooms into roomQueue
            for (int i = 0; i < N; ++i) roomQueue.insert({0, i});

            pthread_mutex_unlock(&roomQueueLock);

        }
                
        // unlock the cleanQueueLock
        pthread_mutex_unlock(&cleanQueueLock);



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

    // vector and queue inits
    roomLocks = vector <pthread_mutex_t> (N, PTHREAD_MUTEX_INITIALIZER);
    timeStayed = vector <time_t> (N, 0);
    stayedInPast = vector <int> (N, 0);
    guestSleep = new sem_t [N];
    for (int i = 0; i < N; ++i) roomQueue.insert({0, i});
    for (int i = 0; i < N; ++i) cleanQueue.push({i,0});

    // semaphore inits
    for (int i = 0; i < N; ++i) sem_init(&guestSleep[i], 0, 0);
    sem_init(&roomsAlloc, 0, 2 * N);
    sem_init(&cleanerAlloc, 0, 0);

    // other mutex and cond inits
    for (int i = 0; i < N; ++i) pthread_mutex_init(&roomLocks[i], NULL);
    
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
