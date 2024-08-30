#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_NODES 36692

// Structure to represent an adjacency list node
struct AdjListNode {
    int dest;
    struct AdjListNode* next;
};

// Structure to represent an adjacency list
struct AdjList {
    struct AdjListNode* head;
};

// Structure to represent a graph
struct Graph {
    int V;
    struct AdjList* array;
};

// Function to create a new adjacency list node
struct AdjListNode* newAdjListNode(int dest) {
    struct AdjListNode* newNode = (struct AdjListNode*) malloc(sizeof(struct AdjListNode));
    newNode->dest = dest;
    newNode->next = NULL;
    return newNode;
}

// Function to create a graph of V vertices
struct Graph* createGraph(int V) {
    struct Graph* graph = (struct Graph*) malloc(sizeof(struct Graph));
    graph->V = V;

    // Create an array of adjacency lists. Size of array will be V
    graph->array = (struct AdjList*) malloc(V * sizeof(struct AdjList));

    // Initialize each adjacency list as empty by making head NULL
    for (int i = 0; i < V; ++i) {
        graph->array[i].head = NULL;
    }

    return graph;
}

// Function to add an edge to an undirected graph
void addEdge(struct Graph* graph, int src, int dest) {
    // Add an edge from src to dest. A new node is added to the adjacency list of src.
    struct AdjListNode* newNode = newAdjListNode(dest);
    newNode->next = graph->array[src].head;
    graph->array[src].head = newNode;

    // Since graph is undirected, add an edge from dest to src also
    newNode = newAdjListNode(src);
    newNode->next = graph->array[dest].head;
    graph->array[dest].head = newNode;
}

// Function to read the graph data from file
void readGraph(const char* filename, struct Graph* graph) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comment lines
        if (line[0] == '#') {
            continue;
        }

        int src, dest;
        if (sscanf(line, "%d\t%d", &src, &dest) == 2) {
            addEdge(graph, src, dest);
        }
    }

    fclose(file);
}

// Function to perform graph coloring and return the number of colors used
int graphColoring(struct Graph* graph) {
    int V = graph->V;
    int* result = (int*) malloc(V * sizeof(int));
    int* available = (int*) malloc(V * sizeof(int));

    // Initialize all vertices as unassigned
    for (int u = 0; u < V; u++) {
        result[u] = -1;
    }

    // Initialize all colors as available
    for (int cr = 0; cr < V; cr++) {
        available[cr] = 0;
    }

    // Assign the first color to the first vertex
    result[0] = 0;

    // Assign colors to remaining V-1 vertices
    for (int u = 1; u < V; u++) {
        // Process all adjacent vertices and mark their colors as unavailable
        struct AdjListNode* adj = graph->array[u].head;
        while (adj != NULL) {
            if (result[adj->dest] != -1) {
                available[result[adj->dest]] = 1;
            }
            adj = adj->next;
        }

        // Find the first available color
        int cr;
        for (cr = 0; cr < V; cr++) {
            if (available[cr] == 0) {
                break;
            }
        }

        result[u] = cr;

        // Reset the values back to false for the next iteration
        adj = graph->array[u].head;
        while (adj != NULL) {
            if (result[adj->dest] != -1) {
                available[result[adj->dest]] = 0;
            }
            adj = adj->next;
        }
    }

    // Find the number of colors used
    int num_colors = 0;
    for (int u = 0; u < V; u++) {
        if (result[u] > num_colors) {
            num_colors = result[u];
        }
    }
    num_colors++;

    free(result);
    free(available);

    return num_colors;
}

// Function to calculate elapsed time in seconds
double getElapsedTime(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
}

int main() {
    const char* filename = "E.txt";
    int num_nodes = MAX_NODES;  // Known number of nodes

    // Create the graph
    struct Graph* graph = createGraph(num_nodes);

    // Read graph data
    readGraph(filename, graph);

    // Measure execution time using gettimeofday
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int num_colors = graphColoring(graph);
    
    gettimeofday(&end, NULL);

    double cpu_time_used = getElapsedTime(start, end);

    // Output results
    printf("Time taken: %f seconds\n", cpu_time_used);
    printf("Number of colors used: %d\n", num_colors);

    // Clean up
    for (int i = 0; i < num_nodes; i++) {
        struct AdjListNode* adj = graph->array[i].head;
        while (adj != NULL) {
            struct AdjListNode* temp = adj;
            adj = adj->next;
            free(temp);
        }
    }
    free(graph->array);
    free(graph);

    return 0;
}
