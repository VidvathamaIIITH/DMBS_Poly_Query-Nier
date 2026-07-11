#include "global.h"

struct AggregateMetric
{
    long long sum = 0;
    long long count = 0;
    int minValue = 0;
    int maxValue = 0;
    bool initialized = false;
};

struct GroupRunMeta
{
    string name;
    uint blockCount = 0;
    long long rowCount = 0;
    vector<uint> rowsPerBlock;
};

class GroupRunReader
{
public:
    GroupRunMeta meta;
    int pageIndex = 0;
    int rowIndex = 0;
    Page page;
    bool valid = false;

    GroupRunReader(const GroupRunMeta &run) : meta(run)
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

static string stripSemicolonGroup(string token)
{
    while (!token.empty() && token.back() == ';')
        token.pop_back();
    return token;
}

static string aggregateTypeToString(AggregateType type)
{
    switch (type)
    {
    case AGG_MAX:
        return "MAX";
    case AGG_MIN:
        return "MIN";
    case AGG_COUNT:
        return "COUNT";
    case AGG_SUM:
        return "SUM";
    case AGG_AVG:
        return "AVG";
    default:
        return "";
    }
}

static bool parseAggregateExpression(const string &token, AggregateExpression &expr)
{
    string cleaned = stripSemicolonGroup(token);
    size_t open = cleaned.find('(');
    size_t close = cleaned.find(')');
    if (open == string::npos || close == string::npos || close <= open + 1)
        return false;

    string aggName = cleaned.substr(0, open);
    string column = cleaned.substr(open + 1, close - open - 1);

    if (aggName == "MAX")
        expr.aggregateType = AGG_MAX;
    else if (aggName == "MIN")
        expr.aggregateType = AGG_MIN;
    else if (aggName == "COUNT")
        expr.aggregateType = AGG_COUNT;
    else if (aggName == "SUM")
        expr.aggregateType = AGG_SUM;
    else if (aggName == "AVG")
        expr.aggregateType = AGG_AVG;
    else
        return false;

    expr.columnName = column;
    return true;
}

static bool isValidHavingOperator(const string &token, BinaryOperator &op)
{
    if (token == "==")
        op = EQUAL;
    else if (token == "!=")
        op = NOT_EQUAL;
    else if (token == ">")
        op = GREATER_THAN;
    else if (token == ">=" || token == "=>")
        op = GEQ;
    else if (token == "<")
        op = LESS_THAN;
    else if (token == "<=" || token == "=<")
        op = LEQ;
    else
        return false;
    return true;
}

bool syntacticParseGROUP()
{
    logger.log("syntacticParseGROUP");

    int arrowIndex = -1;
    for (int i = 0; i < (int)tokenizedQuery.size(); i++)
    {
        if (tokenizedQuery[i] == "<-")
        {
            arrowIndex = i;
            break;
        }
    }

    if (arrowIndex <= 0 || arrowIndex + 1 >= (int)tokenizedQuery.size())
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    for (int i = 0; i < arrowIndex; i++)
        parsedQuery.groupResultRelationNames.push_back(stripSemicolonGroup(tokenizedQuery[i]));

    if (tokenizedQuery[arrowIndex + 1] != "GROUP" || arrowIndex + 2 >= (int)tokenizedQuery.size() || tokenizedQuery[arrowIndex + 2] != "BY")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = GROUP;

    int idx = arrowIndex + 3;
    while (idx < (int)tokenizedQuery.size() && tokenizedQuery[idx] != "FROM")
    {
        parsedQuery.groupByColumns.push_back(stripSemicolonGroup(tokenizedQuery[idx]));
        idx++;
    }

    if (parsedQuery.groupByColumns.empty() || idx + 1 >= (int)tokenizedQuery.size() || tokenizedQuery[idx] != "FROM")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.groupSourceRelationName = stripSemicolonGroup(tokenizedQuery[idx + 1]);
    idx += 2;

    if (idx >= (int)tokenizedQuery.size() || tokenizedQuery[idx] != "HAVING" || idx + 3 >= (int)tokenizedQuery.size())
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    if (!parseAggregateExpression(stripSemicolonGroup(tokenizedQuery[idx + 1]), parsedQuery.groupHavingLeft))
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    if (!isValidHavingOperator(stripSemicolonGroup(tokenizedQuery[idx + 2]), parsedQuery.groupHavingBinaryOperator))
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    string havingRhs = stripSemicolonGroup(tokenizedQuery[idx + 3]);
    regex numeric("[0-9]+");
    if (regex_match(havingRhs, numeric))
    {
        parsedQuery.groupHavingRightIsConstant = true;
        parsedQuery.groupHavingRightConstant = stoi(havingRhs);
    }
    else
    {
        parsedQuery.groupHavingRightIsConstant = false;
        if (!parseAggregateExpression(havingRhs, parsedQuery.groupHavingRightAggregate))
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }

    idx += 4;
    if (idx >= (int)tokenizedQuery.size() || tokenizedQuery[idx] != "RETURN")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    idx++;
    while (idx < (int)tokenizedQuery.size())
    {
        AggregateExpression expr;
        if (!parseAggregateExpression(stripSemicolonGroup(tokenizedQuery[idx]), expr))
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.groupReturnAggregates.push_back(expr);
        idx++;
    }

    if (parsedQuery.groupReturnAggregates.empty() ||
        parsedQuery.groupResultRelationNames.size() != parsedQuery.groupByColumns.size() ||
        parsedQuery.groupResultRelationNames.size() != parsedQuery.groupReturnAggregates.size())
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    return true;
}

bool semanticParseGROUP()
{
    logger.log("semanticParseGROUP");

    if (!tableCatalogue.isTable(parsedQuery.groupSourceRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    Table *table = tableCatalogue.getTable(parsedQuery.groupSourceRelationName);

    for (const string &resultName : parsedQuery.groupResultRelationNames)
    {
        if (tableCatalogue.isTable(resultName))
        {
            cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
            return false;
        }
    }

    for (const string &groupCol : parsedQuery.groupByColumns)
    {
        if (!table->isColumn(groupCol))
        {
            cout << "SEMANTIC ERROR: Group By column doesn't exist in relation" << endl;
            return false;
        }
    }

    auto validateHavingAggregate = [&](const AggregateExpression &expr) -> bool
    {
        if (expr.aggregateType == AGG_COUNT && expr.columnName == "*")
            return true;
        return table->isColumn(expr.columnName);
    };

    if (!validateHavingAggregate(parsedQuery.groupHavingLeft))
    {
        cout << "SEMANTIC ERROR: Having column doesn't exist in relation" << endl;
        return false;
    }

    if (!parsedQuery.groupHavingRightIsConstant && !validateHavingAggregate(parsedQuery.groupHavingRightAggregate))
    {
        cout << "SEMANTIC ERROR: Having column doesn't exist in relation" << endl;
        return false;
    }

    for (const auto &retExpr : parsedQuery.groupReturnAggregates)
    {
        if (!(retExpr.aggregateType == AGG_COUNT && retExpr.columnName == "*") && !table->isColumn(retExpr.columnName))
        {
            cout << "SEMANTIC ERROR: Return column doesn't exist in relation" << endl;
            return false;
        }
    }

    return true;
}

static void updateAggregateMetric(AggregateMetric &metric, AggregateType type, int value)
{
    if (!metric.initialized)
    {
        metric.initialized = true;
        metric.minValue = value;
        metric.maxValue = value;
    }

    metric.count++;
    metric.sum += value;
    metric.minValue = min(metric.minValue, value);
    metric.maxValue = max(metric.maxValue, value);
}

static long long getAggregateMetricValue(const AggregateMetric &metric, AggregateType type)
{
    if (!metric.initialized && type != AGG_COUNT)
        return 0;

    switch (type)
    {
    case AGG_MAX:
        return metric.maxValue;
    case AGG_MIN:
        return metric.minValue;
    case AGG_COUNT:
        return metric.count;
    case AGG_SUM:
        return metric.sum;
    case AGG_AVG:
        if (metric.count == 0)
            return 0;
        // AVG for Phase 2 is rounded up.
        return (metric.sum + metric.count - 1) / metric.count;
    default:
        return 0;
    }
}

static string buildReturnColumnName(const AggregateExpression &expr)
{
    if (expr.aggregateType == AGG_COUNT && expr.columnName == "*")
        return "COUNT";
    return aggregateTypeToString(expr.aggregateType) + expr.columnName;
}

static bool compareRowsByGroupColumn(const vector<int> &left,
                                     const vector<int> &right,
                                     int groupColumnIndex)
{
    return left[groupColumnIndex] < right[groupColumnIndex];
}

static void mergeGroupRows(vector<vector<int>> &rows,
                           vector<vector<int>> &temp,
                           int left,
                           int mid,
                           int right,
                           int groupColumnIndex)
{
    int first = left;
    int second = mid + 1;
    int dest = left;

    while (first <= mid && second <= right)
    {
        if (!compareRowsByGroupColumn(rows[second], rows[first], groupColumnIndex))
            temp[dest++] = rows[first++];
        else
            temp[dest++] = rows[second++];
    }

    while (first <= mid)
        temp[dest++] = rows[first++];
    while (second <= right)
        temp[dest++] = rows[second++];

    for (int idx = left; idx <= right; idx++)
        rows[idx] = temp[idx];
}

static void mergeSortGroupRows(vector<vector<int>> &rows,
                               vector<vector<int>> &temp,
                               int left,
                               int right,
                               int groupColumnIndex)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;
    mergeSortGroupRows(rows, temp, left, mid, groupColumnIndex);
    mergeSortGroupRows(rows, temp, mid + 1, right, groupColumnIndex);
    mergeGroupRows(rows, temp, left, mid, right, groupColumnIndex);
}

static void stableSortRowsByGroupColumn(vector<vector<int>> &rows, int groupColumnIndex)
{
    if (rows.size() <= 1)
        return;
    vector<vector<int>> temp(rows.size());
    mergeSortGroupRows(rows, temp, 0, (int)rows.size() - 1, groupColumnIndex);
}

static void registerGroupRun(const GroupRunMeta &run, int columnCount, uint maxRowsPerBlock)
{
    if (run.blockCount == 0 || tableCatalogue.isTable(run.name))
        return;

    Table *metaTable = new Table();
    metaTable->tableName = run.name;
    metaTable->sourceFileName = "";
    metaTable->columnCount = columnCount;
    metaTable->maxRowsPerBlock = maxRowsPerBlock;
    metaTable->rowCount = run.rowCount;
    metaTable->blockCount = run.blockCount;
    metaTable->rowsPerBlockCount = run.rowsPerBlock;
    metaTable->columns.clear();
    for (int idx = 0; idx < columnCount; idx++)
        metaTable->columns.push_back("C" + to_string(idx));
    tableCatalogue.insertTable(metaTable);
}

static void deleteGroupRun(const GroupRunMeta &run)
{
    if (tableCatalogue.isTable(run.name))
    {
        tableCatalogue.deleteTable(run.name);
        return;
    }

    for (uint idx = 0; idx < run.blockCount; idx++)
        bufferManager.deleteFile(run.name, idx);
}

static GroupRunMeta writeRowsToGroupRun(const string &runName,
                                        const vector<vector<int>> &rows,
                                        uint maxRowsPerBlock,
                                        int columnCount)
{
    GroupRunMeta run;
    run.name = runName;
    run.rowCount = rows.size();
    if (rows.empty())
        return run;

    int pageIndex = 0;
    for (int idx = 0; idx < (int)rows.size(); idx += (int)maxRowsPerBlock)
    {
        int end = min(idx + (int)maxRowsPerBlock, (int)rows.size());
        vector<vector<int>> pageRows(rows.begin() + idx, rows.begin() + end);
        bufferManager.writePage(runName, pageIndex, pageRows, pageRows.size());
        run.rowsPerBlock.push_back((uint)pageRows.size());
        pageIndex++;
    }

    run.blockCount = pageIndex;
    registerGroupRun(run, columnCount, maxRowsPerBlock);
    return run;
}

static vector<GroupRunMeta> generateInitialGroupRuns(Table &table,
                                                     int groupColumnIndex,
                                                     const string &prefix)
{
    vector<GroupRunMeta> runs;
    long long rowsPerRun = (long long)table.maxRowsPerBlock * max(1, (int)BLOCK_COUNT - 1);
    rowsPerRun = max(1LL, rowsPerRun);

    Cursor cursor = table.getCursor();
    vector<vector<int>> runRows;
    runRows.reserve((size_t)rowsPerRun);
    int runCounter = 0;

    vector<int> row = cursor.getNext();
    while (!row.empty())
    {
        runRows.push_back(row);
        if ((long long)runRows.size() == rowsPerRun)
        {
            stableSortRowsByGroupColumn(runRows, groupColumnIndex);
            runs.push_back(writeRowsToGroupRun(prefix + "_R" + to_string(runCounter++), runRows,
                                               table.maxRowsPerBlock, table.columnCount));
            runRows.clear();
        }
        row = cursor.getNext();
    }

    if (!runRows.empty())
    {
        stableSortRowsByGroupColumn(runRows, groupColumnIndex);
        runs.push_back(writeRowsToGroupRun(prefix + "_R" + to_string(runCounter++), runRows,
                                           table.maxRowsPerBlock, table.columnCount));
    }

    return runs;
}

struct GroupMergeNode
{
    vector<int> row;
    int runOrder;
};

static GroupRunMeta mergeGroupRunBatch(const vector<GroupRunMeta> &batch,
                                       int groupColumnIndex,
                                       uint maxRowsPerBlock,
                                       const string &outName,
                                       int columnCount)
{
    vector<GroupRunReader> readers;
    for (const auto &run : batch)
        readers.emplace_back(run);

    auto cmp = [&](const GroupMergeNode &left, const GroupMergeNode &right)
    {
        if (compareRowsByGroupColumn(right.row, left.row, groupColumnIndex))
            return true;
        if (compareRowsByGroupColumn(left.row, right.row, groupColumnIndex))
            return false;
        return left.runOrder > right.runOrder;
    };

    priority_queue<GroupMergeNode, vector<GroupMergeNode>, decltype(cmp)> pq(cmp);
    for (int idx = 0; idx < (int)readers.size(); idx++)
    {
        vector<int> row;
        if (readers[idx].getNext(row))
            pq.push({row, idx});
    }

    GroupRunMeta out;
    out.name = outName;
    int pageIndex = 0;
    vector<vector<int>> pageRows;
    pageRows.reserve(maxRowsPerBlock);

    while (!pq.empty())
    {
        GroupMergeNode node = pq.top();
        pq.pop();

        pageRows.push_back(node.row);
        out.rowCount++;
        if (pageRows.size() == maxRowsPerBlock)
        {
            bufferManager.writePage(out.name, pageIndex, pageRows, pageRows.size());
            out.rowsPerBlock.push_back((uint)pageRows.size());
            out.blockCount++;
            pageIndex++;
            pageRows.clear();
        }

        vector<int> nextRow;
        if (readers[node.runOrder].getNext(nextRow))
            pq.push({nextRow, node.runOrder});
    }

    if (!pageRows.empty())
    {
        bufferManager.writePage(out.name, pageIndex, pageRows, pageRows.size());
        out.rowsPerBlock.push_back((uint)pageRows.size());
        out.blockCount++;
    }

    registerGroupRun(out, columnCount, maxRowsPerBlock);
    return out;
}

static GroupRunMeta externalSortTableByGroupColumn(Table &table,
                                                   int groupColumnIndex,
                                                   const string &prefix)
{
    GroupRunMeta empty;
    empty.name = prefix + "_EMPTY";

    vector<GroupRunMeta> runs = generateInitialGroupRuns(table, groupColumnIndex, prefix + "_P0");
    if (runs.empty())
        return empty;
    if (runs.size() == 1)
        return runs.front();

    int fanIn = max(2, (int)BLOCK_COUNT - 1);
    int pass = 1;
    while (runs.size() > 1)
    {
        vector<GroupRunMeta> nextRuns;
        for (int idx = 0; idx < (int)runs.size(); idx += fanIn)
        {
            int end = min(idx + fanIn, (int)runs.size());
            vector<GroupRunMeta> batch(runs.begin() + idx, runs.begin() + end);
            string outName = prefix + "_P" + to_string(pass) + "_R" + to_string(nextRuns.size());
            nextRuns.push_back(mergeGroupRunBatch(batch, groupColumnIndex, table.maxRowsPerBlock,
                                                  outName, table.columnCount));
            for (const auto &run : batch)
                deleteGroupRun(run);
        }
        runs = nextRuns;
        pass++;
    }

    return runs.front();
}

static void updateMetricsForRow(vector<AggregateMetric> &metrics,
                                const vector<AggregateExpression> &neededExpressions,
                                const vector<int> &expressionColumnIndex,
                                const vector<int> &row)
{
    for (int idx = 0; idx < (int)neededExpressions.size(); idx++)
    {
        if (neededExpressions[idx].aggregateType == AGG_COUNT && neededExpressions[idx].columnName == "*")
        {
            metrics[idx].count++;
            continue;
        }
        int value = row[expressionColumnIndex[idx]];
        updateAggregateMetric(metrics[idx], neededExpressions[idx].aggregateType, value);
    }
}

static bool flushCompletedGroup(Table *resultTable,
                                int groupValue,
                                vector<AggregateMetric> &metrics,
                                const AggregateExpression &havingLeft,
                                BinaryOperator havingOperator,
                                bool havingRightIsConstant,
                                int havingRightConstant,
                                const AggregateExpression &havingRightAggregate,
                                const AggregateExpression &returnExpr,
                                long long &outputRows)
{
    if (metrics.empty())
        return false;

    int havingRightMetricIndex = havingRightIsConstant ? -1 : 1;
    int returnMetricIndex = havingRightIsConstant ? 1 : 2;

    long long leftValue = getAggregateMetricValue(metrics[0], havingLeft.aggregateType);
    long long rightValue = havingRightIsConstant
                               ? havingRightConstant
                               : getAggregateMetricValue(metrics[havingRightMetricIndex], havingRightAggregate.aggregateType);

    if (!evaluateBinOp((int)leftValue, (int)rightValue, havingOperator))
        return false;

    long long returnValue = getAggregateMetricValue(metrics[returnMetricIndex], returnExpr.aggregateType);
    resultTable->writeRow<int>({groupValue, (int)returnValue});
    outputRows++;
    return true;
}

void executeGROUP()
{
    logger.log("executeGROUP");

    Table *table = tableCatalogue.getTable(parsedQuery.groupSourceRelationName);

    for (int groupIdx = 0; groupIdx < (int)parsedQuery.groupByColumns.size(); groupIdx++)
    {
        string resultName = parsedQuery.groupResultRelationNames[groupIdx];
        string groupColumn = parsedQuery.groupByColumns[groupIdx];
        AggregateExpression returnExpr = parsedQuery.groupReturnAggregates[groupIdx];

        int groupColumnIndex = table->getColumnIndex(groupColumn);

        vector<AggregateExpression> neededExpressions;
        neededExpressions.push_back(parsedQuery.groupHavingLeft);
        if (!parsedQuery.groupHavingRightIsConstant)
            neededExpressions.push_back(parsedQuery.groupHavingRightAggregate);
        neededExpressions.push_back(returnExpr);

        vector<int> expressionColumnIndex(neededExpressions.size(), -1);
        for (int i = 0; i < (int)neededExpressions.size(); i++)
        {
            if (!(neededExpressions[i].aggregateType == AGG_COUNT && neededExpressions[i].columnName == "*"))
                expressionColumnIndex[i] = table->getColumnIndex(neededExpressions[i].columnName);
        }

        vector<string> resultColumns = {groupColumn, buildReturnColumnName(returnExpr)};
        Table *resultTable = new Table(resultName, resultColumns);

        GroupRunMeta sortedRun = externalSortTableByGroupColumn(*table, groupColumnIndex,
                                                                resultName + "_GROUPSORT");

        long long outputRows = 0;
        if (sortedRun.blockCount > 0)
        {
            GroupRunReader reader(sortedRun);
            vector<int> row;
            vector<AggregateMetric> currentMetrics;
            int currentGroupValue = 0;
            bool hasCurrentGroup = false;

            while (reader.getNext(row))
            {
                int rowGroupValue = row[groupColumnIndex];
                if (!hasCurrentGroup)
                {
                    currentGroupValue = rowGroupValue;
                    currentMetrics = vector<AggregateMetric>(neededExpressions.size());
                    hasCurrentGroup = true;
                }
                else if (rowGroupValue != currentGroupValue)
                {
                    flushCompletedGroup(resultTable,
                                        currentGroupValue,
                                        currentMetrics,
                                        parsedQuery.groupHavingLeft,
                                        parsedQuery.groupHavingBinaryOperator,
                                        parsedQuery.groupHavingRightIsConstant,
                                        parsedQuery.groupHavingRightConstant,
                                        parsedQuery.groupHavingRightAggregate,
                                        returnExpr,
                                        outputRows);
                    currentGroupValue = rowGroupValue;
                    currentMetrics = vector<AggregateMetric>(neededExpressions.size());
                }

                updateMetricsForRow(currentMetrics, neededExpressions, expressionColumnIndex, row);
            }

            if (hasCurrentGroup)
            {
                flushCompletedGroup(resultTable,
                                    currentGroupValue,
                                    currentMetrics,
                                    parsedQuery.groupHavingLeft,
                                    parsedQuery.groupHavingBinaryOperator,
                                    parsedQuery.groupHavingRightIsConstant,
                                    parsedQuery.groupHavingRightConstant,
                                    parsedQuery.groupHavingRightAggregate,
                                    returnExpr,
                                    outputRows);
            }

            deleteGroupRun(sortedRun);
        }

        if (outputRows == 0)
        {
            resultTable->unload();
            delete resultTable;
            continue;
        }

        resultTable->blockify();
        tableCatalogue.insertTable(resultTable);
    }
}
