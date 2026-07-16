#ifndef SYNTACTICPARSER_H
#define SYNTACTICPARSER_H

#include "tableCatalogue.h"

using namespace std;

enum QueryType
{
    TRANSACTION,
    SETBUFFER,
    CLEAR,
    CROSS,
    DISTINCT,
    EXPORT,
    INDEX,
    JOIN,
    LIST,
    LOAD,
    PRINT,
    PROJECTION,
    RENAME,
    SELECTION,
    SORT,
    SOURCE,
    LOAD_GRAPH,
    PRINT_GRAPH,
    EXPORT_GRAPH,
    DEGREE,
    PATH,
    GROUP,
    LOAD_MATRIX,
    KNN,
    // --- vidvathamaiiith extensions (novel commands) ---
    DESCRIBE,
    CHECKSUM,
    UNDETERMINED
};

// vidvathamaiiith extension: which catalogue the LIST command should enumerate.
enum ListObjectType
{
    LIST_TABLES,
    LIST_GRAPHS,
    LIST_MATRICES
};

enum JoinConditionType
{
    JOIN_ATTR_EQ,
    JOIN_ARITH_EQ,
    JOIN_COND_UNSET
};

enum AggregateType
{
    AGG_MAX,
    AGG_MIN,
    AGG_COUNT,
    AGG_SUM,
    AGG_AVG,
    AGG_UNSET
};

struct AggregateExpression
{
    AggregateType aggregateType = AGG_UNSET;
    string columnName = ""; // "*" is valid only for COUNT(*)
};

enum BinaryOperator
{
    LESS_THAN,
    GREATER_THAN,
    LEQ,
    GEQ,
    EQUAL,
    NOT_EQUAL,
    NO_BINOP_CLAUSE
};

enum SortingStrategy
{
    ASC,
    DESC,
    NO_SORT_CLAUSE
};

enum SelectType
{
    COLUMN,
    INT_LITERAL,
    NO_SELECT_CLAUSE
};

enum GraphType
{
    DIRECTED,
    UNDIRECTED,
    NO_GRAPH_TYPE
};

enum ConditionType {
    NODE_CONDITION,
    EDGE_CONDITION
};

enum AttributeType {
    SPECIFIC, // Attribute name given (e.g. A1)
    ANY       // ANY keyword used
};

struct Condition {
    ConditionType conditionType; // Node or Edge
    AttributeType attributeType; // Specific or ANY
    string attributeName;        // Name if specific, empty if ANY
    BinaryOperator op;           // Only EQUAL (==) is supported per spec, but good to have
    int value;                   // 0 or 1.
    bool hasValue;               // true if == NUMBER is present. false if omitted (implicit equality check across path)
};

enum KnnMetric {
    COSINE,
    EUCLIDEAN,
    MANHATTAN, // vidvathamaiiith extension: L1 (taxicab) distance metric
    NO_METRIC
};

class ParsedQuery
{

public:
    QueryType queryType = UNDETERMINED;

    string clearRelationName = "";

    string crossResultRelationName = "";
    string crossFirstRelationName = "";
    string crossSecondRelationName = "";

    string distinctResultRelationName = "";
    string distinctRelationName = "";

    string exportRelationName = "";

    IndexingStrategy indexingStrategy = NOTHING;
    string indexColumnName = "";
    string indexRelationName = "";

    BinaryOperator joinBinaryOperator = NO_BINOP_CLAUSE;
    string joinResultRelationName = "";
    string joinFirstRelationName = "";
    string joinSecondRelationName = "";
    string joinFirstColumnName = "";
    string joinSecondColumnName = "";

    string loadRelationName = "";

    string printRelationName = "";

    string projectionResultRelationName = "";
    vector<string> projectionColumnList;
    string projectionRelationName = "";

    string renameFromColumnName = "";
    string renameToColumnName = "";
    string renameRelationName = "";

    SelectType selectType = NO_SELECT_CLAUSE;
    BinaryOperator selectionBinaryOperator = NO_BINOP_CLAUSE;
    string selectionResultRelationName = "";
    string selectionRelationName = "";
    string selectionFirstColumnName = "";
    string selectionSecondColumnName = "";
    int selectionIntLiteral = 0;

