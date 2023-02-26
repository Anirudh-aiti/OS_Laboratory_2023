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

typedef struct graphnode
{
    int val;
    struct graphnode *next;
} graphnode;

#define MAX_NODES 10000
#define MAX_EDGES 1000000

void print_graph(graphnode **nodes, int size)
{
    // print to file
    ofstream myfile("out.txt");
    streambuf *coutbuf = cout.rdbuf(); // save old buf
    cout.rdbuf(myfile.rdbuf());        // redirect std::cout to out.txt!

    for (int i = 0; i < size; ++i){
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
        else
            break;
    }
    cout.rdbuf(coutbuf);
}

// function to add an edge to the nodes 2d array
graphnode *add_edge(graphnode **nodes, graphnode *edges, int u, int v)
{
    // add the edge to the graph
    //cout<<"\nadding edge\n";
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
    cout<<"\nadded edge\n";
}

int rand_gen(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

// function to get the degree of a node
int degree(graphnode **nodes, int u)
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

bool compare(const pair<int, int>& a, const pair<int, int>& b) {
    return a.first > b.first; // sort in descending order based on the first element
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    printf("/*********key: %s, key2: %s*******/\n", argv[1], argv[2]);

    key_t key = atoi(argv[1]);
    key_t key2 = atoi(argv[2]);

    int shmid_1 = shmget(key, sizeof(graphnode *) * MAX_NODES, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("error in shmget1 producer");
        exit(EXIT_FAILURE);
    }

    // attach the shared memory
    graphnode **nodes = (graphnode **)shmat(shmid_1, NULL, 0);
    if (nodes == (graphnode **)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    int shmid_2 = shmget(key2, sizeof(graphnode) * MAX_EDGES, IPC_CREAT | 0666);
    if (shmid_2 == -1)
    {
        perror("error in shmget2 producer");
        exit(EXIT_FAILURE);
    }
    graphnode *edges = (graphnode *)shmat(shmid_2, NULL, 0);
    if (nodes == (graphnode **)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    // testing by printing the file
    // print_graph(nodes, MAX_NODES);

    // generate random number (m)
    // add that number  of nodes

    int m = rand_gen(10, 30);
    cout << "m:" << m << endl;

    int idx; // first NULL node*
    for (int i = 0; i < MAX_NODES; i++)
    {
        if (nodes[i] == NULL)
        {
            idx = i;
            break;
        }
    }

    cout<<"idx:"<<idx<<endl;
    // get top k degree nodes

    int dgre [MAX_NODES] = {0};
    vector <pair <int, int>> v;
    for (int i = 0; i < idx; i++)
    {
        dgre[i] = degree(nodes, i);
        //cout<< "<"<<dgre[i]<<","<<i<<">"<<"\t";
        v.push_back(make_pair(dgre[i], i)); // create a pair of original value and index
    }
    sort(v.begin(),v.end(),compare);

    std::cout << "Sorted array in descending order: ";
    for (auto& p : v) {
        std::cout << p.first << " ";
    }
    std::cout << std::endl;

    std::cout << "Indices of sorted array: ";
    for (auto& p : v) {
        std::cout << p.second << " ";
    }
    std::cout << std::endl;

    cout<< "\nhoi\n";
    cout<< v[0].first << "," << v[0].second << endl;
    cout<<"\nhi\n";
    cout<<edges<<endl;

    
    m = 1;
    while(edges != (graphnode*)0) edges += 1;

    cout<<"\nhi2\n"<<endl;
    cout<<edges<<endl;

    for (int i = 0; i < m; i++)
    {
        int k = rand_gen(1, 20);
        cout << "k: " << k << endl;
        k =1;
        // add k edges to the node
        for(int j = 0; j < k ;j++){
            cout<<"<"<<idx<<", "<<v[j].second<<">";
            edges = add_edge(nodes, edges , idx, v[j].second);
        }
        cout<<endl;
        idx++;
    }

    print_graph(nodes,MAX_NODES);
    

    shmdt(nodes);
    shmdt(edges);
    return 0;
}