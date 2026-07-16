/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "global.h"
#include "graph.h"

void executeLOAD_GRAPH()
{
    logger.log("executeLOAD_GRAPH");
    
    Graph* graph = new Graph(parsedQuery.graphName, parsedQuery.graphType == DIRECTED);
    
    if (graph->load())
    {
        graphCatalogue.insertGraph(graph);
        // Output format: Loaded Graph.Node Count:<n>,Edge Count:<e>
        cout << "Loaded Graph.Node Count:" << graph->nodeTable->rowCount << ",Edge Count:" << graph->edgeTable->rowCount << endl;
    }
    else
    {
        // cleanup if failed
        delete graph;
    }
    return;
}
