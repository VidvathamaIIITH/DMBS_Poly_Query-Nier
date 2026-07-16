/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "global.h"

struct JoinPartitionMeta
{
    string name;
    uint blockCount = 0;
    long long rowCount = 0;
    vector<uint> rowsPerBlock;
};

class JoinPartitionReader
{
public:
    JoinPartitionMeta meta;
    int pageIndex = 0;
    int rowIndex = 0;
    Page page;
    bool valid = false;

    JoinPartitionReader(const JoinPartitionMeta &meta) : meta(meta)
    {
        if (this->meta.blockCount > 0)
        {
            this->page = bufferManager.getPage(this->meta.name, 0);
            this->valid = true;
        }
    }

    bool getNext(vector<int> &row)
    {
        if (!this->valid)
            return false;

        if (this->rowIndex >= (int)this->meta.rowsPerBlock[this->pageIndex])
        {
            this->pageIndex++;
            if (this->pageIndex >= (int)this->meta.blockCount)
            {
                this->valid = false;
                return false;
            }
            this->page = bufferManager.getPage(this->meta.name, this->pageIndex);
            this->rowIndex = 0;
        }

        row = this->page.getRow(this->rowIndex++);
        return !row.empty();
    }
};

static string stripSemicolonJoin(string token)
{
    while (!token.empty() && token.back() == ';')
        token.pop_back();
    return token;
}

static bool parseQualifiedName(const string &token, string &relation, string &column)
{
    size_t dotPos = token.find('.');
    if (dotPos == string::npos || dotPos == 0 || dotPos + 1 >= token.size())
        return false;
    relation = token.substr(0, dotPos);
    column = token.substr(dotPos + 1);
    return true;
}

static BinaryOperator parseBinaryOpJoin(const string &op)
{
    if (op == "<")
        return LESS_THAN;
    if (op == ">")
        return GREATER_THAN;
    if (op == ">=" || op == "=>")
        return GEQ;
    if (op == "<=" || op == "=<")
        return LEQ;
    if (op == "==")
        return EQUAL;
    if (op == "!=")
        return NOT_EQUAL;
    return NO_BINOP_CLAUSE;
}

static void deleteJoinPartition(const JoinPartitionMeta &partition)
{
    if (tableCatalogue.isTable(partition.name))
    {
        tableCatalogue.deleteTable(partition.name);
        return;
    }
    for (uint i = 0; i < partition.blockCount; i++)
        bufferManager.deleteFile(partition.name, i);
}

static void registerJoinPartitionMeta(const JoinPartitionMeta &partition, int columnCount, uint maxRowsPerBlock)
{
    if (partition.blockCount == 0)
        return;
    if (tableCatalogue.isTable(partition.name))
        return;

    Table *metaTable = new Table();
    metaTable->tableName = partition.name;
    metaTable->sourceFileName = "";
    metaTable->columnCount = columnCount;
    metaTable->maxRowsPerBlock = maxRowsPerBlock;
    metaTable->rowCount = partition.rowCount;
    metaTable->blockCount = partition.blockCount;
    metaTable->rowsPerBlockCount = partition.rowsPerBlock;
    metaTable->columns.clear();
    for (int i = 0; i < columnCount; i++)
        metaTable->columns.push_back("C" + to_string(i));
    tableCatalogue.insertTable(metaTable);
}

