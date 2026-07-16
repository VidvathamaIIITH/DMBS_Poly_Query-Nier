#include "global.h"
/**
 * @brief
 * SYNTAX: LIST TABLES
 *
 * vidvathamaiiith extension: the LIST command has been generalised to also
 * enumerate the graph and matrix catalogues via
 *   LIST GRAPHS
 *   LIST MATRICES
 * The original `LIST TABLES` behaviour is preserved verbatim.
 */
bool syntacticParseLIST()
{
    logger.log("syntacticParseLIST");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    if (tokenizedQuery[1] == "TABLES")
        parsedQuery.listObjectType = LIST_TABLES;
    else if (tokenizedQuery[1] == "GRAPHS") // vidvathamaiiith extension
        parsedQuery.listObjectType = LIST_GRAPHS;
    else if (tokenizedQuery[1] == "MATRICES") // vidvathamaiiith extension
        parsedQuery.listObjectType = LIST_MATRICES;
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = LIST;
    return true;
}

bool semanticParseLIST()
{
    logger.log("semanticParseLIST");
    return true;
}

void executeLIST()
{
    logger.log("executeLIST");
    switch (parsedQuery.listObjectType)
    {
    case LIST_GRAPHS:
        graphCatalogue.print();
        break;
    case LIST_MATRICES:
        matrixCatalogue.print();
        break;
    case LIST_TABLES:
    default:
        tableCatalogue.print();
        break;
    }
}
