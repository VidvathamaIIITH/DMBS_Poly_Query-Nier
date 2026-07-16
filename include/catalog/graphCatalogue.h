#include "graph.h"
#include <unordered_map>

class GraphCatalogue
{
    unordered_map<string, Graph*> graphs;

public:
    GraphCatalogue() {}
    
    void insertGraph(Graph* graph)
    {
        graphs[graph->name] = graph;
    }

    void deleteGraph(string graphName)
    {
        if (graphs.count(graphName)) {
            delete graphs[graphName];
            graphs.erase(graphName);
        }
    }

    Graph* getGraph(string graphName)
    {
        if (graphs.count(graphName))
            return graphs[graphName];
        return nullptr;
    }

    bool isGraph(string graphName)
    {
        return graphs.count(graphName);
    }

    // vidvathamaiiith extension: enumerate every loaded graph for `LIST GRAPHS`.
    void print()
    {
        cout << "\nGRAPHS" << endl;
        int count = 0;
        for (auto pair : graphs)
        {
            cout << pair.first << (pair.second->directed ? " (D)" : " (U)") << endl;
            count++;
        }
        cout << "\n\nRow Count: " << count << endl;
    }

    void clear() {
        for (auto it : graphs) {
            delete it.second;
        }
        graphs.clear();
    }
};
