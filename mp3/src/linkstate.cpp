
#include <iostream>
#include "graph.h"


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
        return -1;
    }

    Graph graph;
    graph.addVertex(1);
    graph.addVertex(2);
    graph.addVertex(4);

    graph.addEdge(1, 4, 1);
    graph.addEdge(1, 2, 8);

    graph.addVertex(5);
    graph.addVertex(3);
    graph.addVertex(6);
    graph.addVertex(7);

   //graph.addEdge(6, 7, 2);
    graph.addEdge(4, 5, 1);
    graph.addEdge(2, 5, 4);
    graph.addEdge(2, 3, 3);

    // for(int i=0; i<8; i++)
    // 	graph.addVertex(i);

    // graph.addEdge(0, 1, 1);
    // graph.addEdge(0, 2, 5);
    // graph.addEdge(1, 2, 8);
    // graph.addEdge(1, 3, 9);
    // graph.addEdge(1, 5, 7);
    // graph.addEdge(2, 6, 5);
    // graph.addEdge(3, 5, 3);
    // graph.addEdge(5, 7, 9);

    graph.print();
    cout << endl;
    for(int i=1; i<8; i++) {
    	graph.printTopology(i); 
    }
	graph.sendMessage(2, 1, "hello");	
	graph.sendMessage(4, 5, "whats");	
	graph.sendMessage(1, 3, "up");	
	graph.sendMessage(3, 4, "yolo");
	graph.sendMessage(6, 3, "mission impossible");
    return 0;
}

