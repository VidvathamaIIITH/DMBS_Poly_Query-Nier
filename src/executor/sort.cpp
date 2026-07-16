/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "global.h"

struct SortRunMeta
{
    string name;
    uint blockCount = 0;
    long long rowCount = 0;
    vector<uint> rowsPerBlock;
};

class SortRunReader
{
public:
    SortRunMeta meta;
    int pageIndex = 0;
    int rowIndex = 0;
    Page page;
    bool valid = false;

    SortRunReader(const SortRunMeta &run) : meta(run)
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

static string stripTrailingSemicolon(string token)
{
    while (!token.empty() && token.back() == ';')
        token.pop_back();
    return token;
}

static bool parseSortDirection(const string &token, SortingStrategy &strategy)
{
    if (token == "ASC")
    {
        strategy = ASC;
        return true;
    }
    if (token == "DESC")
    {
        strategy = DESC;
        return true;
    }
    return false;
}

static bool compareRowsByKeys(const vector<int> &a,
                              const vector<int> &b,
                              const vector<int> &columnIndices,
                              const vector<SortingStrategy> &strategies)
{
    for (int i = 0; i < (int)columnIndices.size(); i++)
    {
        int col = columnIndices[i];
        if (a[col] == b[col])
            continue;
        if (strategies[i] == ASC)
            return a[col] < b[col];
        return a[col] > b[col];
    }
    return false;
}

static void mergeByKeys(vector<vector<int>> &rows,
                        vector<vector<int>> &temp,
                        int left,
                        int mid,
                        int right,
                        const vector<int> &columnIndices,
                        const vector<SortingStrategy> &strategies)
{
    int i = left;
    int j = mid + 1;
    int k = left;

    while (i <= mid && j <= right)
    {
        // Pick left row on ties to preserve stability.
        if (!compareRowsByKeys(rows[j], rows[i], columnIndices, strategies))
            temp[k++] = rows[i++];
        else
            temp[k++] = rows[j++];
    }

    while (i <= mid)
        temp[k++] = rows[i++];
    while (j <= right)
        temp[k++] = rows[j++];

    for (int idx = left; idx <= right; idx++)
        rows[idx] = temp[idx];
}

static void mergeSortByKeys(vector<vector<int>> &rows,
                            vector<vector<int>> &temp,
                            int left,
                            int right,
                            const vector<int> &columnIndices,
                            const vector<SortingStrategy> &strategies)
{
    if (left >= right)
        return;

    int mid = left + (right - left) / 2;
    mergeSortByKeys(rows, temp, left, mid, columnIndices, strategies);
    mergeSortByKeys(rows, temp, mid + 1, right, columnIndices, strategies);
    mergeByKeys(rows, temp, left, mid, right, columnIndices, strategies);
}

static void stableInMemorySortByKeys(vector<vector<int>> &rows,
                                     const vector<int> &columnIndices,
                                     const vector<SortingStrategy> &strategies)
{
    if (rows.size() <= 1)
        return;
    vector<vector<int>> temp(rows.size());
    mergeSortByKeys(rows, temp, 0, (int)rows.size() - 1, columnIndices, strategies);
}

static void deleteSortRun(const SortRunMeta &run)
{
    if (tableCatalogue.isTable(run.name))
    {
        tableCatalogue.deleteTable(run.name);
        return;
    }
    for (uint i = 0; i < run.blockCount; i++)
        bufferManager.deleteFile(run.name, i);
}

static void registerSortRun(const SortRunMeta &run, int columnCount, uint maxRowsPerBlock)
{
    if (run.blockCount == 0)
        return;
    if (tableCatalogue.isTable(run.name))
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
    for (int i = 0; i < columnCount; i++)
        metaTable->columns.push_back("C" + to_string(i));
    tableCatalogue.insertTable(metaTable);
}

static SortRunMeta writeRowsToSortRun(const string &runName,
                                      const vector<vector<int>> &rows,
                                      uint maxRowsPerBlock,
                                      int columnCount)
{
    SortRunMeta run;
    run.name = runName;
    run.rowCount = rows.size();

    if (rows.empty())
        return run;

    int pageIndex = 0;
    for (int i = 0; i < (int)rows.size(); i += (int)maxRowsPerBlock)
    {
        int right = min(i + (int)maxRowsPerBlock, (int)rows.size());
        vector<vector<int>> pageRows(rows.begin() + i, rows.begin() + right);
        bufferManager.writePage(runName, pageIndex, pageRows, pageRows.size());
        run.rowsPerBlock.push_back((uint)pageRows.size());
        pageIndex++;
    }

    run.blockCount = pageIndex;
    registerSortRun(run, columnCount, maxRowsPerBlock);
    return run;
}

static SortRunMeta materializeTableRange(Table &table,
                                         long long startRow,
                                         long long endRow,
                                         const string &prefix)
{
    SortRunMeta out;
    out.name = prefix;
    if (startRow > endRow)
        return out;

    Cursor cursor = table.getCursor();
    vector<vector<int>> pageRows;
    pageRows.reserve(table.maxRowsPerBlock);

    long long rowIndex = 0;
    int pageIndex = 0;
    vector<int> row = cursor.getNext();
    while (!row.empty())
    {
        if (rowIndex >= startRow && rowIndex <= endRow)
        {
            pageRows.push_back(row);
            out.rowCount++;
            if (pageRows.size() == table.maxRowsPerBlock)
            {
                bufferManager.writePage(out.name, pageIndex, pageRows, pageRows.size());
                out.rowsPerBlock.push_back((uint)pageRows.size());
                out.blockCount++;
                pageIndex++;
                pageRows.clear();
            }
        }
        if (rowIndex > endRow)
            break;
        rowIndex++;
        row = cursor.getNext();
    }

    if (!pageRows.empty())
    {
        bufferManager.writePage(out.name, pageIndex, pageRows, pageRows.size());
        out.rowsPerBlock.push_back((uint)pageRows.size());
        out.blockCount++;
    }

    registerSortRun(out, table.columnCount, table.maxRowsPerBlock);

    return out;
}

static vector<SortRunMeta> generateInitialRuns(Table &table,
                                               long long startRow,
                                               long long endRow,
                                               const vector<int> &columnIndices,
                                               const vector<SortingStrategy> &strategies,
                                               const string &prefix)
{
    vector<SortRunMeta> runs;
    if (startRow > endRow)
        return runs;

    long long rowsPerRun = (long long)table.maxRowsPerBlock * max(1, (int)BLOCK_COUNT - 1);
    rowsPerRun = max(1LL, rowsPerRun);

    Cursor cursor = table.getCursor();
    vector<vector<int>> runRows;
    runRows.reserve((size_t)rowsPerRun);

    long long rowIndex = 0;
    int runCounter = 0;
    vector<int> row = cursor.getNext();

    while (!row.empty())
    {
        if (rowIndex >= startRow && rowIndex <= endRow)
        {
            runRows.push_back(row);
            if ((long long)runRows.size() == rowsPerRun)
            {
                stableInMemorySortByKeys(runRows, columnIndices, strategies);
                runs.push_back(writeRowsToSortRun(prefix + "_R" + to_string(runCounter++), runRows, table.maxRowsPerBlock, table.columnCount));
                runRows.clear();
            }
        }

        if (rowIndex > endRow)
            break;

        rowIndex++;
        row = cursor.getNext();
    }

    if (!runRows.empty())
    {
        stableInMemorySortByKeys(runRows, columnIndices, strategies);
        runs.push_back(writeRowsToSortRun(prefix + "_R" + to_string(runCounter++), runRows, table.maxRowsPerBlock, table.columnCount));
    }

    return runs;
}

struct MergeHeapNode
{
    vector<int> row;
    int runOrder;
};

static SortRunMeta mergeRunBatch(const vector<SortRunMeta> &batch,
                                 const vector<int> &columnIndices,
                                 const vector<SortingStrategy> &strategies,
                                 uint maxRowsPerBlock,
                                 const string &outName,
                                 int columnCount)
{
    vector<SortRunReader> readers;
    for (const auto &run : batch)
        readers.emplace_back(run);

    auto cmp = [&](const MergeHeapNode &left, const MergeHeapNode &right)
    {
        if (compareRowsByKeys(right.row, left.row, columnIndices, strategies))
            return true;
        if (compareRowsByKeys(left.row, right.row, columnIndices, strategies))
            return false;
        return left.runOrder > right.runOrder;
    };

    priority_queue<MergeHeapNode, vector<MergeHeapNode>, decltype(cmp)> pq(cmp);
    for (int i = 0; i < (int)readers.size(); i++)
    {
        vector<int> row;
        if (readers[i].getNext(row))
            pq.push({row, i});
    }

    SortRunMeta out;
    out.name = outName;

    int pageIndex = 0;
    vector<vector<int>> pageRows;
    pageRows.reserve(maxRowsPerBlock);

    while (!pq.empty())
    {
        MergeHeapNode node = pq.top();
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

    registerSortRun(out, columnCount, maxRowsPerBlock);

    return out;
}

static SortRunMeta externalSortRange(Table &table,
                                     long long startRow,
                                     long long endRow,
                                     const vector<int> &columnIndices,
                                     const vector<SortingStrategy> &strategies,
                                     const string &prefix)
{
    SortRunMeta empty;
    empty.name = prefix + "_EMPTY";

    vector<SortRunMeta> runs = generateInitialRuns(table, startRow, endRow, columnIndices, strategies, prefix + "_P0");
    if (runs.empty())
        return empty;

    if (runs.size() == 1)
        return runs.front();

    int fanIn = max(2, (int)BLOCK_COUNT - 1);
    int pass = 1;
    while (runs.size() > 1)
    {
        vector<SortRunMeta> nextRuns;
        for (int i = 0; i < (int)runs.size(); i += fanIn)
        {
            int right = min(i + fanIn, (int)runs.size());
            vector<SortRunMeta> batch(runs.begin() + i, runs.begin() + right);
            string outName = prefix + "_P" + to_string(pass) + "_R" + to_string(nextRuns.size());
            nextRuns.push_back(mergeRunBatch(batch, columnIndices, strategies, table.maxRowsPerBlock, outName, table.columnCount));
            for (const auto &run : batch)
                deleteSortRun(run);
        }
        runs = nextRuns;
        pass++;
    }

    return runs.front();
}

static void rewriteTableFromSegments(Table &table, const vector<SortRunMeta> &segments)
{
    uint oldBlocks = table.blockCount;

    // Delete old pages before writing the in-place sorted content.
    for (uint i = 0; i < oldBlocks; i++)
        bufferManager.deleteFile(table.tableName, i);

    table.rowsPerBlockCount.clear();
    table.blockCount = 0;

    int pageIndex = 0;
    vector<vector<int>> pageRows;
    pageRows.reserve(table.maxRowsPerBlock);

    for (const auto &segment : segments)
    {
        SortRunReader reader(segment);
        vector<int> row;
        while (reader.getNext(row))
        {
            pageRows.push_back(row);
            if (pageRows.size() == table.maxRowsPerBlock)
            {
                bufferManager.writePage(table.tableName, pageIndex, pageRows, pageRows.size());
                table.rowsPerBlockCount.push_back((uint)pageRows.size());
                table.blockCount++;
                pageIndex++;
                pageRows.clear();
            }
        }
    }

    if (!pageRows.empty())
    {
        bufferManager.writePage(table.tableName, pageIndex, pageRows, pageRows.size());
        table.rowsPerBlockCount.push_back((uint)pageRows.size());
        table.blockCount++;
    }
}

/**
 * @brief File contains method to process Phase 2 SORT command.
 *
 * syntax:
 * SORT table_name BY c1, c2 IN ASC, DESC [TOP X] [BOTTOM Y]
 */
bool syntacticParseSORT()
{
    logger.log("syntacticParseSORT");
    if (tokenizedQuery.size() < 6 || tokenizedQuery[0] != "SORT" || tokenizedQuery[2] != "BY")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = SORT;
    parsedQuery.sortTableName = tokenizedQuery[1];

    int index = 3;
    while (index < (int)tokenizedQuery.size() && tokenizedQuery[index] != "IN")
    {
        parsedQuery.sortColumnNames.push_back(stripTrailingSemicolon(tokenizedQuery[index]));
        index++;
    }

    if (parsedQuery.sortColumnNames.empty() || index >= (int)tokenizedQuery.size() || tokenizedQuery[index] != "IN")
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    index++;
    while (index < (int)tokenizedQuery.size())
    {
        string token = stripTrailingSemicolon(tokenizedQuery[index]);
        if (token == "TOP" || token == "BOTTOM")
            break;

        SortingStrategy strategy;
        if (!parseSortDirection(token, strategy))
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
        parsedQuery.sortStrategies.push_back(strategy);
        index++;
    }

    if (parsedQuery.sortStrategies.size() != parsedQuery.sortColumnNames.size())
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    while (index < (int)tokenizedQuery.size())
    {
        string token = stripTrailingSemicolon(tokenizedQuery[index]);
        if (token == "TOP")
        {
            if (parsedQuery.hasTopClause || index + 1 >= (int)tokenizedQuery.size())
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }
            parsedQuery.hasTopClause = true;
            parsedQuery.sortTopRowCount = stoll(stripTrailingSemicolon(tokenizedQuery[index + 1]));
            index += 2;
        }
        else if (token == "BOTTOM")
        {
            if (parsedQuery.hasBottomClause || index + 1 >= (int)tokenizedQuery.size())
            {
                cout << "SYNTAX ERROR" << endl;
                return false;
            }
            parsedQuery.hasBottomClause = true;
            parsedQuery.sortBottomRowCount = stoll(stripTrailingSemicolon(tokenizedQuery[index + 1]));
            index += 2;
        }
        else
        {
            cout << "SYNTAX ERROR" << endl;
            return false;
        }
    }

    return true;
}

bool semanticParseSORT()
{
    logger.log("semanticParseSORT");

    if (!tableCatalogue.isTable(parsedQuery.sortTableName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    for (const string &column : parsedQuery.sortColumnNames)
    {
        if (!tableCatalogue.isColumnFromTable(column, parsedQuery.sortTableName))
        {
            cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
            return false;
        }
    }

    if ((parsedQuery.hasTopClause && parsedQuery.sortTopRowCount < 0) ||
        (parsedQuery.hasBottomClause && parsedQuery.sortBottomRowCount < 0))
    {
        cout << "SEMANTIC ERROR" << endl;
        return false;
    }

    Table *table = tableCatalogue.getTable(parsedQuery.sortTableName);
    if ((parsedQuery.hasTopClause && parsedQuery.sortTopRowCount > table->rowCount) ||
        (parsedQuery.hasBottomClause && parsedQuery.sortBottomRowCount > table->rowCount))
    {
        cout << "SEMANTIC ERROR" << endl;
        return false;
    }

    if (parsedQuery.hasTopClause && parsedQuery.hasBottomClause &&
        parsedQuery.sortTopRowCount + parsedQuery.sortBottomRowCount > table->rowCount)
    {
        cout << "SEMANTIC ERROR" << endl;
        return false;
    }

    return true;
}

void executeSORT()
{
    logger.log("executeSORT");

    Table *table = tableCatalogue.getTable(parsedQuery.sortTableName);
    vector<int> columnIndices;
    for (const string &column : parsedQuery.sortColumnNames)
        columnIndices.push_back(table->getColumnIndex(column));

    long long totalRows = table->rowCount;
    vector<SortRunMeta> outputSegments;

    if (!parsedQuery.hasTopClause && !parsedQuery.hasBottomClause)
    {
        SortRunMeta sorted = externalSortRange(*table, 0, totalRows - 1, columnIndices, parsedQuery.sortStrategies,
                                               table->tableName + "_SORT_ALL");
        outputSegments.push_back(sorted);
    }
    else
    {
        long long topCount = parsedQuery.hasTopClause ? parsedQuery.sortTopRowCount : 0;
        long long bottomCount = parsedQuery.hasBottomClause ? parsedQuery.sortBottomRowCount : 0;

        if (topCount > 0)
        {
            SortRunMeta topSorted = externalSortRange(*table, 0, topCount - 1, columnIndices, parsedQuery.sortStrategies,
                                                      table->tableName + "_SORT_TOP");
            outputSegments.push_back(topSorted);
        }

        long long middleStart = topCount;
        long long middleEnd = totalRows - bottomCount - 1;
        if (middleStart <= middleEnd)
        {
            SortRunMeta middle = materializeTableRange(*table, middleStart, middleEnd, table->tableName + "_SORT_MIDDLE");
            outputSegments.push_back(middle);
        }

        if (bottomCount > 0)
        {
            SortRunMeta bottomSorted = externalSortRange(*table, totalRows - bottomCount, totalRows - 1,
                                                         columnIndices, parsedQuery.sortStrategies,
                                                         table->tableName + "_SORT_BOTTOM");
            outputSegments.push_back(bottomSorted);
        }
    }

    rewriteTableFromSegments(*table, outputSegments);

    for (const auto &segment : outputSegments)
        deleteSortRun(segment);
}
