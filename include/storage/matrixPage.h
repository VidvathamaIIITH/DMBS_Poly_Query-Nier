/*
 * ============================================================================
 *  PolyQuery - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#ifndef MATRIXPAGE_H
#define MATRIXPAGE_H

#include "logger.h"
#include <vector>
#include <string>

using namespace std;

class MatrixPage {
    string matrixName;
    int pageIndex;
    int dimension;
    int rowCount;
    vector<pair<uint, vector<float>>> rows;

public:
    string pageName = "";
    MatrixPage();
    MatrixPage(string matrixName, int pageIndex, int dimension);
    
    // For modifying or sorting, we can access rows
    vector<pair<uint, vector<float>>>& getRows() { return rows; }
    
    int getRowCount() const { return rowCount; }
    void writePage();
};

#endif
