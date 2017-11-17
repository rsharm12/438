
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "graph.h"

void addElements(Graph * graph, string line)
{
    int v1, v2, weight;
    istringstream iss(line);
    iss >> v1;
    iss >> v2;
    iss >> weight;

    if(graph->getNode(v1) == nullptr)
        graph->addVertex(v1);
    if(graph->getNode(v2) == nullptr)
        graph->addVertex(v2);

    if(weight == -999)
        graph->removeEdge(v1, v2);
    else
        graph->addEdge(v1, v2, weight);
}

void sendMessage(Graph * graph, string filename)
{
    int v1, v2;
    string line;
    string message;

    ifstream messagefile (filename);

    while(getline(messagefile, line))
    {
        istringstream iss(line);
        iss >> v1 >> v2;
        /* strip leading space */
        iss.ignore(1, ' ');
        getline(iss, message);

        graph->sendMessageDV(v1, v2, message);
    }

    messagefile.close();
}

int main(int argc, char** argv)
{
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        cout << "Usage: ./distvec topofile messagefile changesfile" << endl;
        return -1;
    }

    Graph graph;
    string line;

    /* fill in graph with initial topofile */
    ifstream topofile (argv[1]);
    while(getline(topofile, line))
    {
        addElements(&graph, line);
    }

    topofile.close();

    graph.distanceVector();

    /* print topology at each vertex */
    graph.printTopology(false);

    // /* read message file and send messages*/
    sendMessage(&graph, argv[2]);
    cout << endl;

    // /* process changesfile and reprint topology + messages */
    ifstream changesfile (argv[3]);

    while(getline(changesfile, line))
    {
        addElements(&graph, line);
        graph.distanceVector();
        graph.printTopology(false);
        sendMessage(&graph, argv[2]);
        cout << endl;
    }

    changesfile.close();

    return 0;
}

