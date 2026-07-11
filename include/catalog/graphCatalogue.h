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
    
    void clear() {
        for (auto it : graphs) {
            delete it.second;
        }
        graphs.clear();
    }
};
