#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

using namespace std;

#define MAX_NODES 10000
#define MAX_EDGES 1000000
#define NUM_CONSUMERS 10

typedef struct graphedge
{
    int src, dest;
} graphedge;

// function to make the graph
map<int, vector<int>> make_graph(graphedge *edges, int *info)
{
    map<int, vector<int>> graph;
    for (int i = 0; i < info[1]; ++i)
    {
        graph[edges[i].src].push_back(edges[i].dest);
        graph[edges[i].dest].push_back(edges[i].src);
    }
    info[0] = graph.size();
    return graph;
}

void update_graph(map<int, vector<int>> &graph, graphedge *edges, int *info, int prev_edges)
{
    // cout << "Graph updated for edge index " << prev_edges << " to " << info[1] << endl;
    for (int i = prev_edges; i < info[1]; ++i)
    {
        graph[edges[i].src].push_back(edges[i].dest);
        graph[edges[i].dest].push_back(edges[i].src);
    }
    info[0] = graph.size();
    return;
}

void dijkstra_algo(int src, map<int, vector<int>> &graph, int index, int *info, int start, int end)
{
    int num_nodes = info[0];
    int num_edges = info[1];

    vector<int> dist(num_nodes, INT_MAX);
    vector<int> pred(num_nodes, -1);
    set<int> visited;
    dist[src] = 0;

    // priority queue for nodes to visit
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.push({0, src});

    while (!pq.empty())
    {
        // get the node with minimum distance
        int node = pq.top().second;
        pq.pop();

        // if we've already visited this node, skip it
        if (visited.count(node))
        {
            continue;
        }

        // mark node as visited
        visited.insert(node);

        // relax edges from this node
        for (int neighbor : graph[node])
        {
            int new_dist = dist[node] + 1;
            if (new_dist < dist[neighbor])
            {
                dist[neighbor] = new_dist;
                pred[neighbor] = node;
                pq.push({new_dist, neighbor});
            }
        }
    }

    // print out the shortest distances and paths to each node
    // cout << "Shortest paths from node " << src << ":\n";
    for (int i = 0; i < num_nodes; i++)
    {
        // cout << "Node " << i << ": ";
    }

    ofstream out;
    char filename[20];
    sprintf(filename, "consumer-%d.txt", index);
    out.open(filename, std::ios::app);
    vector<int> path;
    for (int j = 0; j < num_nodes; j++)
    {
        int len = 0;
        if (j >= start && j <= end)
        {
            continue;
        }
        if (dist[j] != INT_MAX)
        {
            int dest = j;
            while (dest != -1)
            {
                path.push_back(dest);
                dest = pred[dest];
            }
            for (int k = path.size() - 1; k >= 0; k--)
            {
                out << path[k];
                if (k > 0)
                    out << " ";
            }
            out << "\n";
        }
        path.clear();
    }
    out.close();

    return;
}

int main(int argc, char *argv[])
{
    key_t key1 = atoi(argv[1]);
    key_t key2 = atoi(argv[2]);
    int index = atoi(argv[3]);

    printf("Entered consumer - %d\n", index);

    int shmid_1 = shmget(key1, sizeof(graphedge) * MAX_NODES, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("error in shmget1 producer");
        exit(EXIT_FAILURE);
    }
    graphedge *edges = (graphedge *)shmat(shmid_1, NULL, 0);
    if (edges == (graphedge *)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    int shmid_2 = shmget(key2, 2 * sizeof(int), IPC_CREAT | 0666);
    if (shmid_2 == -1)
    {
        perror("error in shmget2 producer");
        exit(EXIT_FAILURE);
    }
    int *info = (int *)shmat(shmid_2, NULL, 0);
    if (info == (int *)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    map<int, vector<int>> graph = make_graph(edges, info);
    int num_nodes = info[0];
    int num_edges = 0;

    int loop_num = 0;

    while (true)
    {
        if (num_edges != info[1])
        {
            loop_num++;
            // cout << "Consumer : " << index << " " << "loop num : " << loop_num << " started" << endl;
            update_graph(graph, edges, info, num_edges);
            num_edges = info[1];
            num_nodes = info[0];

            int size = 0;
            if (num_nodes % NUM_CONSUMERS == 0)
                size = num_nodes / NUM_CONSUMERS;
            else
                size = (num_nodes / NUM_CONSUMERS) + 1;

            int start = index * size;
            int end = start + size - 1;

            if (index == NUM_CONSUMERS - 1)
                end = num_nodes - 1;

            for (int i = start; i <= end; ++i)
                dijkstra_algo(i, graph, index, info, start, end);

            ofstream out;
            char filename[20];
            sprintf(filename, "consumer-%d.txt", index);
            out.open(filename, std::ios::app);
            out << "Completed consumer - " << index << endl;

            // cout << "Consumer : " << index << " " << "loop num : " << loop_num << " ended" << endl;
        }
        sleep(30);
    }

    shmdt(info);
    shmdt(edges);

    return 0;
}
