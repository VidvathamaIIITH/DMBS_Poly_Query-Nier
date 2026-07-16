/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#ifndef GRAPH_H
#define GRAPH_H

#include "table.h"
#include "syntacticParser.h"
#include <unordered_map>

class Graph
{
public:
    string name = "";
    bool directed = false;
    
    Table* nodeTable;
    Table* edgeTable;
    
    // Shadow tables for efficient querying (sorted)
    // Originals kept in CSV order for PRINT/EXPORT compliance
    Table* nodeTableSorted;
    Table* edgeTableSorted;

    Graph(string name, bool directed);
    ~Graph();
    
    bool load();
    void print();
    void exportGraph();
    int degree(int nodeID);
    
    // For PATH query with conditions
    bool isPath(int src, int dest, const vector<Condition>& conditions, int& pathWeight);
    
private:
    void externalSort(Table& table, int columnIndex);
    
    // Helper methods for condition checking
    bool getNodeAttributes(int nodeID, vector<int>& attrs);
    bool checkNodeConditions(int nodeID, const vector<Condition>& conditions, 
                              const unordered_map<string, int>& anyValues);
    bool checkEdgeConditions(const vector<int>& edgeRow, const vector<Condition>& conditions,
                              const unordered_map<string, int>& anyValues);
};

#endif

