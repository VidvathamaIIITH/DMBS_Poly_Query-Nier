#include"global.h"

bool semanticParse(){
    logger.log("semanticParse");
    switch(parsedQuery.queryType){
        case TRANSACTION: return semanticParseTRANSACTION();
        case SETBUFFER: return semanticParseSETBUFFER();
        case CLEAR: return semanticParseCLEAR();
        case CROSS: return semanticParseCROSS();
        case DISTINCT: return semanticParseDISTINCT();
        case EXPORT: return semanticParseEXPORT();
        case INDEX: return semanticParseINDEX();
        case JOIN: return semanticParseJOIN();
        case LIST: return semanticParseLIST();
        case LOAD: return semanticParseLOAD();
        case PRINT: return semanticParsePRINT();
        case PROJECTION: return semanticParsePROJECTION();
        case RENAME: return semanticParseRENAME();
        case SELECTION: return semanticParseSELECTION();
        case SORT: return semanticParseSORT();
        case SOURCE: return semanticParseSOURCE();
        case LOAD_GRAPH: return semanticParseLOAD_GRAPH();
        case PRINT_GRAPH: return semanticParsePRINT_GRAPH();
        case EXPORT_GRAPH: return semanticParseEXPORT_GRAPH();
        case DEGREE: return semanticParseDEGREE();
        case PATH: return semanticParsePATH();
        case GROUP: return semanticParseGROUP();
        case LOAD_MATRIX: return semanticParseLOAD_MATRIX();
        case KNN: return semanticParseKNN();
        default: cout<<"SEMANTIC ERROR"<<endl;
    }

    return false;
}

bool semanticParseLOAD_GRAPH()
{
    logger.log("semanticParseLOAD_GRAPH");
    // Check if graph already exists
    if (graphCatalogue.isGraph(parsedQuery.graphName))
    {
        cout << "SEMANTIC ERROR: Graph already exists" << endl;
        return false;
    }
    // Note: PDF says "However, a graph and a table may share the same name".
    // So we don't check tableCatalogue.
    
    // Check if files exist
    // Files: <graphName>_Nodes_<Type>.csv and <graphName>_Edges_<Type>.csv
    // But PDF says: "To load a directed graph A ... files named A Nodes D.csv and A Edges D.csv"
    string suffix = (parsedQuery.graphType == DIRECTED) ? "D" : "U";
    string nodeFile = parsedQuery.graphName + " Nodes " + suffix; // isFileExists appends .csv
    string edgeFile = parsedQuery.graphName + " Edges " + suffix;
    
    if (!isFileExists(nodeFile))
    {
        cout << "SEMANTIC ERROR: Data file doesn't exist" << endl; // Node file missing
        return false;
    }
    if (!isFileExists(edgeFile))
    {
        cout << "SEMANTIC ERROR: Data file doesn't exist" << endl; // Edge file missing
        return false;
    }
    
    return true;
}

bool semanticParsePRINT_GRAPH()
{
    logger.log("semanticParsePRINT_GRAPH");
    if (!graphCatalogue.isGraph(parsedQuery.graphName))
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }
    return true;
}

bool semanticParseEXPORT_GRAPH()
{
    logger.log("semanticParseEXPORT_GRAPH");
    if (!graphCatalogue.isGraph(parsedQuery.graphName))
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }
    return true;
}

bool semanticParseDEGREE()
{
    logger.log("semanticParseDEGREE");
    if (!graphCatalogue.isGraph(parsedQuery.graphName))
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }
    // Cannot easily check node existence without loading or peeking.
    // Executor will handle "Node does not exist" error as per typical pattern?
    // Or should we check here? Table semantic checks column existence. 
    // But Node ID is data, not schema. Data checks usually in Executor?
    // "If the given node does not exist in the graph, an error Node does not exist should be reported."
    // We defer to executor.
    return true;
}

bool semanticParsePATH()
{
    logger.log("semanticParsePATH");
    if (!graphCatalogue.isGraph(parsedQuery.graphName))
    {
        cout << "SEMANTIC ERROR: Graph doesn't exist" << endl;
        return false;
    }
    // Check if result graph name exists? 
    // "RES <- PATH ..."
    // If RES exists (as graph or table?), error?
    // PDF: "The resulting graph is stored as RES".
    // Usually we check if result exists.
    if (graphCatalogue.isGraph(parsedQuery.pathResultName) || tableCatalogue.isTable(parsedQuery.pathResultName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl; // Standard error
        return false;
    }
    return true;
}

bool semanticParseLOAD_MATRIX()
{
    logger.log("semanticParseLOAD_MATRIX");
    // MatrixCatalogue needs to be checked
    // if (matrixCatalogue.isMatrix(parsedQuery.matrixName)) ...
    // Since matrixCatalogue isn't added yet, we will add it to global.h
    extern class MatrixCatalogue matrixCatalogue; 
    
    // Check if matrix already exists in catalog
    // We will assume matrixCatalogue.isMatrix() exists
    // But since it's not defined yet, we'll write it later and assume the interface
    
    // Check if file exists
    string matrixFile = parsedQuery.matrixName;
    if (!isFileExists(matrixFile))
    {
        cout << "SEMANTIC ERROR: Data file doesn't exist" << endl;
        return false;
    }
    return true;
}

bool semanticParseKNN()
{
    logger.log("semanticParseKNN");
    extern class MatrixCatalogue matrixCatalogue;
    
    // Check if result table name exists
    if (tableCatalogue.isTable(parsedQuery.selectionResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }
    
    // The matrix existence check is deferred to the executor or matrix catalogue implementation
    // because we can't easily query dimensions if it isn't loaded yet.
    
    if (parsedQuery.queryVector.size() == 0) {
        cout << "SEMANTIC ERROR: Invalid Query Vector" << endl;
        return false;
    }
    
    return true;
}
