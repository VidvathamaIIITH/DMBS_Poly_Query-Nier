#ifndef MATRIXBUFFERMANAGER_H
#define MATRIXBUFFERMANAGER_H

#include "matrixPage.h"
#include <deque>
#include <unordered_map>
#include <unordered_set>

using namespace std;

class MatrixBufferManager {
    deque<MatrixPage> pages;
    unordered_set<string> pinnedPages;
    
    bool inPool(string pageName);
    MatrixPage getFromPool(string pageName);
    MatrixPage insertIntoPool(string matrixName, int pageIndex, int dimension);

public:
    MatrixBufferManager();
    MatrixPage getPage(string matrixName, int pageIndex, int dimension);
    void writePage(string pageName, const vector<pair<uint, vector<float>>>& rows);
    void writePage(string matrixName, int pageIndex, int dimension, const vector<pair<uint, vector<float>>>& rows, int rowCount);
    void deleteFile(string matrixName, int pageIndex);
    void deleteFile(string fileName);
    
    void pinPage(string matrixName, int pageIndex);
    void unpinPage(string matrixName, int pageIndex);
};

#endif
