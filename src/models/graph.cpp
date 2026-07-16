/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "graph.h"
#include "global.h"
#include <algorithm>
#include <queue>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <functional>

Graph::Graph(string name, bool directed)
{
    this->name = name;
    this->directed = directed;
    
    string typeSuffix = directed ? "D" : "U";
    string nodeTableName = name + " Nodes " + typeSuffix;
    string edgeTableName = name + " Edges " + typeSuffix;
    
    // Main tables - store original order
    this->nodeTable = new Table(nodeTableName);
    this->edgeTable = new Table(edgeTableName);
    
    // Sorted shadow tables - for efficient querying
    this->nodeTableSorted = new Table(nodeTableName + "_Sorted");
    this->edgeTableSorted = new Table(edgeTableName + "_Sorted");
}

Graph::~Graph() {
    // Cleanup tables from catalogue
    if (this->nodeTable) tableCatalogue.deleteTable(this->nodeTable->tableName);
    if (this->edgeTable) tableCatalogue.deleteTable(this->edgeTable->tableName);
    if (this->nodeTableSorted) tableCatalogue.deleteTable(this->nodeTableSorted->tableName);
    if (this->edgeTableSorted) tableCatalogue.deleteTable(this->edgeTableSorted->tableName);
}

bool Graph::load()
{
    logger.log("Graph::load");
    
    // 1. Load Node Table
    if (!this->nodeTable->load()) {
        logger.log("Graph::load - Failed to load node table");
        return false; 
    }
    
    // Create sorted copy of node table (clone it)
    // We can't just copy the file because Table::load relies on sourceFileName
    // So we invoke load again on the sorted table pointing to same file, then sort it
    this->nodeTableSorted->sourceFileName = this->nodeTable->sourceFileName;
    if (!this->nodeTableSorted->load()) return false;
    
    // Crucial: Clear sourceFileName so that Table::unload doesn't delete the original file!
    // Shadow tables are "temporary" and default behavior is to delete source.
    this->nodeTableSorted->sourceFileName = ""; 
    
    // 2. Load Edge Table (Special handling for Undirected)
    if (!this->directed) {
        // Preprocess: Duplicate edges (streaming - does not load full graph)
        string originalFile = "data/" + this->edgeTable->tableName + ".csv";
        string tempFile = "data/temp/" + this->edgeTable->tableName + "_Bi.csv";
        
        ifstream fin(originalFile);
        if (!fin.is_open()) return false;
        
        ofstream fout(tempFile);
        if (!fout.is_open()) return false;
        
        string line;
        // Read header
        if (getline(fin, line)) {
            fout << line << endl;
        }
        
        while (getline(fin, line)) {
            fout << line << endl;
            
            // Parse line to swap Src and Dest
            stringstream ss(line);
            string segment;
            vector<string> parts;
            while(getline(ss, segment, ',')) {
                 parts.push_back(segment);
            }
            if (parts.size() >= 2) {
                // Swap parts[0] and parts[1]
                string src = parts[0];
                string dest = parts[1];
                parts[0] = dest;
                parts[1] = src;
                
                // Write reverse edge
                for (size_t i = 0; i < parts.size(); i++) {
                    fout << parts[i] << (i == parts.size()-1 ? "" : ",");
                }
                fout << endl;
            }
        }
        fin.close();
        fout.close();
        
        // Point Edge Table to temp file
        this->edgeTable->sourceFileName = tempFile;
    }
    
    if (!this->edgeTable->load()) {
        logger.log("Graph::load - Failed to load edge table");
        return false;
    }
    
    // Create sorted copy of edge table
    this->edgeTableSorted->sourceFileName = this->edgeTable->sourceFileName;
    if (!this->edgeTableSorted->load()) return false;
    
    // Crucial: Clear sourceFileName so that Table::unload doesn't delete the original file!
    this->edgeTableSorted->sourceFileName = ""; 
    
    // Register tables in TableCatalogue so Page/BufferManager can find them
    if (!tableCatalogue.isTable(this->nodeTable->tableName))
        tableCatalogue.insertTable(this->nodeTable);
    if (!tableCatalogue.isTable(this->edgeTable->tableName))
        tableCatalogue.insertTable(this->edgeTable);
        
    // Register shadow tables
    if (!tableCatalogue.isTable(this->nodeTableSorted->tableName))
        tableCatalogue.insertTable(this->nodeTableSorted);
    if (!tableCatalogue.isTable(this->edgeTableSorted->tableName))
        tableCatalogue.insertTable(this->edgeTableSorted);

    // 3. Sort SHADOW Tables
    this->externalSort(*this->nodeTableSorted, 0);
    this->externalSort(*this->edgeTableSorted, 0);

    return true;
}

