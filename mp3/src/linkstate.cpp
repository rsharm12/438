
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

	if(graph->vertices.find(v1) == graph->vertices.end())
	 	graph->addVertex(v1);
	if(graph->vertices.find(v2) == graph->vertices.end())
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

		graph->sendMessage(v1, v2, message);
		//cout << v1 << " " << v2 << message << endl;
	}

	messagefile.close();
}

void printTopology(Graph * graph)
{
	/* print topology at each vertex */
    //graph->print();
    for(auto it = graph->vertices.begin(); it != graph->vertices.end(); it++)
    {
     	graph->printTopology(it->first);
    }
}

int main(int argc, char** argv) 
{
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
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

	/* print topology at each vertex */
	printTopology(&graph);

    /* read message file and send messages*/
	sendMessage(&graph, argv[2]);
	cout << endl;

	/* process changesfile and reprint topology + messages */
	ifstream changesfile (argv[3]);

	while(getline(changesfile, line))
	{
		addElements(&graph, line);
		printTopology(&graph);
		sendMessage(&graph, argv[2]);
		cout << endl;
	}

	changesfile.close();

    return 0;
}

