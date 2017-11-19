#ifndef GRAPH_H
#define GRAPH_H

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>

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
        map<int, bool> ls_known;
        map<int, int> ls_cost;
        map<int, int> ls_predecessor;

        /* distance vector data structures */
        map<int, int> dv_cost;
        map<int, int> dv_next_hop;

        Node(int v);
        void print() const;
    };

    Graph(ostream &);
    ~Graph();
    void addVertex(int v);
    void addEdge(int v1, int v2, int weight);
    void removeEdge(int v1, int v2);
    Node * getNode(int v);
    int getEdge(int v1, int v2) const;

    void addElements(string & line);

    void print() const;
    void printForwardingTableLS() const;
    void printForwardingTableDV() const;

    void sendMessagesLS(ifstream & msgfile) const;
    void sendMessagesDV(ifstream & msgfile) const;

    void linkState();
    void distanceVector();

private:
    void sendMessageLS(int v1, int v2, string & msg) const;
    void sendMessageDV(int v1, int v2, string & msg) const;

    void djikstra(int v);

    map<int, Node*> vertices;
    ostream & outStream;

};

#endif
