
// --- Threads ---
void * guest_thread ( void * params )
{
    // params get the char* ID

    int guestID ;
    pthread_t g = *(pthread_t*) params;

    char *buff = new char[BUFF_SIZE];
    // sprintf(buff, "Guest threadid %ld : I am a guest thread\n",g);
    // fileWrite("hotel.log", buff, "a");

    for(int i=0; i<Y; i++){
        if(guest_t[i]==g){
            guestID = i;
            break;
        }
    }


    // initialize myPriority to a random number between 1 and Y
    int myPriority = rand() % Y + 1;

    // generate a random ID string for the guest
    string ID = to_string(guestID);
    //for (int i = 0; i < 4; ++i) ID += (char)(rand() % 26 + 65);
    // cout << "Guest " << ID << " : my priority is " << myPriority << "\n";
    memset(buff, 0, BUFF_SIZE);
    sprintf(buff, "Guest %d : my priority is %d\n",guestID,myPriority);
    fileWrite("hotel.log", buff, "a");

    while (1)
    {
        // sleep for a random time between 10 and 20 seconds
        int T = rand() % 11 + 10;
        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "Guest %d : sleeping for time %d\n",guestID,T);
        fileWrite("hotel.log", buff, "a");
        sleep(T);

        // lock the mutex for roomQueue
        pthread_mutex_lock(&roomQueueLock);

        // wait for a room with viable priority to be available
        while ((roomQueue.size() > 0 && roomQueue.begin()->first > myPriority) || roomQueue.size()==0 )
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

        memset(buff, 0, BUFF_SIZE);
        sprintf(buff, "Guest %d : got room <priority,room> <%d, %d>\n",guestID,roomInfo.first,roomInfo.second);
        fileWrite("hotel.log", buff, "a");


        // if this guest is the last guest i.e. the 2 * Nth guest
        // then post on the cleanerAlloc semaphore
        if (roomQueue.size() == 0)
        {
            pthread_mutex_unlock(&roomQueueLock);

            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : last guest with room %d\n",guestID,roomInfo.second);
            fileWrite("hotel.log", buff, "a");


            pthread_mutex_lock(&cleanQueueLock);
            // mark all the rooms not clean 


            for(int i=0; i<N; i++){

                int room = cleanQueue.front().first;
                bool clean = cleanQueue.front().second;
                cleanQueue.pop();
                clean = 0;
                cleanQueue.push(make_pair(room,clean));
                 
                // signal all the threads sleeping in the rooms to quit if any 
                if(!available[i]){
                    int other_guest = guest_staying[i];
                    sem_post(&guestSleep[other_guest]);
                }
            }

            pthread_mutex_unlock(&cleanQueueLock);

            // wakeup all the cleaning threads
            for (int i = 0; i < N; ++i) sem_post(&cleanerAlloc);
            continue;
        }

        // if noone has stayed in the same room in the past,
        // push it back to the set with your priority
        if (stayedInPast[roomInfo.second] == 0 ) 
        {
            available[roomInfo.second] = 0; 
            stayedInPast[roomInfo.second]++;
            guest_staying[roomInfo.second] = guestID;

            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : staying in the room %d first! Priority : %d\n",guestID,roomInfo.second,roomInfo.first);
            fileWrite("hotel.log", buff, "a");

            roomQueue.insert({myPriority, roomInfo.second});
            
            // signal guests waiting for the room
            pthread_cond_signal(&roomQueueCond);


        }

        // if someone has already stayed in the room and has vacated it
        else if(( stayedInPast[roomInfo.second]==1 && roomInfo.first==0 ))
        {
            available[roomInfo.second] = 0; 
            stayedInPast[roomInfo.second]++;
            guest_staying[roomInfo.second] = guestID;

            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : staying in the room %d second! Priority : %d\n",guestID,roomInfo.second,roomInfo.first);
            fileWrite("hotel.log", buff, "a");
            
        } 

        // if someone is already staying in the room with less priority, make a post to the 
        // guestSleep semaphore so the current person can leave the room
        else if (stayedInPast[roomInfo.second] == 1 && !available[roomInfo.second])
        {
            int other_guest = guest_staying[roomInfo.second];
            sem_post(&guestSleep[other_guest]);

            available[roomInfo.second] = 0; 
            stayedInPast[roomInfo.second]++;
            guest_staying[roomInfo.second] = guestID;


            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : interrupting guest %d in room %d\n",guestID,other_guest,roomInfo.second);
            fileWrite("hotel.log", buff, "a");

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


        int res = sem_timedwait(&guestSleep[guestID], &sleepTime);

        // if the guest was not interrupted
        if (res == -1)
        {
            pthread_mutex_lock(&roomQueueLock);

            // no one woke you up, so you woke up yourself
            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : woke up from room %d after %ld seconds\n",guestID,roomInfo.second,rand_time);
            fileWrite("hotel.log", buff, "a");
        
            // wake up from sleep
            // if this guest is staying in this room for the first time, 
            // and wasn't interrupted, it would have pushed the room back to the queue
            // update the roomQueue entry with an empty room and priority 0
            
            available[roomInfo.second] = 1;
            if (roomQueue.find({myPriority, roomInfo.second}) != roomQueue.end() && stayedInPast[roomInfo.second] == 1)
            {
                memset(buff, 0, BUFF_SIZE);
                sprintf(buff, "Guest %d : leaving room %d after %ld seconds\n",guestID,roomInfo.second,rand_time);
                fileWrite("hotel.log", buff, "a");
            
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
            memset(buff, 0, BUFF_SIZE);
            sprintf(buff, "Guest %d : interrupted from room %d after %ld seconds\n",guestID,roomInfo.second,rand_time);
            fileWrite("hotel.log", buff, "a");

        }
    }

    pthread_exit(0);
}
