/*
 * ============================================================================
 *  PolyQuery - Multi-Model Relational Algebra Engine
 *  Watermark: vidvathamaiiith
 * ============================================================================
 *
 *  vidvathamaiiith extension: the DESCRIBE command.
 *
 *  DESCRIBE is a metadata / storage-statistics inspector. Unlike PRINT (which
 *  materialises rows), DESCRIBE reports only the catalogue-level bookkeeping the
 *  engine already tracks for a relation, so it is O(1) with respect to the data
 *  size. It is polymorphic across all three data models in the engine: it works
 *  on ordinary tables, on vector matrices, and on graphs.
 *
 *  SYNTAX: DESCRIBE <relation_name>
 */

#include "global.h"
#include "graph.h"

static string indexingStrategyToString(IndexingStrategy strategy)
{
    switch (strategy)
    {
    case BTREE:
        return "BTREE";
    case HASH:
        return "HASH";
    default:
        return "NONE";
    }
}

bool syntacticParseDESCRIBE()
{
    logger.log("syntacticParseDESCRIBE");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = DESCRIBE;
    parsedQuery.describeRelationName = tokenizedQuery[1];
    return true;
}

bool semanticParseDESCRIBE()
{
    logger.log("semanticParseDESCRIBE");
    const string &name = parsedQuery.describeRelationName;
    if (!tableCatalogue.isTable(name) && !matrixCatalogue.isMatrix(name) && !graphCatalogue.isGraph(name))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }
    return true;
}

static void describeTable(const string &name)
{
    Table *table = tableCatalogue.getTable(name);
    cout << "\nDESCRIBE " << name << " (TABLE)" << endl;
    cout << "Model          : Relational Table" << endl;
    cout << "Columns        : ";
    for (size_t i = 0; i < table->columns.size(); i++)
    {
        if (i != 0)
            cout << ", ";
        cout << table->columns[i];
    }
    cout << endl;
    cout << "Column Count   : " << table->columnCount << endl;
    cout << "Row Count      : " << table->rowCount << endl;
    cout << "Block Count    : " << table->blockCount << endl;
    cout << "Rows / Block   : " << table->maxRowsPerBlock << endl;
    cout << "Indexed        : " << (table->indexed ? "YES" : "NO") << endl;
    if (table->indexed)
    {
        cout << "Index Column   : " << table->indexedColumn << endl;
        cout << "Index Strategy : " << indexingStrategyToString(table->indexingStrategy) << endl;
    }
    cout << "Persisted      : " << (table->isPermanent() ? "YES" : "NO (temporary)") << endl;
}

static void describeMatrix(const string &name)
{
    Matrix *matrix = matrixCatalogue.getMatrix(name);
    cout << "\nDESCRIBE " << name << " (VECTOR MATRIX)" << endl;
    cout << "Model          : Vector Matrix" << endl;
    cout << "Dimension      : " << matrix->dimension << endl;
    cout << "Row Count      : " << matrix->rowCount << endl;
    cout << "Block Count    : " << matrix->blockCount << endl;
    cout << "Rows / Block   : " << matrix->maxRowsPerBlock << endl;
}

static void describeGraph(const string &name)
{
    Graph *graph = graphCatalogue.getGraph(name);
    cout << "\nDESCRIBE " << name << " (GRAPH)" << endl;
    cout << "Model          : Graph" << endl;
    cout << "Orientation    : " << (graph->directed ? "DIRECTED" : "UNDIRECTED") << endl;
    long long nodeCount = graph->nodeTable ? graph->nodeTable->rowCount : 0;
    long long edgeCount = graph->edgeTable ? graph->edgeTable->rowCount : 0;
    cout << "Node Count     : " << nodeCount << endl;
    cout << "Edge Count     : " << edgeCount << endl;
}

void executeDESCRIBE()
{
    logger.log("executeDESCRIBE");
    const string &name = parsedQuery.describeRelationName;

    // A name may legitimately be shared across the table and graph catalogues
    // (the graph loader explicitly permits this). We report every model in
    // which the name resolves so DESCRIBE never hides a definition.
    bool found = false;
    if (tableCatalogue.isTable(name))
    {
        describeTable(name);
        found = true;
    }
    if (matrixCatalogue.isMatrix(name))
    {
        describeMatrix(name);
        found = true;
    }
    if (graphCatalogue.isGraph(name))
    {
        describeGraph(name);
        found = true;
    }

    if (!found)
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
    return;
}
