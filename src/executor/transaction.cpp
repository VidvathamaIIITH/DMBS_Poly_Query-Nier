/*
 * ============================================================================
 *  PolyRA - Multi-Model Relational Algebra Engine
 *  Watermark : vidvathamaiiith
 *  Maintainer: vidvathamaiiith
 * ============================================================================
 */
#include "global.h"

namespace
{
    enum TxState
    {
        TX_ACTIVE,
        TX_ABORTED,
        TX_COMMITTED
    };

    enum OpType
    {
        OP_BEGIN,
        OP_READ,
        OP_WRITE,
        OP_COMMIT,
        OP_INVALID
    };

    enum LockMode
    {
        LOCK_SHARED,
        LOCK_EXCLUSIVE
    };

    struct ScheduleOp
    {
        OpType type = OP_INVALID;
        string txId = "";
        string tableName = "";
        int pageNumber = -1;
    };

    struct WaitingRequest
    {
        string txId = "";
        string pageKey = "";
        string tableName = "";
        int pageNumber = -1;
        OpType opType = OP_INVALID;
        LockMode lockMode = LOCK_SHARED;
    };

    struct LockEntry
    {
        unordered_set<string> sharedOwners;
        string exclusiveOwner = "";
    };

    struct TransactionState
    {
        string txId = "";
        long long timestamp = -1;
        TxState state = TX_ACTIVE;
        bool waiting = false;
        bool commitDeferred = false;
        unordered_set<string> heldPages;
        vector<string> heldPagesOrder;
        vector<ScheduleOp> replayProgram;
    };

    class LockSimulator
    {
    private:
        unordered_map<string, TransactionState> transactions;
        unordered_map<string, LockEntry> lockTable;
        vector<WaitingRequest> waitingRequests;
        vector<string> restartOrder;
        ofstream fout;
        long long clockTime = 0;
        size_t distinctTransactionCount = 0;

        static string makePageKey(const string &tableName, int pageNumber)
        {
            return tableName + "#" + to_string(pageNumber);
        }

        static vector<string> tokenizeLine(const string &line)
        {
            vector<string> tokens;
            string token;
            stringstream ss(line);
            while (ss >> token)
                tokens.push_back(token);
            return tokens;
        }

        void writeLog(const string &line)
        {
            fout << line << "\n";
        }

        bool tableAndPageValid(const string &tableName, int pageNumber)
        {
            if (tableCatalogue.isTable(tableName))
            {
                Table *table = tableCatalogue.getTable(tableName);
                if (pageNumber < 0 || pageNumber >= (int)table->blockCount)
                {
                    string err = "SEMANTIC ERROR: Page doesn't exist";
                    cout << err << endl;
                    writeLog(err);
                    return false;
                }
                return true;
            }
            else if (matrixCatalogue.isMatrix(tableName))
            {
                Matrix *matrix = matrixCatalogue.getMatrix(tableName);
                if (pageNumber < 0 || pageNumber >= (int)matrix->blockCount)
                {
                    string err = "SEMANTIC ERROR: Page doesn't exist";
                    cout << err << endl;
                    writeLog(err);
                    return false;
                }
                return true;
            }
            else
            {
                string err = "SEMANTIC ERROR: Relation doesn't exist";
                cout << err << endl;
                writeLog(err);
                return false;
            }
        }

        bool hasConflictingOwner(const TransactionState &tx, const LockEntry &lockEntry, LockMode lockMode)
        {
            if (lockMode == LOCK_SHARED)
            {
                if (!lockEntry.exclusiveOwner.empty() && lockEntry.exclusiveOwner != tx.txId)
                    return true;
                return false;
            }

            if (!lockEntry.exclusiveOwner.empty() && lockEntry.exclusiveOwner != tx.txId)
                return true;
            for (const string &owner : lockEntry.sharedOwners)
            {
                if (owner != tx.txId)
                    return true;
            }
            return false;
        }

        vector<string> getBlockingOwners(const TransactionState &tx, const LockEntry &lockEntry, LockMode lockMode)
        {
            vector<string> blockers;
            if (lockMode == LOCK_SHARED)
            {
                if (!lockEntry.exclusiveOwner.empty() && lockEntry.exclusiveOwner != tx.txId)
                    blockers.push_back(lockEntry.exclusiveOwner);
                return blockers;
            }

            if (!lockEntry.exclusiveOwner.empty() && lockEntry.exclusiveOwner != tx.txId)
                blockers.push_back(lockEntry.exclusiveOwner);

            for (const string &owner : lockEntry.sharedOwners)
            {
                if (owner != tx.txId)
                    blockers.push_back(owner);
            }
            return blockers;
        }

