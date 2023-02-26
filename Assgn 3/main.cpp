#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

typedef struct graphnode
{
    int val;
    struct graphnode *next;
} graphnode;

#define NUM_CONSUMERS 10
#define MAX_NODES 10000
#define MAX_EDGES 1000000

void print_graph(graphnode **nodes, int size)
{
    for (int i = 0; i < size; ++i)
    {
        graphnode *p = nodes[i];
        if (p == NULL)
            break;

        cout << i << " : ";
        while (p != NULL)
        {
            cout << p->val << " ";
            p = p->next;
        }
        cout << endl;
    }
}

int main()
{
    key_t key = 1001;
    int shmid_1 = shmget(key, 40000, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("error in shmget");
        exit(EXIT_FAILURE);
    }

    // attach the shared memory
    graphnode **nodes = (graphnode **)shmat(shmid_1, NULL, 0);
    if (nodes == (graphnode **)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    //
    key_t key2 = 2000;
    int shmid_2 = shmget(key2, 3000000, IPC_CREAT | 0666);
    graphnode *edges = (graphnode *)shmat(shmid_2, NULL, 0);

    // initialize the nodes and edges
    memset(nodes, 0, 40000);
    memset(edges, 0, 3000000);

    // open input file
    FILE *fp = fopen("facebook_combined.txt", "r");
    if (fp == NULL)
    {
        perror("Error in opening file\n");
        exit(EXIT_FAILURE);
    }

    // string to read the file
    char str[100];

    // read the file and build the graph
    while ((fgets(str, 100, fp)) != NULL)
    {
        // read two nodes representing an edge
        int u, v;
        sscanf(str, "%d %d", &u, &v);

        // add the edge to the graph
        graphnode a, b;
        a.val = u;
        b.next = NULL;
        b.val = v;
        b.next = NULL;
        edges[0] = a;
        edges[1] = b;

        // add the new nodes to the adjacency lists of u and v
        if (nodes[v] == NULL)
            nodes[v] = edges;
        else
        {
            graphnode *p = nodes[v];
            nodes[v] = edges;
            nodes[v]->next = p;
        }
        if (nodes[u] == NULL)
            nodes[u] = edges + 1;
        else
        {
            graphnode *p = nodes[u];
            nodes[u] = edges + 1;
            nodes[u]->next = p;
        }

        // increment the edges pointer by 2 to point to the next edge nodes
        edges = edges + 2;
    }

    // close the file
    fclose(fp);
    
    // print the graph
    print_graph(nodes, 300000);

    // fork for producer
    if (fork() == 0)
    {
        char *args[] = {(char *)"./producer" , (char *)NULL } ;
        execvp( *args , args ) ;
        exit(EXIT_SUCCESS);
    }
    // fork for consumers
    else
    {
        sleep(2);
        for (int i = 0; i < NUM_CONSUMERS; ++i)
        if (fork() == 0)
        {
            char *args[] = {(char *)"./customer"  , NULL } ;
            execvp(*args , args ) ;
            exit(EXIT_SUCCESS);
        }
    }

    // wait for all the processes to finish
    // sleep(5);

    // detach the shared memory
    shmdt(nodes);
    shmdt(edges);
    shmctl(shmid_1, IPC_RMID, NULL);
    shmctl(shmid_2, IPC_RMID, NULL);

    return 0;
}