void Graph::externalSort(Table& table, int columnIndex)
{
    // External merge sort respecting 2-block memory constraint
    if (table.blockCount == 0) return;
    
    // Phase 1: Sort individual blocks
    for (int i = 0; i < table.blockCount; i++) {
        Page page = bufferManager.getPage(table.tableName, i);
        int rowC = page.getRowCount();
        if (rowC == 0) continue;
        
        vector<vector<int>> rows;
        for (int r = 0; r < rowC; r++) {
             rows.push_back(page.getRow(r));
        }
        
        std::sort(rows.begin(), rows.end(), [columnIndex](const vector<int>& a, const vector<int>& b) {
            return a[columnIndex] < b[columnIndex];
        });
        
        bufferManager.writePage(table.tableName, i, rows, rowC);
    }
    
    // Phase 2: 2-way merge passes
    if (table.blockCount <= 1) return;
    
    int numBlocks = table.blockCount;
    
    while (numBlocks > 1) {
        for (int i = 0; i < numBlocks; i += 2) {
            if (i + 1 >= numBlocks) continue;
            
            // Merge blocks i and i+1
            Page pageA = bufferManager.getPage(table.tableName, i);
            Page pageB = bufferManager.getPage(table.tableName, i + 1);
            
            vector<vector<int>> merged;
            int ptrA = 0, ptrB = 0;
            int rowsA = pageA.getRowCount();
            int rowsB = pageB.getRowCount();
            
            while (ptrA < rowsA && ptrB < rowsB) {
                vector<int> rowA = pageA.getRow(ptrA);
                vector<int> rowB = pageB.getRow(ptrB);
                
                if (rowA[columnIndex] <= rowB[columnIndex]) {
                    merged.push_back(rowA);
                    ptrA++;
                } else {
                    merged.push_back(rowB);
                    ptrB++;
                }
            }
            
            while (ptrA < rowsA) merged.push_back(pageA.getRow(ptrA++));
            while (ptrB < rowsB) merged.push_back(pageB.getRow(ptrB++));
            
            // Write merged data back
            int maxPerBlock = table.maxRowsPerBlock;
            vector<vector<int>> block1(merged.begin(), merged.begin() + min((int)merged.size(), maxPerBlock));
            bufferManager.writePage(table.tableName, i, block1, block1.size());
            
            if ((int)block1.size() < (int)merged.size()) {
                vector<vector<int>> block2(merged.begin() + block1.size(), merged.end());
                bufferManager.writePage(table.tableName, i + 1, block2, block2.size());
            }
        }
        break; // Single pass for Phase 1 compliance
    }
}

void Graph::print()
{
    // Print in original CSV order using unsorted tables
    
    cout << this->nodeTable->rowCount << endl;
    cout << this->edgeTable->rowCount << endl;
    cout << (this->directed ? "D" : "U") << endl;
    cout << endl; 
    
    // Print Nodes
    this->nodeTable->print();
    cout << endl;
    
    // Print Edges
    this->edgeTable->print();
    cout << endl;
}

void Graph::exportGraph() {
    // EXPORT using unsorted tables to preserve order per TA Q29
    this->nodeTable->makePermanent();
    this->edgeTable->makePermanent();
}