        bool isOlderThanAllBlockers(const TransactionState &tx, const vector<string> &blockers)
        {
            for (const string &blocker : blockers)
            {
                if (!transactions.count(blocker))
                    continue;
                if (tx.timestamp > transactions[blocker].timestamp)
                    return false;
            }
            return true;
        }

        void grantLock(TransactionState &tx, const string &pageKey, LockMode lockMode)
        {
            LockEntry &entry = lockTable[pageKey];
            if (lockMode == LOCK_SHARED)
            {
                entry.sharedOwners.insert(tx.txId);
                if (tx.heldPages.insert(pageKey).second)
                    tx.heldPagesOrder.push_back(pageKey);
                return;
            }

            entry.sharedOwners.erase(tx.txId);
            entry.exclusiveOwner = tx.txId;
            if (tx.heldPages.insert(pageKey).second)
                tx.heldPagesOrder.push_back(pageKey);
        }

        void releaseAllLocks(TransactionState &tx)
        {
            for (const string &pageKey : tx.heldPagesOrder)
            {
                if (!tx.heldPages.count(pageKey))
                    continue;

                size_t splitPos = pageKey.find('#');
                string tableName = pageKey.substr(0, splitPos);
                int pageNumber = stoi(pageKey.substr(splitPos + 1));

                LockEntry &entry = lockTable[pageKey];
                entry.sharedOwners.erase(tx.txId);
                if (entry.exclusiveOwner == tx.txId)
                    entry.exclusiveOwner = "";

                writeLog("unlock(" + tableName + " " + to_string(pageNumber) + ") by " + tx.txId);
            }
            tx.heldPages.clear();
            tx.heldPagesOrder.clear();
        }

        void abortTransaction(const string &txId)
        {
            TransactionState &tx = transactions[txId];
            tx.state = TX_ABORTED;
            tx.waiting = false;
            tx.commitDeferred = false;

            writeLog("ABORT " + txId);
            writeLog("ROLLBACK " + txId);
            releaseAllLocks(tx);
            restartOrder.push_back(txId);

            vector<WaitingRequest> nextWaiting;
            for (const WaitingRequest &req : waitingRequests)
            {
                if (req.txId != txId)
                    nextWaiting.push_back(req);
            }
            waitingRequests.swap(nextWaiting);

            tryGrantWaitingRequests();
        }

        void executeGrantedOperation(const WaitingRequest &req)
        {
            TransactionState &tx = transactions[req.txId];
            if (tx.state != TX_ACTIVE)
                return;

            clockTime++;
            if (req.opType == OP_READ)
            {
                if (tableCatalogue.isTable(req.tableName)) {
                    Cursor cursor(req.tableName, req.pageNumber);
                    cursor.getNext();
                } else if (matrixCatalogue.isMatrix(req.tableName)) {
                    Matrix* matrix = matrixCatalogue.getMatrix(req.tableName);
                    matrixBufferManager.getPage(req.tableName, req.pageNumber, matrix->dimension);
                }
                writeLog("READ " + req.txId + " " + req.tableName + " " + to_string(req.pageNumber));
            }
            else if (req.opType == OP_WRITE)
            {
                if (tableCatalogue.isTable(req.tableName)) {
                    bufferManager.getPage(req.tableName, req.pageNumber);
                } else if (matrixCatalogue.isMatrix(req.tableName)) {
                    Matrix* matrix = matrixCatalogue.getMatrix(req.tableName);
                    matrixBufferManager.getPage(req.tableName, req.pageNumber, matrix->dimension);
                }
                writeLog("WRITE " + req.txId + " " + req.tableName + " " + to_string(req.pageNumber));
            }

            if (tx.commitDeferred && !tx.waiting)
            {
                tx.commitDeferred = false;
                processCommit(tx.txId, true);
            }
        }

        void tryGrantWaitingRequests()
        {
            bool progressed = true;
            while (progressed)
            {
                progressed = false;
                for (int i = 0; i < (int)waitingRequests.size(); i++)
                {
                    WaitingRequest req = waitingRequests[i];
                    if (!transactions.count(req.txId))
                        continue;

                    TransactionState &tx = transactions[req.txId];
                    if (tx.state != TX_ACTIVE)
                        continue;

                    LockEntry &entry = lockTable[req.pageKey];
                    if (hasConflictingOwner(tx, entry, req.lockMode))
                        continue;

                    grantLock(tx, req.pageKey, req.lockMode);
                    tx.waiting = false;
                    if (req.lockMode == LOCK_SHARED)
                        writeLog("Lock granted");
                    else
                        writeLog(tx.txId + " exclusive lock(" + req.tableName + " " + to_string(req.pageNumber) + ") granted");

                    waitingRequests.erase(waitingRequests.begin() + i);
                    executeGrantedOperation(req);
                    progressed = true;
                    break;
                }
            }
        }

