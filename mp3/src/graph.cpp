#include "graph.h"

Graph::Node::Node(int v) : vertex(v)
{
}

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
    // cout << endl;
}

Graph::Graph(ostream & output_stream)
    : outStream(output_stream)
{
}

Graph::~Graph()
{
    for(auto& vertex : vertices)
    {
        delete vertex.second;
    }
    vertices.clear();
}

void Graph::addVertex(int v)
{
    Node* node = new Node(v);
    vertices[v] = node;
}

void Graph::addEdge(int v1, int v2, int weight)
{
    vertices.at(v1)->neighbors[v2] = weight;
    vertices.at(v2)->neighbors[v1] = weight;
}

void Graph::removeEdge(int v1, int v2)
{
    vertices.at(v1)->neighbors.erase(v2);
    vertices.at(v2)->neighbors.erase(v1);
}

Graph::Node * Graph::getNode(int v)
{
    if(vertices.count(v))
        return vertices.at(v);

    return nullptr;
}

int Graph::getEdge(int v1, int v2) const
{
    return vertices.at(v1)->neighbors.at(v2);
}

void Graph::addElements(string & line)
{
    int v1, v2, weight;
    istringstream iss(line);
    iss >> v1;
    iss >> v2;
    iss >> weight;

    if(getNode(v1) == nullptr)
        addVertex(v1);
    if(getNode(v2) == nullptr)
        addVertex(v2);

    if(weight == -999)
        removeEdge(v1, v2);
    else
        addEdge(v1, v2, weight);
}

void Graph::print() const
{
    for(const auto& vertex : vertices)
    {
        cout << "vertex " << vertex.first << ": ";
        for(const auto& neighbor : vertex.second->neighbors)
        {
            cout << neighbor.first << "|" << neighbor.second << " ";
        }
        cout << endl;
    }
}

void Graph::printForwardingTableLS() const
{
    for(const auto& vertex : vertices)
    {
        int start = vertex.first;
        const Node * start_node = vertex.second;

        for(const auto& cost : start_node->ls_cost)
        {
            /* since LS stores predecessor only, we need to loop
             * until we find the start node */

            int dest = cost.first;
            int path_cost = cost.second;

            /* ensure destination is reachable */
            if(start_node->ls_known.at(dest))
            {
                int nextHop = dest;

                if(dest != start)
                {
                    while(start_node->ls_predecessor.at(nextHop) != start)
                    {
                        if(nextHop == start)
                            break;
                        nextHop = start_node->ls_predecessor.at(nextHop);
                    }
                }

                outStream << dest << " " << nextHop << " " << path_cost << endl;
            }
        }

        outStream << endl;
    }
}

void Graph::printForwardingTableDV() const
{
    for(const auto& vertex : vertices)
    {
        // int start = vertex.first;
        const Node * start_node = vertex.second;

        for(const auto& cost : start_node->dv_cost)
        {
            int dest = cost.first;
            int path_cost = cost.second;
            int next_hop = start_node->dv_next_hop.at(dest);

            outStream << dest << " " << next_hop << " " << path_cost << endl;
        }

        outStream << endl;
    }
}

void Graph::sendMessagesLS(ifstream & msgfile) const
{
    int v1, v2;
    string line;
    string message;

    while(getline(msgfile, line))
    {
        istringstream iss(line);
        iss >> v1 >> v2;
        /* strip leading space */
        iss.ignore(1, ' ');
        getline(iss, message);

        sendMessageLS(v1, v2, message);
    }

    outStream << endl;
}

