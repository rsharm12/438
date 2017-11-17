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

    cout << "dv_cost: ";
    for(auto it = dv_cost.begin(); it != dv_cost.end(); it++)
    {
        cout << it->first << "|" << it->second << " ";
    }
    cout << endl;

    cout << "dv_next_hop: ";
    for(auto it = dv_next_hop.begin(); it != dv_next_hop.end(); it++)
    {
        cout << it->first << "|" << it->second << " ";
    }
    cout << endl;

    // cout << "known: ";
    // for(auto it = known.begin(); it != known.end(); it++)
    // {
    //     cout << it->first << "|" << it->second << " ";
    // }
    // cout << endl;

    // cout << "cost: ";
    // for(auto it = cost.begin(); it != cost.end(); it++)
    // {
    //     cout << it->first << "|" << it->second << " ";
    // }
    // cout << endl;

    // cout << "predecessor: ";
    // for(auto it = predecessor.begin(); it != predecessor.end(); it++)
    // {
    //     cout << it->first << "|" << it->second << " ";
    // }
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
    if(vertices.find(v) == vertices.end())
        return nullptr;

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

void Graph::printTopology(bool isLinkState)
{
    /* print topology at each vertex */
    for(auto it = vertices.begin(); it != vertices.end(); it++)
    {
        if(isLinkState)
            printTopologyLS(it->first);
        else
            printTopologyDV(it->first);
    }
}

void Graph::printTopologyLS(int start)
{
    /* run djikstra from each vertex */
    djikstra(start);

    for(auto it = vertices[start]->cost.begin(); it != vertices[start]->cost.end(); it++)
    {
        /* ensure vertex is reachable */
        if(vertices[start]->known[it->first])
        {
            int prevNode = it->first;
            while(vertices[start]->predecessor[prevNode] != start)
            {
                if(prevNode == start) break;
                prevNode = vertices[start]->predecessor[prevNode];
            }

            cout << it->first << " " << prevNode << " " << it->second << endl;
        }
    }

    cout << endl;
}

void Graph::printTopologyDV(int vertex)
{

    for(auto it = vertices[vertex]->dv_cost.begin(); it != vertices[vertex]->dv_cost.end(); it++)
    {
        int dest = it->first;
        int next_hop = vertices[vertex]->dv_next_hop[dest];
        int cost = it->second;

        cout << dest << " " << next_hop << " " << cost << endl;
    }

    cout << endl;
}
void Graph::sendMessageLS(int v1, int v2, string msg)
{
    stack<int> s;
    int prevNode = vertices[v1]->predecessor[v2];
    cout << "from " << v1 << " to " << v2 << " cost ";
    if(vertices[v1]->known[v2])
    {
        /* find path from v1 to v2 */
        while(prevNode != vertices[v1]->predecessor[v1])
        {
            s.push(prevNode);
            prevNode = vertices[v1]->predecessor[prevNode];
        }

        cout << vertices[v1]->cost[v2] << " hops ";

        /* reverse predecessors */
        while(!s.empty())
        {
            cout << s.top() << " ";
            s.pop();
        }
    }
    else
    {
        cout << "infinite hops unreachable ";
    }

    cout << "message " << msg << endl;
}

