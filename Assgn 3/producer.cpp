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
    b.next = NULL;
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


int main(){
    
    return 0;
}