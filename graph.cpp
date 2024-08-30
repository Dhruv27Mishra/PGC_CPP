#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

using namespace std;

void generateRandomGraph(const char* filename, int nodes, int edges) {
    ofstream fout(filename);
    if (!fout) {
        cerr << "Error opening file: " << filename << endl;
        exit(EXIT_FAILURE);
    }

    // Write MatrixMarket header
    fout << "%%MatrixMarket matrix coordinate ";
    fout << "general integer\n";
    fout << "% Comment line\n";

    // Write number of nodes, edges
    fout << nodes << " " << nodes << " " << edges << "\n";

    // Generate random edges
    srand(time(NULL));
    for (int i = 0; i < edges; ++i) {
        int src = rand() % nodes + 1; // Random node from 1 to nodes
        int dst = rand() % nodes + 1; // Random node from 1 to nodes

        fout << src << " " << dst << "\n"; // No weight included
    }

    fout.close();
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " output_file nodes edges\n";
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int nodes = atoi(argv[2]);
    int edges = atoi(argv[3]);

    if (nodes <= 0 || edges <= 0) {
        cerr << "Invalid number of nodes or edges.\n";
        return EXIT_FAILURE;
    }

    generateRandomGraph(filename, nodes, edges);
    cout << "Generated random unweighted graph saved to " << filename << endl;

    return EXIT_SUCCESS;
}