void Graph::sendMessageDV(int v1, int v2, string msg)
{
    cout << "from " << v1 << " to " << v2 << " cost ";
    if(vertices[v1]->dv_cost.find(v2) != vertices[v1]->dv_cost.end())
    {
        /* find path from v1 to v2 */
        cout << vertices[v1]->dv_cost[v2] << " hops ";

        int currNode = v1;
        while(currNode != v2)
        {
            cout << currNode << " ";
            currNode = vertices[currNode]->dv_next_hop[v2];
        }
    }
    else
    {
        cout << "infinite hops unreachable ";
    }

    cout << "message " << msg << endl;
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
        //cout << "currNode=" << currNode << endl;
        auto neighbors = vertices[currNode]->neighbors;
        for(auto n_it = neighbors.begin(); n_it != neighbors.end(); n_it++)
        {
            /* current cost is last known path cost to get to that neighbor from the start */
            int currCost = node->cost[n_it->first];
            /* potential cost is path cost to current node from start + edge weight to neighbor */
            int potentialCost = node->cost[currNode] + n_it->second;
            bool known = node->known[n_it->first];
            if(potentialCost < currCost && !known) {
                node->cost[n_it->first] = potentialCost;
                node->predecessor[n_it->first] = currNode;
            }
            //cout << "neighbor=" << n_it->first << " currCost=" << currCost << " potentialCost=" << potentialCost << endl;
        }

        /* select next unknown node based on minimum cost */
        int currLowestNode = -1;
        int currLowestCost = numeric_limits<int>::max();
        for(auto v_it = vertices.begin(); v_it != vertices.end(); v_it++)
        {
            bool known = node->known[v_it->first];
            int cost = node->cost[v_it->first];
            //cout << "vertex=" << v_it->first << " known=" << known << " cost=" << cost << endl;
            if(!known && cost < currLowestCost)
            {
                currLowestCost = cost;
                currLowestNode = v_it->first;
            }
        }

        if(currLowestNode == -1)
            break;

        //node->predecessor[currLowestNode] = currNode;
        currNode = currLowestNode;
        node->known[currNode] = true;
        i++;
    }
}


void Graph::distanceVector()
{
    /* sender to receiver for each message */
    queue<tuple<int, int>> q;

    /* initialze queue and update inital costs to infinity */
    for(auto it = vertices.begin(); it != vertices.end(); it++)
    {
        int node = it->first;
        Node * currNode = it->second;

        //currNode->dv_cost[node] = 0;
        //currNode->dv_next_hop[node] = node;

        for(auto n_it = currNode->neighbors.begin(); n_it != currNode->neighbors.end(); n_it++)
        {
            currNode->dv_cost[n_it->first] = n_it->second;
            currNode->dv_next_hop[n_it->first] = n_it->first;
            q.push(make_tuple(node, n_it->first));
        }
    }

    while(!q.empty())
    {
        bool shouldBroadcast = false;
        int sender = get<0>(q.front());
        int receiver = get<1>(q.front());
        q.pop();

        Node * senderNode = getNode(sender);
        Node * receiverNode = getNode(receiver);

        for(auto sender_tbl = senderNode->dv_cost.begin(); sender_tbl != senderNode->dv_cost.end(); sender_tbl++)
        {
            int dst = sender_tbl->first; // destination to compare
            /* don't broadcast receivers cost to itself */
            if(dst == receiver) 
                continue;

            int senderToReceiverCost = senderNode->neighbors[receiver]; // get edge weight
            int senderToDstCost = sender_tbl->second;

            /* receiver does not know about dest */
            if(receiverNode->dv_cost.find(dst) == receiverNode->dv_cost.end())
            {
                receiverNode->dv_cost[dst] = senderToReceiverCost + senderToDstCost;
                receiverNode->dv_next_hop[dst] = sender;
                shouldBroadcast = true;
            }
            else
            {
                int receiverToDstCost = receiverNode->dv_cost[dst];
                if(senderToReceiverCost + senderToDstCost < receiverToDstCost)
                {
                    receiverNode->dv_cost[dst] = senderToReceiverCost + senderToDstCost;
                    receiverNode->dv_next_hop[dst] = sender;
                    shouldBroadcast = true;
                }

            }

        }

        /* following changes in dv table broadcast to neighbors */
        if(shouldBroadcast)
        {
            for(auto n_it = receiverNode->neighbors.begin(); n_it != receiverNode->neighbors.end(); n_it++)
            {
                q.push(make_tuple(receiver, n_it->first));
            }

        }

        // cout << sender << "->" << receiver << endl;
        // receiverNode->print();
    }    

}