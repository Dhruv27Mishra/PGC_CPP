#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <utility>
#include <algorithm>
#include <ctime>

// Comparator function for qsort to sort edges
int compare_edges(const void* a, const void* b) {
    int* edge1 = (int*)a;
    int* edge2 = (int*)b;
    if (edge1[0] == edge2[0]) {
        return edge1[1] - edge2[1];
    }
    return edge1[0] - edge2[0];
}

void generate_undirected_graph(int num_nodes, int num_edges, int** edges, int* edge_count) {
    *edge_count = 0;
    int max_edges = num_edges * 2; // Each edge will be stored twice for undirected graph

    // Allocate memory for edges
    *edges = (int*)malloc(max_edges * 2 * sizeof(int));

    if (*edges == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    int* edge_exists = (int*)calloc(num_nodes * num_nodes, sizeof(int));

    if (edge_exists == NULL) {
        printf("Memory allocation failed\n");
        free(*edges);
        exit(1);
    }

    srand(time(NULL));

    while (*edge_count < num_edges) {
        int u = rand() % num_nodes;
        int v = rand() % num_nodes;
        if (u != v && edge_exists[u * num_nodes + v] == 0) {
            // Add edge u -> v
            (*edges)[(*edge_count) * 2] = u;
            (*edges)[(*edge_count) * 2 + 1] = v;
            (*edge_count)++;
            // Add edge v -> u
            (*edges)[(*edge_count) * 2] = v;
            (*edges)[(*edge_count) * 2 + 1] = u;
            (*edge_count)++;
            edge_exists[u * num_nodes + v] = 1;
            edge_exists[v * num_nodes + u] = 1;
        }
    }

    free(edge_exists);
}

void save_graph_to_txt(int* edges, int edge_count, const char* filename, int num_nodes) {
    // Sort edges
    qsort(edges, edge_count, 2 * sizeof(int), compare_edges);

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Unable to open file %s\n", filename);
        exit(1);
    }

    fprintf(file, "# Directed graph (each unordered pair of nodes is saved twice, once in each direction): E.txt\n");
    fprintf(file, "# Enron email network (edge indicated that email was exchanged, undirected edges)\n");
    fprintf(file, "# Nodes: %d Edges: %d\n", num_nodes, edge_count);
    fprintf(file, "# FromNodeId\tToNodeId\n");

    for (int i = 0; i < edge_count; i++) {
        fprintf(file, "%d\t%d\n", edges[i * 2], edges[i * 2 + 1]);
    }

    fclose(file);
}

extern "C" {
    #include "ECLgraph.h"
}

int convert_snap_to_ecl(const char* input_filename, const char* output_filename) {
    // Record the start time
    clock_t start_time = clock();

    printf("SNAP to ECL Graph Converter\n");
    printf("Copyright 2016 Texas State University\n");

    FILE* fin = fopen(input_filename, "rt");
    if (fin == NULL) {
        fprintf(stderr, "ERROR: could not open input file %s\n\n", input_filename);
        return -1;
    }

    char line[256], word1[16], word2[16], word3[16];
    char* ptr = line;
    size_t linesize = 256;
    int cnt;
    do {
        cnt = getline(&ptr, &linesize, fin);
    } while ((cnt > 0) && (strstr(line, "Nodes:") == 0));
    if (cnt <= 0) {
        fprintf(stderr, "ERROR: could not find line with node and edge counts\n\n");
        return -1;
    }
    if ((strstr(line, "Nodes:") == 0) || (strstr(line, "Edges:") == 0)) {
        fprintf(stderr, "ERROR: could not find line with node and edge counts\n\n");
        return -1;
    }

    int nodes = -1, edges = -1;
    cnt = sscanf(line, "%s %s %d %s %d", word1, word2, &nodes, word3, &edges);
    if ((cnt != 5) || (nodes < 1) || (edges < 0) || (strcmp(word1, "#") != 0) || (strcmp(word2, "Nodes:") != 0) || (strcmp(word3, "Edges:") != 0)) {
        fprintf(stderr, "ERROR: failed to parse nodes and edge counts\n\n");
        return -1;
    }

    printf("%s\t#name\n", input_filename);
    printf("%d\t#nodes\n", nodes);
    printf("%d\t#edges\n", edges);
    printf("no\t#weights\n");

    while (((cnt = getline(&ptr, &linesize, fin)) > 0) && (strstr(line, "#") != 0)) {}
    if (cnt < 3) {
        fprintf(stderr, "ERROR: could not find non-comment line\n\n");
        return -1;
    }

    ECLgraph g;
    g.nodes = nodes;
    g.edges = edges;
    g.nindex = (int*)calloc(nodes + 1, sizeof(int));
    g.nlist = (int*)malloc(edges * sizeof(int));
    g.eweight = NULL;
    if ((g.nindex == NULL) || (g.nlist == NULL)) {
        fprintf(stderr, "ERROR: memory allocation failed\n\n");
        return -1;
    }

    int src, dst;
    std::vector<std::pair<int, int>> v;
    cnt = 0;
    if (sscanf(line, "%d %d", &src, &dst) == 2) {
        cnt++;
        if ((src < 0) || (src >= nodes)) {
            fprintf(stderr, "ERROR: source out of range\n\n");
            return -1;
        }
        if ((dst < 0) || (dst >= nodes)) {
            fprintf(stderr, "ERROR: source out of range\n\n");
            return -1;
        }
        v.push_back(std::make_pair(src, dst));
    }
    while (fscanf(fin, "%d %d", &src, &dst) == 2) {
        cnt++;
        if ((src < 0) || (src >= nodes)) {
            fprintf(stderr, "ERROR: source out of range\n\n");
            return -1;
        }
        if ((dst < 0) || (dst >= nodes)) {
            fprintf(stderr, "ERROR: source out of range\n\n");
            return -1;
        }
        v.push_back(std::make_pair(src, dst));
    }
    fclose(fin);
    if (cnt != edges) {
        fprintf(stderr, "ERROR: failed to read correct number of edges\n\n");
        return -1;
    }

    std::sort(v.begin(), v.end());

    g.nindex[0] = 0;
    for (int i = 0; i < edges; i++) {
        int src = v[i].first;
        int dst = v[i].second;
        g.nindex[src + 1] = i + 1;
        g.nlist[i] = dst;
    }

    for (int i = 1; i < (nodes + 1); i++) {
        g.nindex[i] = std::max(g.nindex[i - 1], g.nindex[i]);
    }

    writeECLgraph(g, output_filename);
    freeECLgraph(g);

    // Record the end time
    clock_t end_time = clock();
    // Calculate and print the total time taken
    double total_time = double(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Total time taken: %.2f seconds\n", total_time);

    return 0;
}

int main() {
    int num_nodes, num_edges;

    printf("Enter the number of nodes: ");
    scanf("%d", &num_nodes);
    printf("Enter the number of edges: ");
    scanf("%d", &num_edges);

    int* edges;
    int edge_count;
    generate_undirected_graph(num_nodes, num_edges, &edges, &edge_count);
    save_graph_to_txt(edges, edge_count, "E.txt", num_nodes);

    printf("Graph with %d nodes and %d edges has been saved to 'E.txt'.\n", num_nodes, edge_count);

    free(edges);

    // Convert the generated graph to ECLgraph format
    const char* input_filename = "E.txt";
    const char* output_filename = "ECLgraph.egr";
    convert_snap_to_ecl(input_filename, output_filename);

    return 0;
}

