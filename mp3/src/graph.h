
#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <iostream>
#include <limits>


using namespace std;

class Graph 
{

public:
	struct Node 
	{
		int vertex;
		/* store vertex number mapped to edge weight of neighbors */
		unordered_map<int, int> neighbors;
		/* djikstra's data structures */	
		unordered_map<int, bool> known;
		unordered_map<int, int> cost;
		unordered_map<int, int> predecessor;
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

	void djikstra(int v);


private:

	unordered_map<int, Node*> vertices;

};



#endif

