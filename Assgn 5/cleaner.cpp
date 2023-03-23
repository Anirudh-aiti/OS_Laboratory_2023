void * cleaner_thread ( void * params )
{
    // generate a random ID string for the cleaner
    string ID = "";
    for (int i = 0; i < 4; ++i) ID += (char)(rand() % 26 + 65);

    while (1)
    {
        // wait for the cleanerAlloc semaphore to be posted
        sem_wait(&cleanerAlloc);
        char* buff = new char[BUFF_SIZE];

        
        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "Cleaner %s : started\n",ID.c_str());
        fileWrite("hotel.log", buff, "a");


        // lock the cleanQueueLock
        pthread_mutex_lock(&cleanQueueLock);

        // pop a room from the cleanQueue
        int room = cleanQueue.front().first;
        bool clean = cleanQueue.front().second;

        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "Cleaner %s : cleaning room %d\n",ID.c_str(),room);
        fileWrite("hotel.log", buff, "a");

        cleanQueue.pop();


        // unlock the cleanQueueLock
        pthread_mutex_unlock(&cleanQueueLock);

        // clean the room : wait for (lambda = 1.250) * timeStayed[room] seconds
        int clean_time = 1.250 * (float)timeStayed[room];
        sleep(clean_time);

        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "Cleaner %s : cleaned room %d\n",ID.c_str(),room);
        fileWrite("hotel.log", buff, "a");


        // lock the clean Queue
        pthread_mutex_lock(&cleanQueueLock);

        // push the room back to the cleanQueue
        clean =1;
        cleanQueue.push(make_pair(room,clean));


        // check if it is the last cleaning thread
        if(cleanQueue.size() == N && cleanQueue.front().second)
        {

            pthread_mutex_lock(&roomQueueLock);

            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Cleaner %s : I am the last cleaner!!\n",ID.c_str());
            fileWrite("hotel.log", buff, "a");

            timeStayed = vector <time_t> (N, 0);
            stayedInPast = vector <int> (N, 0);

            // push rooms into roomQueue
            for (int i = 0; i < N; ++i) roomQueue.insert({0, i});


            pthread_mutex_unlock(&roomQueueLock);

            pthread_cond_broadcast(&roomQueueCond);
        }
                
        // unlock the cleanQueueLock
        pthread_mutex_unlock(&cleanQueueLock);



    }

    pthread_exit(0);
}