int Graph::degree(int nodeID)
{
    // Use SORTED tables for efficient query processing per TA Q1/35
    // Note: degree check can use linear scan on unsorted, but sorted is better if we switch to binary search later
    // For now, keep linear scan logic but on sorted table for consistency with isPath
    
    // Check node exists
    bool nodeFound = false;
    // Binary search for node existence in sorted table
    int start = 0, end = this->nodeTableSorted->blockCount - 1;
    while (start <= end) {
        int mid = start + (end - start) / 2;
        Page page = bufferManager.getPage(this->nodeTableSorted->tableName, mid);
        int rowC = page.getRowCount();
        if (rowC == 0) { end = mid - 1; continue; }
        
        int firstID = page.getRow(0)[0];
        int lastID = page.getRow(rowC-1)[0];
        
        if (nodeID >= firstID && nodeID <= lastID) {
            for (int r = 0; r < rowC; r++) {
                if (page.getRow(r)[0] == nodeID) { nodeFound = true; break; }
            }
            break;
        } else if (nodeID < firstID) {
            end = mid - 1;
        } else {
            start = mid + 1;
        }
    }
    
    if (!nodeFound) return -1;
    
    int count = 0;
    // Find the first block where nodeID could exist
    int startB = 0, endB = this->edgeTableSorted->blockCount - 1;
    int firstBlock = -1;
    while (startB <= endB) {
        int mid = startB + (endB - startB) / 2;
        Page p = bufferManager.getPage(this->edgeTableSorted->tableName, mid);
        int rowC = p.getRowCount();
        if (rowC == 0) { endB = mid - 1; continue; }
        
        int lastID = p.getRow(rowC-1)[0];
        if (lastID >= nodeID) {
            firstBlock = mid;
            endB = mid - 1;
        } else {
            startB = mid + 1;
        }
    }
    
    if (firstBlock != -1) {
        for (int i = firstBlock; i < this->edgeTableSorted->blockCount; i++) {
            Page page = bufferManager.getPage(this->edgeTableSorted->tableName, i);
            int rowC = page.getRowCount();
            if (rowC > 0 && page.getRow(0)[0] > nodeID) break;
            for (int r = 0; r < rowC; r++) {
                vector<int> row = page.getRow(r);
                if (row[0] == nodeID) count++;
                else if (row[0] > nodeID) break;
            }
        }
    }
    
    // For directed, we also need in-degree (scan for row[1] == nodeID)
    // Unfortunately, edgeTableSorted is only sorted by Src (col 0).
    // To get in-degree efficiently, we'd need another shadow table sorted by Dest.
    // For now, if it's directed, we have to do a full scan or just use the existing one.
    // Given the constraints, let's just do a full scan for in-degree if directed.
    if (this->directed) {
        for (int i = 0; i < this->edgeTableSorted->blockCount; i++) {
            Page page = bufferManager.getPage(this->edgeTableSorted->tableName, i);
            int rowC = page.getRowCount();
            for (int r = 0; r < rowC; r++) {
                if (page.getRow(r)[1] == nodeID) count++;
            }
        }
    }
    
    return count;
}

// Helper: Get node attributes using SORTED table
bool Graph::getNodeAttributes(int nodeID, vector<int>& attrs) {
    int start = 0, end = this->nodeTableSorted->blockCount - 1;
    while (start <= end) {
        int mid = start + (end - start) / 2;
        Page page = bufferManager.getPage(this->nodeTableSorted->tableName, mid);
        int rowC = page.getRowCount();
        if (rowC == 0) { end = mid - 1; continue; }
        
        int firstID = page.getRow(0)[0];
        int lastID = page.getRow(rowC-1)[0];
        
        if (nodeID >= firstID && nodeID <= lastID) {
            for (int r = 0; r < rowC; r++) {
                vector<int> row = page.getRow(r);
                if (row[0] == nodeID) {
                    attrs = row;
                    return true;
                }
            }
            return false;
        } else if (nodeID < firstID) {
            end = mid - 1;
        } else {
            start = mid + 1;
        }
    }
    return false;
}

// Helper: Check if a node satisfies node conditions
bool Graph::checkNodeConditions(int nodeID, const vector<Condition>& conditions, 
                                  const unordered_map<string, int>& anyValues) {
    vector<int> nodeAttrs;
    if (!getNodeAttributes(nodeID, nodeAttrs)) return false;
    
    for (const auto& cond : conditions) {
        if (cond.conditionType != NODE_CONDITION) continue;
        
        int colIdx = this->nodeTableSorted->getColumnIndex(cond.attributeName);
        if (colIdx == -1 && cond.attributeType == SPECIFIC) continue;
        
        if (cond.attributeType == ANY) {
            if (anyValues.count(cond.attributeName)) {
                if (colIdx != -1 && nodeAttrs[colIdx] != anyValues.at(cond.attributeName)) {
                    return false;
                }
            }
        } else if (cond.hasValue) {
            if (colIdx != -1 && nodeAttrs[colIdx] != cond.value) {
                return false;
            }
        }
    }
    return true;
}

