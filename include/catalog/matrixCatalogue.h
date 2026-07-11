#ifndef MATRIX_CATALOGUE_H
#define MATRIX_CATALOGUE_H

#include "matrix.h"
#include "logger.h"
#include <unordered_map>
#include <iostream>

using namespace std;

class MatrixCatalogue
{
    unordered_map<string, Matrix*> matrices;

public:
    MatrixCatalogue() {}
    ~MatrixCatalogue() {
        for (auto pair : matrices) {
            delete pair.second;
        }
    }
    
    void insertMatrix(Matrix* matrix)
    {
        matrices[matrix->matrixName] = matrix;
    }

    void deleteMatrix(string matrixName)
    {
        if (isMatrix(matrixName)) {
            delete matrices[matrixName];
            matrices.erase(matrixName);
        }
    }

    Matrix* getMatrix(string matrixName)
    {
        if (isMatrix(matrixName)) return matrices[matrixName];
        return nullptr;
    }

    bool isMatrix(string matrixName)
    {
        return matrices.find(matrixName) != matrices.end();
    }
    
    void print()
    {
        cout << "\nMATRICES\n";
        for (auto pair : matrices)
        {
            cout << pair.first << endl;
        }
    }
};

#endif
