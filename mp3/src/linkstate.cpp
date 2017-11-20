#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "graph.h"

int main(int argc, char** argv)
{
    if (argc != 4) {
        cout << "Usage: ./linkstate topofile messagefile changesfile" << endl;
        return -1;
    }

    ofstream outfile("output.txt", std::ios::out);
    ifstream topofile (argv[1]);
    ifstream msgfile (argv[2]);
    ifstream changesfile (argv[3]);

    Graph graph(outfile);
    string line;

    /* fill in graph with initial topofile */
    while(getline(topofile, line))
        graph.addElements(line);

    topofile.close();

    /* process changesfile and reprint topology + messages */
    while (true)
    {
        /* run djikstra and print topology */
        graph.linkState();
        graph.printForwardingTableLS();

        /* read message file and send messages*/
        msgfile.clear();
        msgfile.seekg(0, ios::beg);
        graph.sendMessagesLS(msgfile);

        /* stop if all changes have been applied */
        if (changesfile.eof())
            break;

        /* add the next change to the graph */
        line.clear();
        while(line.length() == 0 && !changesfile.eof())
            getline(changesfile, line);

        /* stop if all changes have been applied */
        if (changesfile.eof() && line.length() == 0)
            break;

        graph.addElements(line);
    }

    changesfile.close();
    msgfile.close();
    outfile.close();

    return 0;
}
