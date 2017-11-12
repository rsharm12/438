
#ifndef GRAPH_H
#define GRAPH_H

#include <unordered_map>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <stack>

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
	void printTopology(int start);
	void sendMessage(int v1, int v2, string msg);

	void djikstra(int v);

	
	unordered_map<int, Node*> vertices;


private:

};



#endif