    SortingStrategy sortingStrategy = NO_SORT_CLAUSE;
    string sortResultRelationName = "";
    string sortColumnName = "";
    string sortRelationName = "";

    string sourceFileName = "";

    // Graph related fields
    string graphName = "";
    GraphType graphType = NO_GRAPH_TYPE;
    
    // PATH query
    string pathResultName = "";
    int pathSrcNodeID = -1;
    int pathDestNodeID = -1;
    vector<Condition> pathConditions;
    
    // DEGREE query 
    int degreeNodeID = -1;

    // SETBUFFER
    int setBufferSize = 0;

    // Phase 3 TRANSACTION
    string transactionInputFile = "";

    // Phase 2 SORT (in-place, multi-key, TOP/BOTTOM)
    string sortTableName = "";
    vector<string> sortColumnNames;
    vector<SortingStrategy> sortStrategies;
    bool hasTopClause = false;
    bool hasBottomClause = false;
    long long sortTopRowCount = 0;
    long long sortBottomRowCount = 0;

    // Phase 2 JOIN
    JoinConditionType joinConditionType = JOIN_COND_UNSET;
    string joinLeftToken = "";  // qualified token: A.col
    string joinRightToken = ""; // qualified token: B.col
    char joinArithmeticOperator = '\0'; // '+' or '-'
    int joinArithmeticConstant = 0;
    bool joinHasWhereClause = false;
    string joinWhereToken = ""; // qualified token: A.col
    BinaryOperator joinWhereBinaryOperator = NO_BINOP_CLAUSE;
    int joinWhereConstant = 0;
    bool joinHasProjectClause = false;
    vector<string> joinProjectTokens; // qualified tokens in order

    // Phase 2 GROUP BY
    vector<string> groupResultRelationNames;
    string groupSourceRelationName = "";
    vector<string> groupByColumns;
    AggregateExpression groupHavingLeft;
    BinaryOperator groupHavingBinaryOperator = NO_BINOP_CLAUSE;
    bool groupHavingRightIsConstant = true;
    int groupHavingRightConstant = 0;
    AggregateExpression groupHavingRightAggregate;
    vector<AggregateExpression> groupReturnAggregates;

    // Phase 4 MATRIX / KNN
    string matrixName = "";
    int matrixDimension = 0;
    vector<float> queryVector;
    int knnTopX = 0;
    KnnMetric knnMetric = NO_METRIC;

    // --- vidvathamaiiith extensions ---
    // LIST target selector (LIST TABLES | GRAPHS | MATRICES)
    ListObjectType listObjectType = LIST_TABLES;
    // DESCRIBE <relation> metadata inspector
    string describeRelationName = "";
    // CHECKSUM <table> deterministic content fingerprint
    string checksumRelationName = "";

    ParsedQuery();
    void clear();
};

bool syntacticParseSETBUFFER();
bool syntacticParseTRANSACTION();

bool syntacticParse();
bool syntacticParseCLEAR();
bool syntacticParseCROSS();
bool syntacticParseDISTINCT();
bool syntacticParseEXPORT();
bool syntacticParseINDEX();
bool syntacticParseJOIN();
bool syntacticParseLIST();
bool syntacticParseLOAD();
bool syntacticParsePRINT();
bool syntacticParsePROJECTION();
bool syntacticParseRENAME();
bool syntacticParseSELECTION();
bool syntacticParseSORT();
bool syntacticParseSOURCE();

// Graph syntactic parsers
bool syntacticParseLOAD_GRAPH();
bool syntacticParsePRINT_GRAPH();
bool syntacticParseEXPORT_GRAPH();
bool syntacticParseDEGREE();
bool syntacticParsePATH();
bool syntacticParseGROUP();

// Phase 4 Vector syntactic parsers
bool syntacticParseLOAD_MATRIX();
bool syntacticParseKNN();

// vidvathamaiiith extension syntactic parsers
bool syntacticParseDESCRIBE();
bool syntacticParseCHECKSUM();

bool isFileExists(string tableName);
bool isQueryFile(string fileName);

#endif
