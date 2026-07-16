/*
 * ============================================================================
 *  PolyQuery - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "global.h"

void executeLOAD_MATRIX()
{
    logger.log("executeLOAD_MATRIX");
    Matrix* matrix = new Matrix(parsedQuery.matrixName, parsedQuery.matrixDimension);
    if (matrix->load())
    {
        matrixCatalogue.insertMatrix(matrix);
        cout << "Loaded Matrix. Block Count: " << matrix->blockCount << " Row Count: " << matrix->rowCount << endl;
    }
    else
    {
        delete matrix;
    }
}