bool syntacticParseJOIN()
{
    logger.log("syntacticParseJOIN");

    if (tokenizedQuery.size() < 9 || tokenizedQuery[1] != "<-" || tokenizedQuery[2] != "JOIN" || tokenizedQuery[5] != "ON")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = JOIN;
    parsedQuery.joinResultRelationName = tokenizedQuery[0];
    parsedQuery.joinFirstRelationName = tokenizedQuery[3];
    parsedQuery.joinSecondRelationName = tokenizedQuery[4];

    int idx = 6;
    string t0 = stripSemicolonJoin(tokenizedQuery[idx]);
    string t1 = (idx + 1 < (int)tokenizedQuery.size()) ? stripSemicolonJoin(tokenizedQuery[idx + 1]) : "";
    string t2 = (idx + 2 < (int)tokenizedQuery.size()) ? stripSemicolonJoin(tokenizedQuery[idx + 2]) : "";
    string t3 = (idx + 3 < (int)tokenizedQuery.size()) ? stripSemicolonJoin(tokenizedQuery[idx + 3]) : "";
    string t4 = (idx + 4 < (int)tokenizedQuery.size()) ? stripSemicolonJoin(tokenizedQuery[idx + 4]) : "";

    if (idx + 2 < (int)tokenizedQuery.size() && t1 == "==")
    {
        parsedQuery.joinConditionType = JOIN_ATTR_EQ;
        parsedQuery.joinLeftToken = t0;
        parsedQuery.joinRightToken = t2;
        idx += 3;
    }
    else if (idx + 4 < (int)tokenizedQuery.size() && (t1 == "+" || t1 == "-") && t3 == "==")
    {
        parsedQuery.joinConditionType = JOIN_ARITH_EQ;
        parsedQuery.joinLeftToken = t0;
        parsedQuery.joinArithmeticOperator = t1[0];
        parsedQuery.joinRightToken = t2;
        regex numeric("[0-9]+");
        if (!regex_match(t4, numeric))
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.joinArithmeticConstant = stoi(t4);
        idx += 5;
    }
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    while (idx < (int)tokenizedQuery.size())
    {
        string token = stripSemicolonJoin(tokenizedQuery[idx]);
        if (token == "WHERE")
        {
            if (parsedQuery.joinHasWhereClause || idx + 3 >= (int)tokenizedQuery.size())
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }

            parsedQuery.joinHasWhereClause = true;
            parsedQuery.joinWhereToken = stripSemicolonJoin(tokenizedQuery[idx + 1]);
            parsedQuery.joinWhereBinaryOperator = parseBinaryOpJoin(stripSemicolonJoin(tokenizedQuery[idx + 2]));
            if (parsedQuery.joinWhereBinaryOperator == NO_BINOP_CLAUSE)
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }

            regex numeric("[-]?[0-9]+");
            string rhs = stripSemicolonJoin(tokenizedQuery[idx + 3]);
            if (!regex_match(rhs, numeric))
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }
            parsedQuery.joinWhereConstant = stoi(rhs);
            idx += 4;
        }
        else if (token == "PROJECT")
        {
            if (parsedQuery.joinHasProjectClause || idx + 1 >= (int)tokenizedQuery.size())
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }

            parsedQuery.joinHasProjectClause = true;
            idx++;
            while (idx < (int)tokenizedQuery.size())
            {
                string colToken = stripSemicolonJoin(tokenizedQuery[idx]);
                if (colToken.empty())
                {
                    idx++;
                    continue;
                }
                parsedQuery.joinProjectTokens.push_back(colToken);
                idx++;
            }
        }
        else
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }

    if (parsedQuery.joinHasProjectClause && parsedQuery.joinProjectTokens.empty())
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    return true;
}

