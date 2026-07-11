#include "global.h"
#include "graph.h"

void executePRINT_GRAPH()
{
    logger.log("executePRINT_GRAPH");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.graphName);
    if (graph) {
        graph->print();
    }
    // Else semantic checks should have caught it, or we print nothing?
    return;
}

void executeEXPORT_GRAPH()
{
    logger.log("executeEXPORT_GRAPH");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.graphName);
    if (graph) {
        graph->exportGraph();
    }
    return;
}

void executeDEGREE()
{
    logger.log("executeDEGREE");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.graphName);
    if (!graph) {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return;
    }
    int deg = graph->degree(parsedQuery.degreeNodeID);
    if (deg == -1) cout << "Node does not exist" << endl;
    else cout << deg << endl;
}

void executePATH()
{
    logger.log("executePATH");
    Graph* graph = graphCatalogue.getGraph(parsedQuery.graphName);
    if (!graph) {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return;
    }
    // Check node existence first for specific "Node does not exist" error
    if (graph->degree(parsedQuery.pathSrcNodeID) == -1 || graph->degree(parsedQuery.pathDestNodeID) == -1) {
        cout << "Node does not exist" << endl;
        return;
    }

    int weight = 0;
    if (graph->isPath(parsedQuery.pathSrcNodeID, parsedQuery.pathDestNodeID, parsedQuery.pathConditions, weight))
    {
        cout << "TRUE " << weight << endl;
    }
    else
    {
        cout << "FALSE" << endl;
    }
}
