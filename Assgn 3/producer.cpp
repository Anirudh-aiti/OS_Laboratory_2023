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
typedef struct graphedge
{
    int src, dest;
} graphedge;

#define NUM_CONSUMERS 10
#define MAX_NODES 10000
#define MAX_EDGES 500000

// global variable to keep track of the current number of edges in the graph
int prev_edges=0;

// function to print the graph in "prod.txt"
void print_graph(map <int, vector <int>> m)
{
    ofstream out("prod.txt");
    for (auto i = m.begin(); i != m.end(); ++i)
    {
        out << i->first << " : ";
        for (auto j = i->second.begin(); j != i->second.end(); ++j) out << *j << " ";
        out << endl;
    }
    out.close();
}

// function to make the graph
map <int, vector <int>> make_graph(graphedge * edges, int * info)
{
    map <int, vector <int>> graph;
    for (int i = 0; i < info[1]; ++i)
    {
        graph[edges[i].src].push_back(edges[i].dest);
        graph[edges[i].dest].push_back(edges[i].src);
    }
    info[0] = graph.size();
    return graph;
}

// update graph :: update the adjacency lists of the graph only for the new edges added
void update_graph(map <int, vector <int>> &graph, graphedge * edges, int * info)
{     
    cout << "Graph updated for edge index " << prev_edges << " to " << info[1] << endl;
    for (int i = prev_edges; i < info[1]; ++i)
    {
        graph[edges[i].src].push_back(edges[i].dest);
        graph[edges[i].dest].push_back(edges[i].src);
    }
    info[0] = graph.size();
    prev_edges = info[1];
}


// function to add an edge to the graph in the shared memory
void add_edge(graphedge * edges, int u, int v, int *info)
{
    // add the edge to the graph
    graphedge a;
    a.src = u;
    a.dest = v;
    edges[info[1]] = a;
    info[1]++;
}

// function to generate a random number between min and max (both inclusive)
int rand_gen(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

// comparator function to sort the map
bool compare(const pair<int, vector<int>>& a, const pair<int, vector<int>>& b) {
    return a.second.size() > b.second.size();
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    printf("/********* Key 1: %s, Key 2: %s *******/\n", argv[1], argv[2]);

    key_t key1 = atoi(argv[1]);
    key_t key2 = atoi(argv[2]);

    int shmid_1 = shmget(key1, sizeof(graphedge) * MAX_NODES, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("error in shmget1 producer");
        exit(EXIT_FAILURE);
    }
    // attach the shared memory
    graphedge * edges = (graphedge *)shmat(shmid_1, NULL, 0);
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
    int *info = (int*)shmat(shmid_2, NULL, 0);
    if (info == (int *)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    map <int, vector <int>> graph;

    // make the graph
    update_graph(graph, edges, info);

    while(1)
    {
        
        // print info
        cout << "Producer:: Number of nodes: " << info[0] << endl;
        cout << "Producer:: Number of edges: " << info[1] << endl;
        
        //generate random number m : number of new nodes to be added
        int m = rand_gen(10,30);
        cout<< "m: "<< m << endl;

        for(int i=0; i < m ; i++ )
        {
            // generate a random number k between 1 and 20 
            // and index of the new node
            int k = rand_gen(1,20), new_node = info[0];
            cout<<"k: "<<k<<endl;

            // get the cumulative degree array
            vector <int> cum_dgre;
            cum_dgre.push_back(graph[0].size());

            for (int i=1; i < info[0] ; i++)
            {
                cum_dgre.push_back(graph[i].size() + cum_dgre[i-1]);
            }
            
            // map to keep track of the nodes already having an edge with the new node
            map <int, bool> added;
            for (int j = 0; j < k; j++)
            {
                // next integer in the cumulative degree array
                int prob_int = rand_gen(1, cum_dgre[info[0]-1] - 1); 
                int next_int = upper_bound(cum_dgre.begin(), cum_dgre.end(), prob_int) - cum_dgre.begin();
                
                // if the node was already added, continue
                if (added[next_int]) continue;
                
                // add the new edge to the graph
                add_edge(edges, new_node, next_int, info);
                added[next_int] = true;

                cout << ">> New edge added :: " << new_node << " - " << next_int << endl;
            }
            
            info[0] += 1;
            // update the graph
            update_graph(graph, edges, info);
        }

        cout << "Producer:: Number of nodes after updation : " << info[0] << endl;

        // make the graph
        update_graph(graph, edges, info);

        // print the graph
        print_graph(graph);
        sleep(50);
    }

    shmdt(info);
    shmdt(edges);
    return 0;
}