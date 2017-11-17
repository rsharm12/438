
#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <stack>
#include <queue>
#include <tuple>
#include <fstream>

using namespace std;

class Graph
{

public:
    struct Node
    {
        int vertex;

        /* store vertex number mapped to edge weight of neighbors */
        map<int, int> neighbors;

        /* djikstra's data structures */
        map<int, bool> known;
        map<int, int> cost;
        map<int, int> predecessor;

        /* distance vector data structures */
        map<int, int> dv_cost;
        map<int, int> dv_next_hop;


        Node(int v)
        {
            vertex = v;
        }
        void print() const;
    };

    ~Graph();
    void addVertex(int v);
    void addEdge(int v1, int v2, int weight);
    void removeEdge(int v1, int v2);
    Node * getNode(int v);

    void print() const;
    void printTopology(bool isLinkState);

    void sendMessageLS(int v1, int v2, string msg);
    void sendMessageDV(int v1, int v2, string msg);

    void djikstra(int v);
    void distanceVector();

    ofstream fout;

private:

    void printTopologyLS(int start);
    void printTopologyDV(int vertex);

    unordered_map<int, Node*> vertices;

};



#endif

