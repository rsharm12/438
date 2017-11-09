
#include <iostream>
#include "graph.h"


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
        return -1;
    }

    Graph graph;
    // graph.addVertex(1);
    // graph.addVertex(2);
    // graph.addVertex(4);

    // graph.addEdge(1, 4, 1);
    // graph.addEdge(1, 2, 8);

    // graph.addVertex(5);
    // graph.addVertex(3);

    // graph.addEdge(4, 5, 1);
    // graph.addEdge(2, 5, 4);
    // graph.addEdge(2, 3, 3);

    for(int i=0; i<8; i++)
    	graph.addVertex(i);

    graph.addEdge(0, 1, 1);
    graph.addEdge(0, 2, 5);
    graph.addEdge(1, 2, 8);
    graph.addEdge(1, 3, 9);
    graph.addEdge(1, 5, 7);
    graph.addEdge(2, 6, 5);
    graph.addEdge(3, 5, 3);
    graph.addEdge(5, 7, 9);

    graph.print();

    graph.djikstra(3);
    graph.getNode(3)->print();

    return 0;
}