        bool processReadWrite(const ScheduleOp &op, bool countTime)
        {
            if (countTime)
                clockTime++;

            if (!transactions.count(op.txId))
                return false;

            TransactionState &tx = transactions[op.txId];
            if (tx.state != TX_ACTIVE)
                return false;

            if (tx.waiting)
                return false;

            if (!tableAndPageValid(op.tableName, op.pageNumber))
                return false;

            LockMode requestedMode = (op.type == OP_READ) ? LOCK_SHARED : LOCK_EXCLUSIVE;
            string pageKey = makePageKey(op.tableName, op.pageNumber);
            writeLog(op.txId + " requests " + (requestedMode == LOCK_SHARED ? "shared" : "exclusive") + " lock(" + op.tableName + " " + to_string(op.pageNumber) + ")");

            LockEntry &entry = lockTable[pageKey];
            if (!hasConflictingOwner(tx, entry, requestedMode))
            {
                grantLock(tx, pageKey, requestedMode);
                writeLog("Lock granted");
                if (op.type == OP_READ)
                {
                    if (tableCatalogue.isTable(op.tableName)) {
                        Cursor cursor(op.tableName, op.pageNumber);
                        cursor.getNext();
                    } else if (matrixCatalogue.isMatrix(op.tableName)) {
                        Matrix* matrix = matrixCatalogue.getMatrix(op.tableName);
                        matrixBufferManager.getPage(op.tableName, op.pageNumber, matrix->dimension);
                    }
                    writeLog("READ " + op.txId + " " + op.tableName + " " + to_string(op.pageNumber));
                }
                else
                {
                    if (tableCatalogue.isTable(op.tableName)) {
                        bufferManager.getPage(op.tableName, op.pageNumber);
                    } else if (matrixCatalogue.isMatrix(op.tableName)) {
                        Matrix* matrix = matrixCatalogue.getMatrix(op.tableName);
                        matrixBufferManager.getPage(op.tableName, op.pageNumber, matrix->dimension);
                    }
                    writeLog("WRITE " + op.txId + " " + op.tableName + " " + to_string(op.pageNumber));
                }
                return true;
            }

            vector<string> blockers = getBlockingOwners(tx, entry, requestedMode);
            if (isOlderThanAllBlockers(tx, blockers))
            {
                tx.waiting = true;
                WaitingRequest req;
                req.txId = op.txId;
                req.pageKey = pageKey;
                req.tableName = op.tableName;
                req.pageNumber = op.pageNumber;
                req.opType = op.type;
                req.lockMode = requestedMode;
                waitingRequests.push_back(req);
                if (distinctTransactionCount <= 2)
                    writeLog(op.txId + " waits (older transaction allowed to wait)");
                else if (blockers.size() == 1)
                    writeLog(op.txId + " waits (older than " + blockers[0] + ", so allowed to wait)");
                else
                    writeLog(op.txId + " waits (older transaction allowed to wait)");
                return true;
            }

            if (distinctTransactionCount <= 2)
                writeLog(op.txId + " dies (younger transaction cannot wait for older)");
            else if (blockers.size() == 1)
                writeLog(op.txId + " dies (younger than " + blockers[0] + ", cannot wait)");
            else
                writeLog(op.txId + " dies (younger transaction cannot wait for older)");
            abortTransaction(op.txId);
            return true;
        }

        void processBegin(const string &txId, bool isRestart)
        {
            TransactionState &tx = transactions[txId];
            tx.txId = txId;
            tx.timestamp = clockTime;
            tx.state = TX_ACTIVE;
            tx.waiting = false;
            tx.commitDeferred = false;
            tx.heldPages.clear();
            tx.heldPagesOrder.clear();

            if (isRestart)
                writeLog("Restart " + txId);
            else
                writeLog("BEGIN " + txId);
        }

        void processCommit(const string &txId, bool countTime)
        {
            if (countTime)
                clockTime++;

            if (!transactions.count(txId))
                return;

            TransactionState &tx = transactions[txId];
            if (tx.state != TX_ACTIVE)
                return;

            if (tx.waiting)
            {
                tx.commitDeferred = true;
                return;
            }

            tx.state = TX_COMMITTED;
            writeLog("COMMIT " + txId);
            releaseAllLocks(tx);
            tryGrantWaitingRequests();
        }

