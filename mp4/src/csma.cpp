#include "csma.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "Usage: ./csma input" << endl;
        return -1;
    }

    ifstream inputFile (argv[1]);

    string line;

    /* fill in graph with initial topofile */
    while(getline(inputFile, line))
        cout << line << endl;

    inputFile.close();

    return 0;
}
