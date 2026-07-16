/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#ifndef MATRIX_H
#define MATRIX_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "logger.h"

using namespace std;

class Matrix
{
public:
    string sourceFileName = "";
    string matrixName = "";
    int dimension = 0;
    long long int rowCount = 0;
    uint blockCount = 0;
    uint maxRowsPerBlock = 0;
    vector<uint> rowsPerBlockCount;

    Matrix() {}
    Matrix(string matrixName, int dimension);
    
    bool blockify();
    bool load();
    void unload();
    void makePermanent();
    bool isPermanent();
};

#endif