void Graph::sendMessageLS(int v1, int v2, string & msg) const
{
    stack<int> s;
    const Node * v1_node = vertices.at(v1);

    outStream << "from " << v1 << " to " << v2 << " ";

    /* check if v2 is reachable */
    if(v1_node->ls_known.at(v2))
    {
        /* find path from v1 to v2 */
        int prevNode = v2;
        while(prevNode != v1)
        {
            prevNode = v1_node->ls_predecessor.at(prevNode);
            s.push(prevNode);
        }

        outStream << "cost " << v1_node->ls_cost.at(v2) << " ";

        /* reverse predecessors to print hops*/
        outStream << "hops ";
        while(!s.empty())
        {
            outStream << s.top() << " ";
            s.pop();
        }
    }
    else
    {
        outStream << "cost infinite hops unreachable ";
    }

    outStream << "message " << msg << endl;
}

void Graph::sendMessagesDV(ifstream & msgfile) const
{
    int v1, v2;
    string line;
    string message;

    while(getline(msgfile, line))
    {
        istringstream iss(line);
        iss >> v1 >> v2;
        /* strip leading space */
        iss.ignore(1, ' ');
        getline(iss, message);

        sendMessageDV(v1, v2, message);
    }

    outStream << endl;
}

void Graph::sendMessageDV(int v1, int v2, string & msg) const
{
    const Node * v1_node = vertices.at(v1);

    outStream << "from " << v1 << " to " << v2 << " ";

    /* check if v2 is reachable */
    if(v1_node->dv_cost.count(v2))
    {
        /* find path from v1 to v2 */
        outStream << "cost " << v1_node->dv_cost.at(v2) << " ";

        outStream << "hops ";
        int currNode = v1;
        while(currNode != v2)
        {
            outStream << currNode << " ";
            currNode = vertices.at(currNode)->dv_next_hop.at(v2);
        }
    }
    else
    {
        outStream << "cost infinite hops unreachable ";
    }

    outStream << "message " << msg << endl;
}

void Graph::linkState()
{
    for(const auto& vertex : vertices)
    {
        /* initialize the djikstra data structures */
        for(const auto& others : vertices)
        {
            vertex.second->ls_predecessor[others.first] = -1;
            if(vertex.first == others.first)
            {
                vertex.second->ls_known[others.first] = true;
                vertex.second->ls_cost[others.first] = 0;
            }
            else
            {
                vertex.second->ls_known[others.first] = false;
                vertex.second->ls_cost[others.first] = numeric_limits<int>::max();
            }
        }

        /* run djikstra on the node */
        djikstra(vertex.first);
    }
}

void Graph::djikstra(int v)
{
    Node * node = vertices[v];
    int currNode = v;

    while(true)
    {
        for(const auto& neighbor : vertices[currNode]->neighbors)
        {
            /* current cost is last known path cost to get to that neighbor from the start */
            int currCost = node->ls_cost[neighbor.first];
            /* potential cost is path cost to current node from start + edge weight to neighbor */
            int potentialCost = node->ls_cost[currNode] + neighbor.second;
            /* known is whether we have seen this node before */
            bool known = node->ls_known[neighbor.first];

            if(potentialCost < currCost && !known)
            {
                node->ls_cost[neighbor.first] = potentialCost;
                node->ls_predecessor[neighbor.first] = currNode;
            }
            else if(potentialCost == currCost && !known)
            {

            }
        }

        /* select next unknown node based on minimum cost */
        int currLowestNode = -1;
        int currLowestCost = numeric_limits<int>::max();
        for(const auto& vertex : vertices)
        {
            bool known = node->ls_known[vertex.first];
            int cost = node->ls_cost[vertex.first];

            if(!known && cost < currLowestCost)
            {
                currLowestCost = cost;
                currLowestNode = vertex.first;
            }
            else if(!known && cost == currLowestCost)
            {
                /* tie breaker, choose node with lower ID */
                if(vertex.first < currLowestNode)
                {
                    currLowestCost = cost;
                    currLowestNode = vertex.first;
                }
            }
        }

        if(currLowestNode == -1)
            break;

        currNode = currLowestNode;
        node->ls_known[currNode] = true;
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

        currNode->dv_cost.clear();
        currNode->dv_next_hop.clear();

        currNode->dv_cost[node] = 0;
        currNode->dv_next_hop[node] = node;

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
