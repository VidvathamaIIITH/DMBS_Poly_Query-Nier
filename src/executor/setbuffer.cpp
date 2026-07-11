#include "global.h"

/**
 * @brief
 * SYNTAX: SETBUFFER K
 */
bool syntacticParseSETBUFFER()
{
    logger.log("syntacticParseSETBUFFER");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    regex numeric("[0-9]+");
    if (!regex_match(tokenizedQuery[1], numeric))
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = SETBUFFER;
    parsedQuery.setBufferSize = stoi(tokenizedQuery[1]);
    return true;
}

bool semanticParseSETBUFFER()
{
    logger.log("semanticParseSETBUFFER");
    if (parsedQuery.setBufferSize < 2 || parsedQuery.setBufferSize > 10)
    {
        cout << "SEMANTIC ERROR" << endl;
        return false;
    }
    return true;
}

void executeSETBUFFER()
{
    logger.log("executeSETBUFFER");
    BLOCK_COUNT = (uint)parsedQuery.setBufferSize;
    // Keep already loaded pages intact; new accesses naturally follow updated BLOCK_COUNT.
}