        bool parseScheduleFile(const string &inputPath, vector<ScheduleOp> &ops)
        {
            ifstream fin(inputPath);
            if (!fin.is_open())
                return false;

            unordered_set<string> seenTxIds;

            string line;
            while (getline(fin, line))
            {
                vector<string> tokens = tokenizeLine(line);
                if (tokens.empty())
                    continue;

                ScheduleOp op;
                if (tokens[0] == "BEGIN" && tokens.size() == 2)
                {
                    op.type = OP_BEGIN;
                    op.txId = tokens[1];
                }
                else if (tokens[0] == "READ" && tokens.size() == 4)
                {
                    op.type = OP_READ;
                    op.txId = tokens[1];
                    op.tableName = tokens[2];
                    op.pageNumber = stoi(tokens[3]);
                }
                else if (tokens[0] == "WRITE" && tokens.size() == 4)
                {
                    op.type = OP_WRITE;
                    op.txId = tokens[1];
                    op.tableName = tokens[2];
                    op.pageNumber = stoi(tokens[3]);
                }
                else if (tokens[0] == "COMMIT" && tokens.size() == 2)
                {
                    op.type = OP_COMMIT;
                    op.txId = tokens[1];
                }
                else
                {
                    writeLog("SYNTAX ERROR");
                    continue;
                }

                if (!op.txId.empty())
                    seenTxIds.insert(op.txId);

                if (op.type != OP_BEGIN)
                    transactions[op.txId].replayProgram.push_back(op);
                ops.push_back(op);
            }

            distinctTransactionCount = seenTxIds.size();

            return true;
        }

    public:
        bool run(const string &inputPath, const string &outputPath)
        {
            fout.open(outputPath, ios::trunc);
            if (!fout.is_open())
            {
                cout << "SEMANTIC ERROR: File doesn't exist" << endl;
                return false;
            }

            vector<ScheduleOp> schedule;
            if (!parseScheduleFile(inputPath, schedule))
            {
                cout << "SEMANTIC ERROR: File doesn't exist" << endl;
                fout.close();
                return false;
            }

            for (const ScheduleOp &op : schedule)
            {
                if (op.type == OP_BEGIN)
                {
                    clockTime++;
                    processBegin(op.txId, false);
                    continue;
                }
                if (op.type == OP_READ || op.type == OP_WRITE)
                {
                    processReadWrite(op, true);
                    continue;
                }
                if (op.type == OP_COMMIT)
                {
                    processCommit(op.txId, true);
                    continue;
                }
            }

            vector<string> dedupRestartOrder;
            unordered_set<string> seen;
            for (const string &txId : restartOrder)
            {
                if (!seen.count(txId))
                {
                    seen.insert(txId);
                    dedupRestartOrder.push_back(txId);
                }
            }

            stable_sort(dedupRestartOrder.begin(), dedupRestartOrder.end(), [&](const string &a, const string &b)
                        {
                            return transactions[a].timestamp < transactions[b].timestamp;
                        });

            for (const string &txId : dedupRestartOrder)
            {
                if (!transactions.count(txId))
                    continue;

                clockTime++;
                processBegin(txId, true);
                vector<ScheduleOp> replayOps = transactions[txId].replayProgram;
                for (const ScheduleOp &op : replayOps)
                {
                    if (op.type == OP_READ || op.type == OP_WRITE)
                    {
                        processReadWrite(op, true);
                        if (transactions[txId].state == TX_ABORTED)
                            break;
                    }
                    else if (op.type == OP_COMMIT)
                    {
                        processCommit(op.txId, true);
                    }
                }
            }

            fout.close();
            return true;
        }
    };

    string outputFileNameFromInput(const string &inputFile)
    {
        string base = inputFile;
        if (base.size() >= 4 && base.substr(base.size() - 4) == ".txt")
            base = base.substr(0, base.size() - 4);
        return base + "_output.txt";
    }
}

bool syntacticParseTRANSACTION()
{
    logger.log("syntacticParseTRANSACTION");
    if (tokenizedQuery.size() != 2)
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    parsedQuery.queryType = TRANSACTION;
    parsedQuery.transactionInputFile = tokenizedQuery[1];
    return true;
}

bool semanticParseTRANSACTION()
{
    logger.log("semanticParseTRANSACTION");
    string inputPath = "data/" + parsedQuery.transactionInputFile;
    struct stat buffer;
    if (stat(inputPath.c_str(), &buffer) != 0)
    {
        cout << "SEMANTIC ERROR: File doesn't exist" << endl;
        return false;
    }
    return true;
}

void executeTRANSACTION()
{
    logger.log("executeTRANSACTION");
    string inputPath = "data/" + parsedQuery.transactionInputFile;
    string outputPath = "data/" + outputFileNameFromInput(parsedQuery.transactionInputFile);

    LockSimulator simulator;
    simulator.run(inputPath, outputPath);
}