bool semanticParseJOIN()
{
    logger.log("semanticParseJOIN");

    if (tableCatalogue.isTable(parsedQuery.joinResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }

    if (!tableCatalogue.isTable(parsedQuery.joinFirstRelationName) || !tableCatalogue.isTable(parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    auto validateQualifiedColumn = [&](const string &token) -> bool
    {
        string rel, col;
        if (!parseQualifiedName(token, rel, col))
            return false;
        if (rel != parsedQuery.joinFirstRelationName && rel != parsedQuery.joinSecondRelationName)
            return false;
        return tableCatalogue.isColumnFromTable(col, rel);
    };

    if (!validateQualifiedColumn(parsedQuery.joinLeftToken) || !validateQualifiedColumn(parsedQuery.joinRightToken))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    if (parsedQuery.joinHasWhereClause && !validateQualifiedColumn(parsedQuery.joinWhereToken))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    for (const string &projectToken : parsedQuery.joinProjectTokens)
    {
        if (!validateQualifiedColumn(projectToken))
        {
            cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
            return false;
        }
    }

    return true;
}

static int partitionHashValue(int key, int partitionCount)
{
    if (partitionCount <= 1)
        return 0;
    int hashed = key % partitionCount;
    if (hashed < 0)
        hashed += partitionCount;
    return hashed;
}

static vector<JoinPartitionMeta> buildPartitions(Table &table,
                                                 const string &baseName,
                                                 int partitionCount,
                                                 function<int(const vector<int> &)> keyExtractor)
{
    vector<JoinPartitionMeta> partitions(partitionCount);
    for (int i = 0; i < partitionCount; i++)
        partitions[i].name = baseName + "_P" + to_string(i);

    vector<vector<vector<int>>> buffers(partitionCount);
    vector<int> pageIndex(partitionCount, 0);

    Cursor cursor = table.getCursor();
    vector<int> row = cursor.getNext();
    while (!row.empty())
    {
        int key = keyExtractor(row);
        int partitionId = partitionHashValue(key, partitionCount);
        buffers[partitionId].push_back(row);
        partitions[partitionId].rowCount++;

        if ((int)buffers[partitionId].size() == (int)table.maxRowsPerBlock)
        {
            bufferManager.writePage(partitions[partitionId].name, pageIndex[partitionId], buffers[partitionId], buffers[partitionId].size());
            partitions[partitionId].rowsPerBlock.push_back((uint)buffers[partitionId].size());
            partitions[partitionId].blockCount++;
            pageIndex[partitionId]++;
            buffers[partitionId].clear();
        }

        row = cursor.getNext();
    }

    for (int i = 0; i < partitionCount; i++)
    {
        if (!buffers[i].empty())
        {
            bufferManager.writePage(partitions[i].name, pageIndex[i], buffers[i], buffers[i].size());
            partitions[i].rowsPerBlock.push_back((uint)buffers[i].size());
            partitions[i].blockCount++;
        }
        registerJoinPartitionMeta(partitions[i], table.columnCount, table.maxRowsPerBlock);
    }

    return partitions;
}

void executeJOIN()
{
    logger.log("executeJOIN");

    Table *left = tableCatalogue.getTable(parsedQuery.joinFirstRelationName);
    Table *right = tableCatalogue.getTable(parsedQuery.joinSecondRelationName);

    string rel;
    string col;
    parseQualifiedName(parsedQuery.joinLeftToken, rel, col);
    int leftJoinCol = (rel == left->tableName) ? left->getColumnIndex(col) : right->getColumnIndex(col);
    int rightJoinCol = -1;
    parseQualifiedName(parsedQuery.joinRightToken, rel, col);
    if (rel == right->tableName)
        rightJoinCol = right->getColumnIndex(col);
    else
        rightJoinCol = left->getColumnIndex(col);

    // If ON clause tokens were reversed relative to JOIN(table1, table2), normalize.
    bool leftTokenFromRightTable = false;
    parseQualifiedName(parsedQuery.joinLeftToken, rel, col);
    if (rel == right->tableName)
        leftTokenFromRightTable = true;

    int leftCondCol = leftTokenFromRightTable ? rightJoinCol : leftJoinCol;
    int rightCondCol = leftTokenFromRightTable ? leftJoinCol : rightJoinCol;

    int partitionCount = max(1, (int)BLOCK_COUNT - 1);

    function<int(const vector<int> &)> leftPartitionKey;
    function<int(const vector<int> &)> rightPartitionKey;

    if (parsedQuery.joinConditionType == JOIN_ATTR_EQ)
    {
        leftPartitionKey = [&](const vector<int> &row)
        { return row[leftCondCol]; };
        rightPartitionKey = [&](const vector<int> &row)
        { return row[rightCondCol]; };
    }
    else
    {
        leftPartitionKey = [&](const vector<int> &row)
        { return row[leftCondCol]; };

        if (parsedQuery.joinArithmeticOperator == '+')
        {
            rightPartitionKey = [&](const vector<int> &row)
            { return parsedQuery.joinArithmeticConstant - row[rightCondCol]; };
        }
        else
        {
            rightPartitionKey = [&](const vector<int> &row)
            { return parsedQuery.joinArithmeticConstant + row[rightCondCol]; };
        }
    }

    vector<JoinPartitionMeta> leftPartitions = buildPartitions(*left, parsedQuery.joinResultRelationName + "_L", partitionCount, leftPartitionKey);
    vector<JoinPartitionMeta> rightPartitions = buildPartitions(*right, parsedQuery.joinResultRelationName + "_R", partitionCount, rightPartitionKey);

    vector<pair<bool, int>> projectedColumns; // {true => left table, false => right table}
    vector<string> resultColumns;

    if (parsedQuery.joinHasProjectClause)
    {
        for (const string &token : parsedQuery.joinProjectTokens)
        {
            string tableName, columnName;
            parseQualifiedName(token, tableName, columnName);
            if (tableName == left->tableName)
            {
                projectedColumns.push_back({true, left->getColumnIndex(columnName)});
                resultColumns.push_back(columnName);
            }
            else
            {
                projectedColumns.push_back({false, right->getColumnIndex(columnName)});
                resultColumns.push_back(columnName);
            }
        }
    }
    else
    {
        for (int i = 0; i < (int)left->columns.size(); i++)
        {
            projectedColumns.push_back({true, i});
            resultColumns.push_back(left->columns[i]);
        }
        for (int i = 0; i < (int)right->columns.size(); i++)
        {
            projectedColumns.push_back({false, i});
            resultColumns.push_back(right->columns[i]);
        }
    }

    int whereColumnIndex = -1;
    bool whereOnLeft = true;
    if (parsedQuery.joinHasWhereClause)
    {
        string whereTable, whereColumn;
        parseQualifiedName(parsedQuery.joinWhereToken, whereTable, whereColumn);
        if (whereTable == left->tableName)
        {
            whereOnLeft = true;
            whereColumnIndex = left->getColumnIndex(whereColumn);
        }
        else
        {
            whereOnLeft = false;
            whereColumnIndex = right->getColumnIndex(whereColumn);
        }
    }

    Table *resultTable = new Table(parsedQuery.joinResultRelationName, resultColumns);
    long long matchCount = 0;

    for (int pid = 0; pid < partitionCount; pid++)
    {
        unordered_multimap<int, vector<int>> hashTable;

        JoinPartitionReader leftReader(leftPartitions[pid]);
        vector<int> leftRow;
        while (leftReader.getNext(leftRow))
            hashTable.emplace(leftPartitionKey(leftRow), leftRow);

        JoinPartitionReader rightReader(rightPartitions[pid]);
        vector<int> rightRow;
        while (rightReader.getNext(rightRow))
        {
            int probeKey = rightPartitionKey(rightRow);
            auto range = hashTable.equal_range(probeKey);
            for (auto it = range.first; it != range.second; ++it)
            {
                const vector<int> &candidateLeft = it->second;
                bool joinOk = false;

                if (parsedQuery.joinConditionType == JOIN_ATTR_EQ)
                {
                    joinOk = (candidateLeft[leftCondCol] == rightRow[rightCondCol]);
                }
                else if (parsedQuery.joinArithmeticOperator == '+')
                {
                    joinOk = (candidateLeft[leftCondCol] + rightRow[rightCondCol] == parsedQuery.joinArithmeticConstant);
                }
                else
                {
                    joinOk = (candidateLeft[leftCondCol] - rightRow[rightCondCol] == parsedQuery.joinArithmeticConstant);
                }

                if (!joinOk)
                    continue;

                if (parsedQuery.joinHasWhereClause)
                {
                    int whereValue = whereOnLeft ? candidateLeft[whereColumnIndex] : rightRow[whereColumnIndex];
                    if (!evaluateBinOp(whereValue, parsedQuery.joinWhereConstant, parsedQuery.joinWhereBinaryOperator))
                        continue;
                }

                vector<int> outputRow;
                outputRow.reserve(projectedColumns.size());
                for (auto proj : projectedColumns)
                {
                    if (proj.first)
                        outputRow.push_back(candidateLeft[proj.second]);
                    else
                        outputRow.push_back(rightRow[proj.second]);
                }

                resultTable->writeRow<int>(outputRow);
                matchCount++;
            }
        }
    }

    for (const auto &partition : leftPartitions)
        deleteJoinPartition(partition);
    for (const auto &partition : rightPartitions)
        deleteJoinPartition(partition);

    if (matchCount == 0)
    {
        resultTable->unload();
        delete resultTable;
        return;
    }

    resultTable->blockify();
    tableCatalogue.insertTable(resultTable);
}
