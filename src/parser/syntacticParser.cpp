#include "global.h"

bool syntacticParse()
{
    logger.log("syntacticParse");
    string possibleQueryType = tokenizedQuery[0];

    if (tokenizedQuery.size() < 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    if (possibleQueryType == "TRANSACTION")
        return syntacticParseTRANSACTION();
    else if (possibleQueryType == "CLEAR")
        return syntacticParseCLEAR();
    else if (possibleQueryType == "SETBUFFER")
        return syntacticParseSETBUFFER();
    else if (possibleQueryType == "INDEX")
        return syntacticParseINDEX();
    else if (possibleQueryType == "LIST")
        return syntacticParseLIST();
    else if (possibleQueryType == "LOAD")
    {
        if (tokenizedQuery.size() > 1 && tokenizedQuery[1] == "GRAPH")
            return syntacticParseLOAD_GRAPH();
        if (tokenizedQuery.size() > 2 && tokenizedQuery[1] == "VECTOR" && tokenizedQuery[2] == "MATRIX")
            return syntacticParseLOAD_MATRIX();
        return syntacticParseLOAD();
    }
    else if (possibleQueryType == "PRINT")
    {
        if (tokenizedQuery.size() > 1 && tokenizedQuery[1] == "GRAPH")
            return syntacticParsePRINT_GRAPH();
        return syntacticParsePRINT();
    }
    else if (possibleQueryType == "RENAME")
        return syntacticParseRENAME();
    else if(possibleQueryType == "EXPORT")
    {
        if (tokenizedQuery.size() > 1 && tokenizedQuery[1] == "GRAPH")
            return syntacticParseEXPORT_GRAPH();
        return syntacticParseEXPORT();
    }
    else if(possibleQueryType == "SOURCE")
        return syntacticParseSOURCE();
    else if (possibleQueryType == "DEGREE")
        return syntacticParseDEGREE();
    else if (possibleQueryType == "SORT")
        return syntacticParseSORT();
    else
    {
        int arrowIndex = -1;
        for (int i = 0; i < (int)tokenizedQuery.size(); i++)
        {
            if (tokenizedQuery[i] == "<-")
            {
                arrowIndex = i;
                break;
            }
        }

        if (arrowIndex == -1 || arrowIndex + 1 >= (int)tokenizedQuery.size())
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        possibleQueryType = tokenizedQuery[arrowIndex + 1];
        if (possibleQueryType == "PROJECT")
            return syntacticParsePROJECTION();
        else if (possibleQueryType == "SELECT")
            return syntacticParseSELECTION();
        else if (possibleQueryType == "KNN")
            return syntacticParseKNN();
        else if (possibleQueryType == "JOIN")
            return syntacticParseJOIN();
        else if (possibleQueryType == "CROSS")
            return syntacticParseCROSS();
        else if (possibleQueryType == "DISTINCT")
            return syntacticParseDISTINCT();
        else if (possibleQueryType == "SORT")
            return syntacticParseSORT();
        else if (possibleQueryType == "PATH")
            return syntacticParsePATH();
        else if (possibleQueryType == "GROUP")
            return syntacticParseGROUP();
        else
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }
    return false;
}

ParsedQuery::ParsedQuery()
{
}

void ParsedQuery::clear()
{
    logger.log("ParseQuery::clear");
    this->queryType = UNDETERMINED;

    this->clearRelationName = "";

    this->crossResultRelationName = "";
    this->crossFirstRelationName = "";
    this->crossSecondRelationName = "";

    this->distinctResultRelationName = "";
    this->distinctRelationName = "";

    this->exportRelationName = "";

    this->indexingStrategy = NOTHING;
    this->indexColumnName = "";
    this->indexRelationName = "";

    this->joinBinaryOperator = NO_BINOP_CLAUSE;
    this->joinResultRelationName = "";
    this->joinFirstRelationName = "";
    this->joinSecondRelationName = "";
    this->joinFirstColumnName = "";
    this->joinSecondColumnName = "";

    this->loadRelationName = "";

    this->printRelationName = "";

    this->projectionResultRelationName = "";
    this->projectionColumnList.clear();
    this->projectionRelationName = "";

    this->renameFromColumnName = "";
    this->renameToColumnName = "";
    this->renameRelationName = "";

    this->selectType = NO_SELECT_CLAUSE;
    this->selectionBinaryOperator = NO_BINOP_CLAUSE;
    this->selectionResultRelationName = "";
    this->selectionRelationName = "";
    this->selectionFirstColumnName = "";
    this->selectionSecondColumnName = "";
    this->selectionIntLiteral = 0;

    this->sortingStrategy = NO_SORT_CLAUSE;
    this->sortResultRelationName = "";
    this->sortColumnName = "";
    this->sortRelationName = "";

    this->sourceFileName = "";
    
    // Graph cleanup
    this->graphName = "";
    this->graphType = NO_GRAPH_TYPE;
    this->pathResultName = "";
    this->pathSrcNodeID = -1;
    this->pathDestNodeID = -1;
    this->pathConditions.clear();
    this->degreeNodeID = -1;

    this->setBufferSize = 0;
    this->transactionInputFile = "";

    this->sortTableName = "";
    this->sortColumnNames.clear();
    this->sortStrategies.clear();
    this->hasTopClause = false;
    this->hasBottomClause = false;
    this->sortTopRowCount = 0;
    this->sortBottomRowCount = 0;

    this->joinConditionType = JOIN_COND_UNSET;
    this->joinLeftToken = "";
    this->joinRightToken = "";
    this->joinArithmeticOperator = '\0';
    this->joinArithmeticConstant = 0;
    this->joinHasWhereClause = false;
    this->joinWhereToken = "";
    this->joinWhereBinaryOperator = NO_BINOP_CLAUSE;
    this->joinWhereConstant = 0;
    this->joinHasProjectClause = false;
    this->joinProjectTokens.clear();

    this->groupResultRelationNames.clear();
    this->groupSourceRelationName = "";
    this->groupByColumns.clear();
    this->groupHavingLeft = AggregateExpression();
    this->groupHavingBinaryOperator = NO_BINOP_CLAUSE;
    this->groupHavingRightConstant = 0;
    this->groupHavingRightAggregate = AggregateExpression();
    this->groupReturnAggregates.clear();

    this->matrixName = "";
    this->matrixDimension = 0;
    this->queryVector.clear();
    this->knnTopX = 0;
}

/**
 * @brief Checks to see if source file exists. Called when LOAD command is
 * invoked.
 *
 * @param tableName 
 * @return true 
 * @return false 
 */
bool isFileExists(string tableName)
{
    string fileName = "data/" + tableName + ".csv";
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

/**
 * @brief Checks to see if source file exists. Called when SOURCE command is
 * invoked.
 *
 * @param tableName 
 * @return true 
 * @return false 
 */
bool isQueryFile(string fileName){
    fileName = "data/" + fileName + ".ra";
    struct stat buffer;
    return (stat(fileName.c_str(), &buffer) == 0);
}

bool syntacticParseLOAD_GRAPH()
{
    logger.log("syntacticParseLOAD_GRAPH");
    // Syntax: LOAD GRAPH <graph name> U/D
    if (tokenizedQuery.size() != 4 || tokenizedQuery[1] != "GRAPH")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    parsedQuery.queryType = LOAD_GRAPH;
    parsedQuery.graphName = tokenizedQuery[2];
    string type = tokenizedQuery[3];
    if (type == "D") parsedQuery.graphType = DIRECTED;
    else if (type == "U") parsedQuery.graphType = UNDIRECTED;
    else {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    return true;
}

bool syntacticParsePRINT_GRAPH()
{
    logger.log("syntacticParsePRINT_GRAPH");
    if (tokenizedQuery.size() != 3 || tokenizedQuery[1] != "GRAPH")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = PRINT_GRAPH;
    parsedQuery.graphName = tokenizedQuery[2];
    return true;
}

bool syntacticParseEXPORT_GRAPH()
{
    logger.log("syntacticParseEXPORT_GRAPH");
    if (tokenizedQuery.size() != 3 || tokenizedQuery[1] != "GRAPH")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = EXPORT_GRAPH;
    parsedQuery.graphName = tokenizedQuery[2];
    return true;
}

bool syntacticParseDEGREE()
{
    logger.log("syntacticParseDEGREE");
    // DEGREE <graph name> <node id>
    if (tokenizedQuery.size() != 3)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = DEGREE;
    parsedQuery.graphName = tokenizedQuery[1];
    parsedQuery.degreeNodeID = stoi(tokenizedQuery[2]);
    return true;
}

bool syntacticParsePATH()
{
    logger.log("syntacticParsePATH");
    
    // Syntax: RES <- PATH <graph_name> <src_NodeID> <dest_NodeID> [WHERE <conditions>]
    
    if (tokenizedQuery.size() < 6)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    parsedQuery.queryType = PATH;
    parsedQuery.pathResultName = tokenizedQuery[0];
    parsedQuery.graphName = tokenizedQuery[3];
    parsedQuery.pathSrcNodeID = stoi(tokenizedQuery[4]);
    parsedQuery.pathDestNodeID = stoi(tokenizedQuery[5]);
    
    if (tokenizedQuery.size() > 6 && tokenizedQuery[6] == "WHERE")
    {
        // Parse Conditions from index 7
        for (int i = 7; i < tokenizedQuery.size(); i++)
        {
            if (tokenizedQuery[i] == "AND") continue;
            
            string condStr = tokenizedQuery[i];
            while (i+1 < tokenizedQuery.size() && tokenizedQuery[i+1] != "AND") {
                 i++;
                 condStr += tokenizedQuery[i];
            }
            
            Condition cond;
            size_t eqPos = condStr.find("==");
            if (eqPos != string::npos) {
                cond.hasValue = true;
                string valStr = condStr.substr(eqPos + 2);
                cond.value = stoi(valStr);
                condStr = condStr.substr(0, eqPos);
            } else {
                cond.hasValue = false;
            }
            
            size_t parenOpen = condStr.find('(');
            size_t parenClose = condStr.find(')');
            
            if (parenOpen == string::npos || parenClose == string::npos) {
                 cout << "SYNTAX ERROR" << endl;
                 return false;
            }
            
            string attr = condStr.substr(0, parenOpen);
            string typeChar = condStr.substr(parenOpen + 1, parenClose - parenOpen - 1);
            
            if (typeChar == "N") cond.conditionType = NODE_CONDITION;
            else if (typeChar == "E") cond.conditionType = EDGE_CONDITION;
            else {
                 cout << "SYNTAX ERROR" << endl;
                 return false;
            }
            
            if (attr == "ANY") {
                cond.attributeType = ANY;
                cond.attributeName = "ANY"; // For consistency with isPath check
            } else {
                cond.attributeType = SPECIFIC;
                cond.attributeName = attr;
            }
            
            parsedQuery.pathConditions.push_back(cond);
        }
    }
    
    return true;
}

bool syntacticParseLOAD_MATRIX()
{
    logger.log("syntacticParseLOAD_MATRIX");
    // LOAD VECTOR MATRIX <matrix_name> <dimension>
    if (tokenizedQuery.size() != 5)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = LOAD_MATRIX;
    parsedQuery.matrixName = tokenizedQuery[3];
    parsedQuery.matrixDimension = stoi(tokenizedQuery[4]);
    return true;
}

bool syntacticParseKNN()
{
    logger.log("syntacticParseKNN");
    // Result <- KNN <matrix_name> QUERY_VEC [f1, f2...] TOP <X> METRIC <COSINE|EUCLIDEAN>
    // Result is tokenizedQuery[0]
    // <- is [1]
    // KNN is [2]
    // matrix_name is [3]
    // QUERY_VEC is [4]
    
    parsedQuery.queryType = KNN;
    parsedQuery.selectionResultRelationName = tokenizedQuery[0]; // Reuse selectionResultRelationName for result
    parsedQuery.matrixName = tokenizedQuery[3];
    
    if (tokenizedQuery.size() < 10 || tokenizedQuery[4] != "QUERY_VEC")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    // Combine everything from [5] to find the [] array
    int curr = 5;
    if (tokenizedQuery[curr] != "[") {
        string t = tokenizedQuery[curr];
        if (t.front() != '[') {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }
    
    string vecStr = "";
    for(int i = 5; i < tokenizedQuery.size(); i++) {
        vecStr += tokenizedQuery[i] + " ";
        if (tokenizedQuery[i].find(']') != string::npos) {
            curr = i + 1;
            break;
        }
    }
    
    // Parse the vector
    string cleanStr = "";
    for(char c : vecStr) {
        if(c != '[' && c != ']') cleanStr += c;
    }
    
    regex delim("[^\\s,]+");
    auto words_begin = sregex_iterator(cleanStr.begin(), cleanStr.end(), delim);
    auto words_end = sregex_iterator();
    for (sregex_iterator i = words_begin; i != words_end; ++i) {
        parsedQuery.queryVector.push_back(stof(i->str()));
    }
    
    if (curr >= tokenizedQuery.size() || tokenizedQuery[curr] != "TOP") {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    curr++;
    parsedQuery.knnTopX = stoi(tokenizedQuery[curr]);
    curr++;
    
    if (curr >= tokenizedQuery.size() || tokenizedQuery[curr] != "METRIC") {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    curr++;
    if (tokenizedQuery[curr] == "COSINE") parsedQuery.knnMetric = COSINE;
    else if (tokenizedQuery[curr] == "EUCLIDEAN") parsedQuery.knnMetric = EUCLIDEAN;
    else {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }
    
    return true;
}

