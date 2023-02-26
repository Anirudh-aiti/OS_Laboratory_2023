#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

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
    if (nodes[i] != NULL)
    {
        graphnode *p = nodes[i];

        cout << i << " : ";
        while (p != NULL)
        {
            cout << p->val << " ";
            p = p->next;
        }
        cout << endl;
    }
    else break;
}

// function to add an edge to the nodes 2d array 
graphnode * add_edge(graphnode ** nodes, graphnode * edges, int u, int v)
{
    // add the edge to the graph
    graphnode a, b;
    a.val = u;
    a.next = NULL;
    b.val = v;
    b.next = NULL;
    edges[0] = a;
    edges[1] = b;

    // add the new nodes to the adjacency lists of u and v
    if (nodes[v] == NULL) 
    {
        nodes[v] = edges;
        nodes[v]->next = NULL;
    }
    else
    {
        graphnode *p = nodes[v];
        nodes[v] = edges;
        nodes[v]->next = p;
    }
    if (nodes[u] == NULL) 
    {
        nodes[u] = edges + 1;
        nodes[u]->next = NULL;
    }
    else
    {
        graphnode *p = nodes[u];
        nodes[u] = edges + 1;
        nodes[u]->next = p;
    }

    return edges + 2;
}

// function to get the degree of a node 
int degree(graphnode ** nodes, int u)
{
    int deg = 0;
    graphnode *p = nodes[u];
    while (p != NULL)
    {
        deg++;
        p = p->next;
    }
    return deg;
}

signed main(int argc, char const *argv[])
{
    key_t key = 1001;
    int shmid_1 = shmget(key, sizeof(graphnode *) * MAX_NODES, IPC_CREAT | 0666);
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
    int shmid_2 = shmget(key2, sizeof(graphnode) * MAX_EDGES, IPC_CREAT | 0666);
    graphnode *edges = (graphnode *)shmat(shmid_2, NULL, 0);

    // initialize the nodes and edges
    memset(nodes, 0, sizeof(graphnode *) * MAX_NODES);
    memset(edges, 0, sizeof(graphnode) * MAX_EDGES);

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
        // increment the edges pointer by 2 to point to the next edge nodes
        edges = add_edge(nodes, edges, u, v);
    }

    // close the file
    fclose(fp);
    
    // print the graph
    print_graph(nodes, MAX_NODES);

    //key to string 
    char shmid1_str[20],shmid2_str[20];
    sprintf(shmid1_str,"%d",key);
    sprintf(shmid2_str,"%d",key2);

    // fork for producer
    if (fork() == 0)
    {
        char *args[] = {(char *)"./producer", shmid1_str, shmid2_str, (char *)NULL } ;
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

    // detach the shared memory
    shmdt(nodes);
    shmdt(edges);
    shmctl(shmid_1, IPC_RMID, NULL);
    shmctl(shmid_2, IPC_RMID, NULL);

    return 0;
}
