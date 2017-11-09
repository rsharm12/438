
#include "graph.h"



void Graph::Node::print() const
{
	cout << "vertex: " << vertex << endl;
	cout << "neighbors: ";
	for(auto it = neighbors.begin(); it != neighbors.end(); it++)
	{
		cout << it->first << "|" << it->second << " ";
	}
	cout << endl;

	cout << "known: ";
	for(auto it = known.begin(); it != known.end(); it++)
	{
		cout << it->first << "|" << it->second << " ";
	}
	cout << endl;	

	cout << "cost: ";
	for(auto it = cost.begin(); it != cost.end(); it++)
	{
		cout << it->first << "|" << it->second << " ";
	}
	cout << endl;	

	cout << "predecessor: ";
	for(auto it = predecessor.begin(); it != predecessor.end(); it++)
	{
		cout << it->first << "|" << it->second << " ";
	}
	cout << endl;
}


Graph::~Graph()
{
	for(auto it = vertices.begin(); it != vertices.end(); it++)
	{
		delete it->second;
	}

}

void Graph::addVertex(int v)
{
	Node* node = new Node(v);
	vertices[v] = node;
}

void Graph::addEdge(int v1, int v2, int weight)
{

	vertices[v1]->neighbors[v2] = weight;
	vertices[v2]->neighbors[v1] = weight;

}

void Graph::removeEdge(int v1, int v2)
{
	vertices[v1]->neighbors.erase(v2);
	vertices[v2]->neighbors.erase(v1);
}

Graph::Node * Graph::getNode(int v)
{
	return vertices[v];
}


void Graph::print() const
{
	for(auto v_it = vertices.begin(); v_it != vertices.end(); v_it++)
	{
		cout << "vertex " << v_it->first << ": ";
		auto neighbors = v_it->second->neighbors;
		for(auto n_it = neighbors.begin(); n_it != neighbors.end(); n_it++)
		{
			cout << n_it->first << "|" << n_it->second << " ";
		}
		cout << endl;
	}
}


void Graph::djikstra(int v)
{

	Node * node = vertices[v];

	for(auto v_it = vertices.begin(); v_it != vertices.end(); v_it++)
	{
		int vertex = v_it->first;
		if(v == vertex) 
		{
			node->known[vertex] = true;
			node->cost[vertex] = 0;
		}
		else 
		{
			node->cost[vertex] = numeric_limits<int>::max();
			node->known[vertex] = false;
		}

		node->predecessor[vertex] = -1;
	}

	int currNode = v;

	int i = 0;
	while(true)
	{
		cout << "currNode=" << currNode << endl;
		auto neighbors = vertices[currNode]->neighbors;
		for(auto n_it = neighbors.begin(); n_it != neighbors.end(); n_it++)
		{
			/* current cost is last known path cost to get to that neighbor from the start */
			int currCost = node->cost[n_it->first];
			/* potential cost is path cost to current node from start + edge weight to neighbor */
			int potentialCost = node->cost[currNode] + n_it->second;
			if(potentialCost < currCost)
				node->cost[n_it->first] = potentialCost;
			cout << "neighbor=" << n_it->first << " currCost=" << currCost << " potentialCost=" << potentialCost << endl;
		}

		/* select next unknown node based on minimum cost */
		int currLowestNode = -1;
		int currLowestCost = numeric_limits<int>::max();
		for(auto v_it = vertices.begin(); v_it != vertices.end(); v_it++)
		{
			bool known = node->known[v_it->first];
			int cost = node->cost[v_it->first];
			cout << "vertex=" << v_it->first << " known=" << known << " cost=" << cost << endl;
			if(!known && cost < currLowestCost)
			{
				currLowestCost = cost;
				currLowestNode = v_it->first;
			}
		}


		if(currLowestNode == -1)
			break;

		currNode = currLowestNode;
		node->known[currNode] = true;
		i++;
	}

}