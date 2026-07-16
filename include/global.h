/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include"executor.h"
#include"graphCatalogue.h"
#include"matrixCatalogue.h"
#include"matrixBufferManager.h"

extern float BLOCK_SIZE;
extern uint BLOCK_COUNT;
extern uint PRINT_COUNT;
extern vector<string> tokenizedQuery;
extern ParsedQuery parsedQuery;
extern TableCatalogue tableCatalogue;
extern GraphCatalogue graphCatalogue;
extern MatrixCatalogue matrixCatalogue;
extern BufferManager bufferManager;
extern MatrixBufferManager matrixBufferManager;
