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
#include <fstream>

using namespace std;

#define MAX_NODES 10000
#define MAX_EDGES 1000000
#define NUM_CONSUMERS 10

typedef struct graphnode
{
    int val;
    struct graphnode *next;
} graphnode;

// graphnode *add_edge(graphnode **nodes, graphnode *edges, int u, int v)
// {
//     // add the edge to the graph
//     graphnode a, b;
//     a.val = u;
//     b.next = NULL;
//     b.val = v;
//     b.next = NULL;
//     edges[0] = a;
//     edges[1] = b;

//     // add the new nodes to the adjacency lists of u and v
//     if (nodes[v] == NULL)
//     {
//         nodes[v] = edges;
//         nodes[v]->next = NULL;
//     }
//     else
//     {
//         graphnode *p = nodes[v];
//         nodes[v] = edges;
//         nodes[v]->next = p;
//     }
//     if (nodes[u] == NULL)
//     {
//         nodes[u] = edges + 1;
//         nodes[u]->next = NULL;
//     }
//     else
//     {
//         graphnode *p = nodes[u];
//         nodes[u] = edges + 1;
//         nodes[u]->next = p;
//     }

//     return edges + 2;
// }

int nodes_count(graphnode **nodes)
{
    int count = 0;
    for (int i = 0; i < MAX_NODES; ++i)
    {
        if (nodes[i] != NULL)
        {
            count++;
        }
        else
        {
            break;
        }
    }
    return count;
}

// Dijkstra's algorithm
void dijkstra_algo(int source, graphnode **nodes, int count, int start, int end, int index)
{
    int i, u, v, dist;
    int front = -1;
    int back = -1;
    int path[MAX_NODES];
    int visited[MAX_NODES];
    int distances[MAX_NODES];
    int prev[MAX_NODES];
    int pqueue[MAX_NODES];

    for (i = 0; i < MAX_NODES; i++)
    {
        visited[i] = 0;
        distances[i] = INT_MAX;
        prev[i] = -1;
    }
    distances[source] = 0;
    pqueue[++back] = source;

    while (front != back)
    {
        // dequeue the node with minimum distance
        u = pqueue[++front];
        visited[u] = 1;

        graphnode *p = nodes[u];
        while (p != NULL)
        {
            v = p->val;
            if (!visited[v])
            {
                dist = distances[u] + 1;
                if (dist < distances[v])
                {
                    distances[v] = dist;
                    prev[v] = u;
                    pqueue[++back] = v;
                }
            }
            p = p->next;
        }
    }

    ofstream out;
    char filename[20];
    sprintf(filename, "consumer-%d.txt", index);
    out.open(filename, std::ios::app);
    for (int j = 0; j < count; j++)
    {
        int len = 0;

        if (j >= start && j <= end)
        {
            continue;
        }
        int dest = j;
        while (dest != -1)
        {
            path[len++] = dest;
            dest = prev[dest];
        }
        for (i = len - 1; i >= 0; i--)
        {
            out << path[i];
            if (i > 0) {
                out << " -> ";
            }
        }
        out << "\n";
        for (i = 0; i < MAX_NODES; i++)
            path[i] = 0;
    }
    out.close();
    return;
}

int main(int argc, char *argv[])
{
    int shmid_1 = atoi(argv[1]);
    int shmid_2 = atoi(argv[2]);
    int index = atoi(argv[3]);

    graphnode **nodes = (graphnode **)shmat(shmid_1, NULL, 0);
    graphnode *edges = (graphnode *)shmat(shmid_2, NULL, 0);

    // int index = atoi(argv[1]);
    // graphnode **nodes = (graphnode **)malloc(sizeof(graphnode *) * MAX_NODES);
    // graphnode *edges = (graphnode *)malloc(sizeof(graphnode) * MAX_EDGES);
    // memset(nodes, 0, sizeof(graphnode *) * MAX_NODES);
    // memset(edges, 0, sizeof(graphnode) * MAX_EDGES);
    // FILE *fp = fopen("facebook_combined.txt", "r");
    // if (fp == NULL)
    // {
    //     perror("Error in opening file\n");
    //     exit(EXIT_FAILURE);
    // }
    // // string to read the file
    // char str[100];
    // // read the file and build the graph
    // while ((fgets(str, 100, fp)) != NULL)
    // {
    //     // read two nodes representing an edge
    //     int u, v;
    //     sscanf(str, "%d %d", &u, &v);
    //     // add the edge to the graph
    //     // increment the edges pointer by 2 to point to the next edge nodes
    //     edges = add_edge(nodes, edges, u, v);
    // }
    // fclose(fp);

    while (true)
    {
        int count = nodes_count(nodes);
        int size = 0;

        if (count % NUM_CONSUMERS == 0)
        {
            size = count / NUM_CONSUMERS;
        }
        else
        {
            size = (count / NUM_CONSUMERS) + 1;
        }

        int start = index * size;
        int end = start + size - 1;

        if (index == 9)
            end = count - 1;

        for (int i = start; i <= end; i++)
        {
            dijkstra_algo(i, nodes, count, start, end, index);
        }

        sleep(30);
    }

    return 0;
}