// Helper: Check if an edge satisfies edge conditions
bool Graph::checkEdgeConditions(const vector<int>& edgeRow, const vector<Condition>& conditions,
                                  const unordered_map<string, int>& anyValues) {
    for (const auto& cond : conditions) {
        if (cond.conditionType != EDGE_CONDITION) continue;
        
        int colIdx = this->edgeTableSorted->getColumnIndex(cond.attributeName);
        if (colIdx == -1 && cond.attributeType == SPECIFIC) continue;
        
        if (cond.attributeType == ANY) {
            if (anyValues.count(cond.attributeName)) {
                if (colIdx != -1 && edgeRow[colIdx] != anyValues.at(cond.attributeName)) {
                    return false;
                }
            }
        } else if (cond.hasValue) {
            if (colIdx != -1 && edgeRow[colIdx] != cond.value) {
                return false;
            }
        }
    }
    return true;
}

bool Graph::isPath(int src, int dest, const vector<Condition>& conditions, int& pathWeight)
{
    // 0. Base Case: src == dest
    if (src == dest) {
        pathWeight = 0;
        // Even for self-loop, some condition might hold for src node
        // We try all possible combinations to see if ANY works
    }

    // 1. Check if src/dest exist
    if (degree(src) == -1 || degree(dest) == -1) return false;

    // 2. Identify all "Flexible" parts of conditions
    // - Implicit ANY (e.g. B1(E) with no value): value can be 0 or 1
    // - Explicit ANY (e.g. ANY(E) == 1): column can be any valid attribute column
    // - Mixed ANY (e.g. ANY(N)): column can be any, value can be 0 or 1
    
    struct FlexibleCondition {
        int condIdx;
        bool isAny; // Literal "ANY"
        vector<int> possibleColumns; // Indices in table
        vector<int> possibleValues; 
    };
    
    vector<FlexibleCondition> flexNodes;
    vector<FlexibleCondition> flexEdges;
    
    for (size_t i = 0; i < conditions.size(); i++) {
        const auto& cond = conditions[i];
        bool isNode = (cond.conditionType == NODE_CONDITION);
        Table* table = isNode ? this->nodeTableSorted : this->edgeTableSorted;
        
        FlexibleCondition flex;
        flex.condIdx = i;
        
        if (cond.attributeName == "ANY") {
            flex.isAny = true;
            for (int col = (isNode ? 1 : 3); col < (int)table->columns.size(); col++) {
                flex.possibleColumns.push_back(col);
            }
            if (!cond.hasValue) {
                flex.possibleValues = {0, 1};
            } else {
                flex.possibleValues = {cond.value};
            }
        } else {
            flex.isAny = false;
            flex.possibleColumns = { table->getColumnIndex(cond.attributeName) };
            if (!cond.hasValue) {
                flex.possibleValues = {0, 1};
            } else {
                flex.possibleValues = {cond.value};
            }
        }
        
        if (isNode) flexNodes.push_back(flex);
        else flexEdges.push_back(flex);
    }
    
    // We need to try all combinations of assignments for ALL flexible conditions
    // To keep it sane, we can just find *one* combination that works.
    
    // Total combinations = Product(possibleColumns.size * possibleValues.size)
    // This can be huge! But usually num conditions is small.
    
    // Let's use a recursion or iterative approach to pick assignments
    vector<pair<int, int>> nodeAssignments(flexNodes.size()); // {colIdx, value}
    vector<pair<int, int>> edgeAssignments(flexEdges.size());
    
    auto runDijkstra = [&](const unordered_map<int, pair<int, int>>& nodeMap, 
                           const unordered_map<int, pair<int, int>>& edgeMap) -> bool {
        
        // Check source and dest node conditions immediately
        auto checkNode = [&](int nodeID) {
            vector<int> attrs;
            if (!getNodeAttributes(nodeID, attrs)) return false;
            for (auto const& [idx, assign] : nodeMap) {
                if (assign.first == -1 || attrs[assign.first] != assign.second) return false;
            }
            // Also check fixed node conditions
            for (size_t i = 0; i < conditions.size(); i++) {
                if (conditions[i].conditionType == NODE_CONDITION && conditions[i].attributeName != "ANY" && conditions[i].hasValue) {
                    int col = this->nodeTableSorted->getColumnIndex(conditions[i].attributeName);
                    if (col != -1 && attrs[col] != conditions[i].value) return false;
                }
            }
            return true;
        };
        
        if (!checkNode(src) || !checkNode(dest)) return false;
        if (src == dest) { pathWeight = 0; return true; }

        unordered_map<int, int> dist;
        priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
        dist[src] = 0;
        pq.push({0, src});

        while(!pq.empty()) {
            int d = pq.top().first;
            int u = pq.top().second;
            pq.pop();
            if (u == dest) { pathWeight = d; return true; }
            if (dist.count(u) && dist[u] < d) continue;

            // Find the first block where u could have neighbors
            int startB = 0, endB = this->edgeTableSorted->blockCount - 1;
            int firstBlock = -1;
            while (startB <= endB) {
                int mid = startB + (endB - startB) / 2;
                Page p = bufferManager.getPage(this->edgeTableSorted->tableName, mid);
                int rowC = p.getRowCount();
                if (rowC == 0) { endB = mid - 1; continue; }
                
                int lastID = p.getRow(rowC-1)[0];
                if (lastID >= u) {
                    firstBlock = mid;
                    endB = mid - 1;
                } else {
                    startB = mid + 1;
                }
            }
            if (firstBlock == -1) continue;

            for (int i = firstBlock; i < this->edgeTableSorted->blockCount; i++) {
                Page page = bufferManager.getPage(this->edgeTableSorted->tableName, i);
                int rowC = page.getRowCount();
                if (rowC > 0 && page.getRow(0)[0] > u) break;

                for (int r = 0; r < rowC; r++) {
                    vector<int> row = page.getRow(r);
                    if (row[0] < u) continue;
                    if (row[0] > u) break;
                    
                    int v = row[1];
                    int w = row[2];
                    
                    // Check edge conditions
                    bool edgeOk = true;
                    for (auto const& [idx, assign] : edgeMap) {
                        if (assign.first == -1 || row[assign.first] != assign.second) { edgeOk = false; break; }
                    }
                    if (!edgeOk) continue;
                    for (size_t k = 0; k < conditions.size(); k++) {
                        if (conditions[k].conditionType == EDGE_CONDITION && conditions[k].attributeName != "ANY" && conditions[k].hasValue) {
                            int col = this->edgeTableSorted->getColumnIndex(conditions[k].attributeName);
                             if (col != -1 && row[col] != conditions[k].value) { edgeOk = false; break; }
                        }
                    }
                    if (!edgeOk) continue;

                    // Check node v
                    if (v != dest && !checkNode(v)) continue;

                    if (!dist.count(v) || dist[v] > d + w) {
                        dist[v] = d + w;
                        pq.push({dist[v], v});
                    }
                }
            }
        }
        return false;
    };

    // Combinations of Flexible Conditions
    // We'll use a simple recursive explorer
    function<bool(int, int, unordered_map<int, pair<int, int>>&, unordered_map<int, pair<int, int>>&)> explore;
    explore = [&](int nodeIdx, int edgeIdx, unordered_map<int, pair<int, int>>& nMap, unordered_map<int, pair<int, int>>& eMap) -> bool {
        if (nodeIdx < (int)flexNodes.size()) {
            for (int col : flexNodes[nodeIdx].possibleColumns) {
                for (int val : flexNodes[nodeIdx].possibleValues) {
                    nMap[flexNodes[nodeIdx].condIdx] = {col, val};
                    if (explore(nodeIdx + 1, edgeIdx, nMap, eMap)) return true;
                }
            }
            return false;
        }
        if (edgeIdx < (int)flexEdges.size()) {
            for (int col : flexEdges[edgeIdx].possibleColumns) {
                for (int val : flexEdges[edgeIdx].possibleValues) {
                    eMap[flexEdges[edgeIdx].condIdx] = {col, val};
                    if (explore(nodeIdx, edgeIdx + 1, nMap, eMap)) return true;
                }
            }
            return false;
        }
        // Base case: all flexible conditions assigned
        return runDijkstra(nMap, eMap);
    };

    unordered_map<int, pair<int, int>> nMap, eMap;
    return explore(0, 0, nMap, eMap);
}

